#ifndef slic3r_GPUMeshSlicer_hpp_
#define slic3r_GPUMeshSlicer_hpp_

#include <vector>
#include <memory>
#include <string>

#include "libslic3r/TriangleMesh.hpp"
#include "libslic3r/Point.hpp"
#include "libslic3r/Polygon.hpp"

#include <GL/glew.h>

namespace Slic3r {

// Forward declaration
struct MeshSlicingParams;

// GPU Intersection Line - simplified structure for GPU output
struct GPUIntersectionLine {
    Point a;
    Point b;
    int slice_id;
    int triangle_id;
    int edge_flags;
};

// GPU Mesh Slicer - accelerates triangle-plane intersection using OpenGL compute shaders
class GPUMeshSlicer {
public:
    GPUMeshSlicer();
    ~GPUMeshSlicer();

    // Non-copyable
    GPUMeshSlicer(const GPUMeshSlicer&) = delete;
    GPUMeshSlicer& operator=(const GPUMeshSlicer&) = delete;

    // Check if GPU slicing is available
    static bool is_available();

    // Get singleton instance
    static GPUMeshSlicer& instance();

    // Initialize GPU resources (must be called with valid OpenGL context)
    bool init();

    // Release GPU resources
    void shutdown();

    // Check if initialized
    bool is_initialized() const { return m_initialized; }

    // Main slicing function - returns intersection lines per slice
    // Output is compatible with make_loops() in TriangleMeshSlicer
    std::vector<std::vector<GPUIntersectionLine>> slice(
        const indexed_triangle_set& mesh,
        const std::vector<float>& zs,
        const Transform3d& trafo,
        float scale_xy = 1.0f
    );

    // Get last error message
    const std::string& get_error() const { return m_error; }

    // Performance statistics
    struct Stats {
        double upload_time_ms = 0;
        double compute_time_ms = 0;
        double download_time_ms = 0;
        size_t triangles_processed = 0;
        size_t intersections_found = 0;
    };
    const Stats& get_stats() const { return m_stats; }

private:
    // Compile compute shader
    bool compile_shader();

    // Create GPU buffers
    bool create_buffers(size_t num_vertices, size_t num_triangles, size_t num_slices);

    // Upload mesh data to GPU
    bool upload_mesh(const indexed_triangle_set& mesh, const Transform3d& trafo);

    // Upload slice Z values
    bool upload_slices(const std::vector<float>& zs);

    // Execute compute shader
    bool execute(size_t num_triangles, size_t num_slices);

    // Download results from GPU
    bool download_results(std::vector<std::vector<GPUIntersectionLine>>& output, size_t num_slices);

    // Cleanup buffers
    void cleanup_buffers();

    // Set error message
    void set_error(const std::string& msg);

private:
    bool m_initialized = false;
    std::string m_error;
    Stats m_stats;

    // OpenGL handles
    GLuint m_compute_program = 0;
    GLuint m_vertex_buffer = 0;
    GLuint m_index_buffer = 0;
    GLuint m_slice_z_buffer = 0;
    GLuint m_output_buffer = 0;
    GLuint m_counter_buffer = 0;

    // Uniform locations
    GLint m_loc_transform = -1;
    GLint m_loc_num_triangles = -1;
    GLint m_loc_num_slices = -1;
    GLint m_loc_max_intersections = -1;
    GLint m_loc_scale_xy = -1;

    // Buffer sizes
    size_t m_max_vertices = 0;
    size_t m_max_triangles = 0;
    size_t m_max_slices = 0;
    size_t m_max_intersections_per_slice = 0;

    // Singleton instance
    static std::unique_ptr<GPUMeshSlicer> s_instance;
};

// Helper function to check if GPU slicing should be used
bool should_use_gpu_slicing(size_t num_triangles, size_t num_slices);

} // namespace Slic3r

#endif // slic3r_GPUMeshSlicer_hpp_
