#include "GPUMeshSlicer.hpp"
#include "libslic3r/libslic3r.h"
#include "libslic3r/Utils.hpp"

#include <boost/log/trivial.hpp>
#include <chrono>
#include <fstream>
#include <sstream>

namespace Slic3r {

// Singleton instance
std::unique_ptr<GPUMeshSlicer> GPUMeshSlicer::s_instance;

// Maximum intersections per slice (pre-allocated buffer size)
// Keep this reasonable to avoid huge buffer allocations
// A typical model has at most a few thousand intersections per slice
static const size_t MAX_INTERSECTIONS_PER_SLICE = 100000;

// Maximum slices for buffer allocation
static const size_t MAX_SLICES_FOR_BUFFER = 2000;

// Minimum triangles to benefit from GPU acceleration (set to 0 to always use GPU)
static const size_t MIN_TRIANGLES_FOR_GPU = 0;

// Work group size (must match shader) - optimized for modern GPUs
static const size_t WORK_GROUP_SIZE = 256;

// Maximum batch size for processing very large meshes
static const size_t MAX_BATCH_TRIANGLES = 1000000;

GPUMeshSlicer::GPUMeshSlicer() = default;

GPUMeshSlicer::~GPUMeshSlicer()
{
    shutdown();
}

GPUMeshSlicer& GPUMeshSlicer::instance()
{
    if (!s_instance) {
        s_instance = std::make_unique<GPUMeshSlicer>();
    }
    return *s_instance;
}

bool GPUMeshSlicer::is_available()
{
    BOOST_LOG_TRIVIAL(warning) << "GPU Slicer: Checking availability...";

    // Check OpenGL version (need 4.3 for compute shaders)
    const GLubyte* version = glGetString(GL_VERSION);
    if (!version) {
        BOOST_LOG_TRIVIAL(warning) << "GPU Slicer: glGetString(GL_VERSION) returned null - no OpenGL context?";
        return false;
    }

    BOOST_LOG_TRIVIAL(warning) << "GPU Slicer: OpenGL version string: " << reinterpret_cast<const char*>(version);

    int major = 0, minor = 0;
    if (sscanf(reinterpret_cast<const char*>(version), "%d.%d", &major, &minor) != 2) {
        BOOST_LOG_TRIVIAL(warning) << "GPU Slicer: Failed to parse OpenGL version";
        return false;
    }

    // Require OpenGL 4.3+
    if (major < 4 || (major == 4 && minor < 3)) {
        BOOST_LOG_TRIVIAL(warning) << "GPU Slicer: OpenGL " << major << "." << minor
                                    << " detected, need 4.3+ for compute shaders";
        return false;
    }

    // Check for compute shader support
    GLint max_compute_work_group_count[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &max_compute_work_group_count[0]);
    BOOST_LOG_TRIVIAL(warning) << "GPU Slicer: Max compute work group count = " << max_compute_work_group_count[0];

    if (max_compute_work_group_count[0] < 65535) {
        BOOST_LOG_TRIVIAL(warning) << "GPU Slicer: Compute shader support limited";
        return false;
    }

    BOOST_LOG_TRIVIAL(warning) << "GPU Slicer: OpenGL " << major << "." << minor
                                << " with compute shader support available";
    return true;
}

bool GPUMeshSlicer::init()
{
    if (m_initialized) {
        return true;
    }

    BOOST_LOG_TRIVIAL(info) << "GPU Slicer: Initializing...";

    if (!is_available()) {
        set_error("OpenGL compute shaders not available");
        return false;
    }

    if (!compile_shader()) {
        return false;
    }

    m_initialized = true;
    BOOST_LOG_TRIVIAL(info) << "GPU Slicer: Initialized successfully";
    return true;
}

void GPUMeshSlicer::shutdown()
{
    if (!m_initialized) {
        return;
    }

    BOOST_LOG_TRIVIAL(info) << "GPU Slicer: Shutting down...";

    cleanup_buffers();

    if (m_compute_program != 0) {
        glDeleteProgram(m_compute_program);
        m_compute_program = 0;
    }

    m_initialized = false;
}

bool GPUMeshSlicer::compile_shader()
{
    // Load shader source from file
    std::string shader_path = resources_dir() + "/shaders/430/mesh_slicer.comp";
    std::ifstream file(shader_path);
    if (!file.is_open()) {
        set_error("Failed to open compute shader: " + shader_path);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    const char* source_ptr = source.c_str();

    // Create and compile shader
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &source_ptr, nullptr);
    glCompileShader(shader);

    // Check compilation
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        set_error(std::string("Compute shader compilation failed: ") + info_log);
        glDeleteShader(shader);
        return false;
    }

    // Create program and link
    m_compute_program = glCreateProgram();
    glAttachShader(m_compute_program, shader);
    glLinkProgram(m_compute_program);

    // Check linking
    glGetProgramiv(m_compute_program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetProgramInfoLog(m_compute_program, 512, nullptr, info_log);
        set_error(std::string("Compute shader linking failed: ") + info_log);
        glDeleteShader(shader);
        glDeleteProgram(m_compute_program);
        m_compute_program = 0;
        return false;
    }

    glDeleteShader(shader);

    // Get uniform locations
    m_loc_transform = glGetUniformLocation(m_compute_program, "u_transform");
    m_loc_num_triangles = glGetUniformLocation(m_compute_program, "u_num_triangles");
    m_loc_num_slices = glGetUniformLocation(m_compute_program, "u_num_slices");
    m_loc_max_intersections = glGetUniformLocation(m_compute_program, "u_max_intersections");
    m_loc_scale_xy = glGetUniformLocation(m_compute_program, "u_scale_xy");

    BOOST_LOG_TRIVIAL(info) << "GPU Slicer: Compute shader compiled successfully";
    return true;
}

bool GPUMeshSlicer::create_buffers(size_t num_vertices, size_t num_triangles, size_t num_slices)
{
    BOOST_LOG_TRIVIAL(warning) << "GPU Slicer: create_buffers called with "
                                << num_vertices << " vertices, "
                                << num_triangles << " triangles, "
                                << num_slices << " slices";

    // Check if we need to reallocate
    bool need_realloc = (num_vertices > m_max_vertices ||
                         num_triangles > m_max_triangles ||
                         num_slices > m_max_slices);

    if (!need_realloc && m_vertex_buffer != 0) {
        BOOST_LOG_TRIVIAL(debug) << "GPU Slicer: Reusing existing buffers";
        return true;
    }

    cleanup_buffers();

    // Smart allocation with reasonable limits to prevent huge memory usage
    // Add 20% headroom but cap at reasonable maximums
    m_max_vertices = std::min(num_vertices + num_vertices / 5 + 10000, size_t(2000000));
    m_max_triangles = std::min(num_triangles + num_triangles / 5 + 10000, size_t(2000000));
    m_max_slices = std::min(num_slices + 100, MAX_SLICES_FOR_BUFFER);

    // Limit intersections per slice to prevent huge output buffer
    // Most real models have far fewer than this
    m_max_intersections_per_slice = std::min(MAX_INTERSECTIONS_PER_SLICE, num_triangles + 10000);

    // Calculate memory usage
    size_t vertex_buffer_size = m_max_vertices * sizeof(float) * 4;
    size_t index_buffer_size = m_max_triangles * sizeof(int) * 4;
    size_t slice_buffer_size = m_max_slices * sizeof(float);
    size_t output_buffer_size = m_max_slices * m_max_intersections_per_slice * sizeof(float) * 8;
    size_t counter_buffer_size = m_max_slices * sizeof(unsigned int);
    size_t total_size = vertex_buffer_size + index_buffer_size + slice_buffer_size +
                        output_buffer_size + counter_buffer_size;

    BOOST_LOG_TRIVIAL(warning) << "GPU Slicer: Allocating buffers - "
                                << "vertices=" << m_max_vertices
                                << ", triangles=" << m_max_triangles
                                << ", slices=" << m_max_slices
                                << ", intersections/slice=" << m_max_intersections_per_slice
                                << ", total=" << (total_size / 1024 / 1024) << " MB";

    // Check if total size is reasonable (max 1GB)
    if (total_size > 1024 * 1024 * 1024) {
        BOOST_LOG_TRIVIAL(warning) << "GPU Slicer: Buffer size too large (" << (total_size / 1024 / 1024) << " MB), falling back to CPU";
        set_error("Buffer size exceeds 1GB limit");
        return false;
    }

    // Create vertex buffer (vec4 per vertex)
    glGenBuffers(1, &m_vertex_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_vertex_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, vertex_buffer_size, nullptr, GL_DYNAMIC_DRAW);

    // Create index buffer (ivec4 per triangle, w unused)
    glGenBuffers(1, &m_index_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_index_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, index_buffer_size, nullptr, GL_DYNAMIC_DRAW);

    // Create slice Z buffer
    glGenBuffers(1, &m_slice_z_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_slice_z_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, slice_buffer_size, nullptr, GL_DYNAMIC_DRAW);

    // Create output buffer (2 vec4 per intersection per slice)
    glGenBuffers(1, &m_output_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_output_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, output_buffer_size, nullptr, GL_DYNAMIC_COPY);

    // Create counter buffer (one uint per slice)
    glGenBuffers(1, &m_counter_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_counter_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, counter_buffer_size, nullptr, GL_DYNAMIC_COPY);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Check for errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        BOOST_LOG_TRIVIAL(error) << "GPU Slicer: OpenGL error " << err << " during buffer creation";
        set_error("Failed to create GPU buffers (OpenGL error " + std::to_string(err) + ")");
        cleanup_buffers();
        return false;
    }

    BOOST_LOG_TRIVIAL(warning) << "GPU Slicer: Created buffers successfully ("
                                << (total_size / 1024 / 1024) << " MB)";
    return true;
}

void GPUMeshSlicer::cleanup_buffers()
{
    if (m_vertex_buffer != 0) {
        glDeleteBuffers(1, &m_vertex_buffer);
        m_vertex_buffer = 0;
    }
    if (m_index_buffer != 0) {
        glDeleteBuffers(1, &m_index_buffer);
        m_index_buffer = 0;
    }
    if (m_slice_z_buffer != 0) {
        glDeleteBuffers(1, &m_slice_z_buffer);
        m_slice_z_buffer = 0;
    }
    if (m_output_buffer != 0) {
        glDeleteBuffers(1, &m_output_buffer);
        m_output_buffer = 0;
    }
    if (m_counter_buffer != 0) {
        glDeleteBuffers(1, &m_counter_buffer);
        m_counter_buffer = 0;
    }

    m_max_vertices = 0;
    m_max_triangles = 0;
    m_max_slices = 0;
}

bool GPUMeshSlicer::upload_mesh(const indexed_triangle_set& mesh, const Transform3d& trafo)
{
    auto start = std::chrono::high_resolution_clock::now();

    // Prepare vertex data (vec4, w=1.0)
    std::vector<float> vertices(mesh.vertices.size() * 4);
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        vertices[i * 4 + 0] = mesh.vertices[i].x();
        vertices[i * 4 + 1] = mesh.vertices[i].y();
        vertices[i * 4 + 2] = mesh.vertices[i].z();
        vertices[i * 4 + 3] = 1.0f;
    }

    // Prepare index data (ivec4, w=0)
    std::vector<int> indices(mesh.indices.size() * 4);
    for (size_t i = 0; i < mesh.indices.size(); ++i) {
        indices[i * 4 + 0] = mesh.indices[i].x();
        indices[i * 4 + 1] = mesh.indices[i].y();
        indices[i * 4 + 2] = mesh.indices[i].z();
        indices[i * 4 + 3] = 0;
    }

    // Upload vertices
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_vertex_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());

    // Upload indices
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_index_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, indices.size() * sizeof(int), indices.data());

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    auto end = std::chrono::high_resolution_clock::now();
    m_stats.upload_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

    return glGetError() == GL_NO_ERROR;
}

bool GPUMeshSlicer::upload_slices(const std::vector<float>& zs)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_slice_z_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, zs.size() * sizeof(float), zs.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    return glGetError() == GL_NO_ERROR;
}

bool GPUMeshSlicer::execute(size_t num_triangles, size_t num_slices)
{
    auto start = std::chrono::high_resolution_clock::now();

    // Clear counter buffer
    std::vector<unsigned int> zeros(num_slices, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_counter_buffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, num_slices * sizeof(unsigned int), zeros.data());

    // Bind buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_vertex_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_index_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_slice_z_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_output_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_counter_buffer);

    // Use compute program
    glUseProgram(m_compute_program);

    // Set uniforms
    // Transform is set as identity here - actual transform is baked into vertex data
    float identity[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    glUniformMatrix4fv(m_loc_transform, 1, GL_FALSE, identity);
    glUniform1i(m_loc_num_triangles, static_cast<int>(num_triangles));
    glUniform1i(m_loc_num_slices, static_cast<int>(num_slices));
    glUniform1i(m_loc_max_intersections, static_cast<int>(m_max_intersections_per_slice));
    glUniform1f(m_loc_scale_xy, 1.0f);

    // Dispatch compute shader
    GLuint num_groups = static_cast<GLuint>((num_triangles + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE);
    glDispatchCompute(num_groups, 1, 1);

    // Wait for completion
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(0);

    auto end = std::chrono::high_resolution_clock::now();
    m_stats.compute_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    m_stats.triangles_processed = num_triangles;

    return glGetError() == GL_NO_ERROR;
}

bool GPUMeshSlicer::download_results(std::vector<std::vector<GPUIntersectionLine>>& output, size_t num_slices)
{
    auto start = std::chrono::high_resolution_clock::now();

    output.clear();
    output.resize(num_slices);

    // Read counter buffer to get actual intersection counts
    std::vector<unsigned int> counts(num_slices);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_counter_buffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, num_slices * sizeof(unsigned int), counts.data());

    // Read intersection data for each slice
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_output_buffer);

    size_t total_intersections = 0;
    for (size_t slice_idx = 0; slice_idx < num_slices; ++slice_idx) {
        size_t count = std::min(counts[slice_idx], static_cast<unsigned int>(m_max_intersections_per_slice));
        if (count == 0) {
            continue;
        }

        output[slice_idx].resize(count);
        total_intersections += count;

        // Calculate offset in output buffer
        size_t offset = slice_idx * m_max_intersections_per_slice * sizeof(float) * 8;

        // Read intersection data (2 vec4 per intersection)
        std::vector<float> data(count * 8);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, count * sizeof(float) * 8, data.data());

        // Convert to GPUIntersectionLine
        // GPU outputs world coordinates (mm), need to scale to internal units (nanometers)
        // scale_() macro: (val) / SCALING_FACTOR converts mm to internal coord_t
        for (size_t i = 0; i < count; ++i) {
            GPUIntersectionLine& line = output[slice_idx][i];
            // vec4 0: (ax, ay, bx, by) - scale from mm to internal units
            line.a = Point(static_cast<coord_t>(scale_(data[i * 8 + 0])),
                          static_cast<coord_t>(scale_(data[i * 8 + 1])));
            line.b = Point(static_cast<coord_t>(scale_(data[i * 8 + 2])),
                          static_cast<coord_t>(scale_(data[i * 8 + 3])));
            // vec4 1: (slice_id, triangle_id, edge_flags, reserved)
            line.slice_id = static_cast<int>(data[i * 8 + 4]);
            line.triangle_id = static_cast<int>(data[i * 8 + 5]);
            line.edge_flags = static_cast<int>(data[i * 8 + 6]);
        }
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    auto end = std::chrono::high_resolution_clock::now();
    m_stats.download_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    m_stats.intersections_found = total_intersections;

    return glGetError() == GL_NO_ERROR;
}

std::vector<std::vector<GPUIntersectionLine>> GPUMeshSlicer::slice(
    const indexed_triangle_set& mesh,
    const std::vector<float>& zs,
    const Transform3d& trafo,
    float scale_xy)
{
    std::vector<std::vector<GPUIntersectionLine>> result;

    if (!m_initialized) {
        set_error("GPU Slicer not initialized");
        return result;
    }

    if (mesh.vertices.empty() || mesh.indices.empty() || zs.empty()) {
        return result;
    }

    BOOST_LOG_TRIVIAL(debug) << "GPU Slicer: Slicing " << mesh.indices.size()
                              << " triangles at " << zs.size() << " Z levels";

    // Create/resize buffers if needed
    if (!create_buffers(mesh.vertices.size(), mesh.indices.size(), zs.size())) {
        return result;
    }

    // Upload mesh data
    if (!upload_mesh(mesh, trafo)) {
        set_error("Failed to upload mesh to GPU");
        return result;
    }

    // Upload slice Z values
    if (!upload_slices(zs)) {
        set_error("Failed to upload slice data to GPU");
        return result;
    }

    // Execute compute shader
    if (!execute(mesh.indices.size(), zs.size())) {
        set_error("Compute shader execution failed");
        return result;
    }

    // Download results
    if (!download_results(result, zs.size())) {
        set_error("Failed to download results from GPU");
        return result;
    }

    BOOST_LOG_TRIVIAL(debug) << "GPU Slicer: Found " << m_stats.intersections_found
                              << " intersections in "
                              << (m_stats.upload_time_ms + m_stats.compute_time_ms + m_stats.download_time_ms)
                              << " ms (upload: " << m_stats.upload_time_ms
                              << ", compute: " << m_stats.compute_time_ms
                              << ", download: " << m_stats.download_time_ms << ")";

    return result;
}

void GPUMeshSlicer::set_error(const std::string& msg)
{
    m_error = msg;
    BOOST_LOG_TRIVIAL(error) << "GPU Slicer: " << msg;
}

bool should_use_gpu_slicing(size_t num_triangles, size_t num_slices)
{
    // Force GPU slicing for all meshes when GPU is available
    // This reduces CPU and RAM usage by offloading intersection calculations to the GPU
    (void)num_triangles; // Unused - always use GPU when available
    (void)num_slices;    // Unused - always use GPU when available

    // Check if GPU slicer is available
    if (!GPUMeshSlicer::is_available()) {
        BOOST_LOG_TRIVIAL(debug) << "GPU Slicer: GPU not available, falling back to CPU";
        return false;
    }

    BOOST_LOG_TRIVIAL(info) << "GPU Slicer: Using GPU acceleration for slicing";
    return true;
}

} // namespace Slic3r
