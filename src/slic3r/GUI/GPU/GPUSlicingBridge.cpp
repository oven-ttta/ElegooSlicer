#include "GPUSlicingBridge.hpp"
#include "GPUMeshSlicer.hpp"
#include "libslic3r/TriangleMeshSlicer.hpp"
#include "libslic3r/TriangleMesh.hpp"
#include "libslic3r/AppConfig.hpp"
#include "../GUI_App.hpp"

#include <wx/app.h>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

#include <boost/log/trivial.hpp>

namespace Slic3r {
namespace GUI {

static std::atomic<bool> s_gpu_bridge_initialized{false};
static std::atomic<bool> s_gpu_available{false};

// Convert GPU edge_flags to local edge indices (0, 1, 2)
// edge_flags bits:
//   0: Edge 0 (v0 -> v1) intersected
//   1: Edge 1 (v1 -> v2) intersected
//   2: Edge 2 (v2 -> v0) intersected
//   3: v0 on plane
//   4: v1 on plane
//   5: v2 on plane
static void parse_edge_flags(int edge_flags, const stl_triangle_vertex_indices& triangle_indices,
                              const Vec3i32& face_edge_id,
                              int& out_a_id, int& out_b_id, int& out_edge_a_id, int& out_edge_b_id)
{
    out_a_id = -1;
    out_b_id = -1;
    out_edge_a_id = -1;
    out_edge_b_id = -1;

    // Track which intersection points we've found
    int point_idx = 0;

    // Check edge intersections first (bits 0-2)
    for (int edge = 0; edge < 3; ++edge) {
        if ((edge_flags & (1 << edge)) && point_idx < 2) {
            // This edge was intersected (not at a vertex)
            if (point_idx == 0) {
                out_edge_a_id = face_edge_id(edge);
            } else {
                out_edge_b_id = face_edge_id(edge);
            }
            ++point_idx;
        }
    }

    // Check vertex-on-plane cases (bits 3-5)
    for (int vtx = 0; vtx < 3; ++vtx) {
        if ((edge_flags & (1 << (vtx + 3))) && point_idx < 2) {
            // This vertex is on the plane
            if (point_idx == 0) {
                out_a_id = triangle_indices(vtx);
            } else {
                out_b_id = triangle_indices(vtx);
            }
            ++point_idx;
        }
    }
}

// Worker function to perform GPU slicing on UI thread
static std::vector<GPUIntersectionLines> perform_gpu_slicing(
    const indexed_triangle_set& mesh,
    const std::vector<float>& zs,
    const Transform3d& trafo,
    const std::vector<Vec3i32>& face_edge_ids)
{
    std::vector<GPUIntersectionLines> result;

    if (!GPUMeshSlicer::is_available()) {
        BOOST_LOG_TRIVIAL(debug) << "GPU Slicing Bridge: GPU not available";
        return result;
    }

    GPUMeshSlicer& gpu_slicer = GPUMeshSlicer::instance();

    if (!gpu_slicer.is_initialized()) {
        if (!gpu_slicer.init()) {
            BOOST_LOG_TRIVIAL(warning) << "GPU Slicing Bridge: Failed to initialize GPU slicer: "
                                        << gpu_slicer.get_error();
            return result;
        }
    }

    // Perform GPU slicing
    auto gpu_lines = gpu_slicer.slice(mesh, zs, trafo);

    if (gpu_lines.empty()) {
        BOOST_LOG_TRIVIAL(warning) << "GPU Slicing Bridge: GPU slicing returned no results";
        return result;
    }

    // Convert GPU intersection lines to GPUIntersectionLines with proper edge IDs
    result.resize(zs.size());

    for (size_t slice_idx = 0; slice_idx < gpu_lines.size(); ++slice_idx) {
        const auto& lines = gpu_lines[slice_idx];
        result[slice_idx].reserve(lines.size());

        for (const auto& line : lines) {
            GPUIntersectionResult ir;
            ir.a = line.a;
            ir.b = line.b;

            // Look up the face edge IDs for this triangle
            if (line.triangle_id >= 0 && line.triangle_id < (int)face_edge_ids.size()) {
                const Vec3i32& face_edges = face_edge_ids[line.triangle_id];
                const stl_triangle_vertex_indices& indices = mesh.indices[line.triangle_id];

                // Parse edge_flags to determine a_id, b_id, edge_a_id, edge_b_id
                parse_edge_flags(line.edge_flags, indices, face_edges,
                                ir.a_id, ir.b_id, ir.edge_a_id, ir.edge_b_id);
            }

            result[slice_idx].push_back(std::move(ir));
        }
    }

    auto stats = gpu_slicer.get_stats();
    BOOST_LOG_TRIVIAL(info) << "GPU Slicing Bridge: Completed - "
                             << stats.triangles_processed << " triangles, "
                             << stats.intersections_found << " intersections, "
                             << "time: " << (stats.upload_time_ms + stats.compute_time_ms + stats.download_time_ms) << "ms";

    return result;
}

void init_gpu_slicing_bridge()
{
    if (s_gpu_bridge_initialized.exchange(true)) {
        BOOST_LOG_TRIVIAL(warning) << "GPU Slicing Bridge: Already initialized, skipping";
        return; // Already initialized
    }

    BOOST_LOG_TRIVIAL(warning) << "GPU Slicing Bridge: Initializing...";
    BOOST_LOG_TRIVIAL(warning) << "GPU Slicing Bridge: Checking GPU availability...";

    // Check if GPU is available
    s_gpu_available = GPUMeshSlicer::is_available();
    BOOST_LOG_TRIVIAL(warning) << "GPU Slicing Bridge: GPU available = " << s_gpu_available.load();

    if (s_gpu_available) {
        // Register the GPU slicing callback with libslic3r
        set_gpu_slicing_callback([](const indexed_triangle_set& mesh,
                                    const std::vector<float>& zs,
                                    const Transform3d& trafo,
                                    const std::vector<Vec3i32>& face_edge_ids) -> std::vector<GPUIntersectionLines> {
            return gpu_slice_mesh(mesh, zs, trafo, face_edge_ids);
        });

        // Read preference and set GPU slicing enabled/disabled
        bool use_gpu = true; // Default to enabled
        AppConfig* app_config = wxGetApp().app_config;
        if (app_config) {
            // Set default value if not exists
            if (app_config->get("use_gpu_slicing").empty()) {
                app_config->set_bool("use_gpu_slicing", true);
                app_config->save();
            }
            use_gpu = app_config->get_bool("use_gpu_slicing");
        }
        set_gpu_slicing_enabled(use_gpu);

        BOOST_LOG_TRIVIAL(info) << "GPU Slicing Bridge: Initialized successfully, GPU acceleration "
                                 << (use_gpu ? "enabled" : "disabled") << " (preference)";
    } else {
        BOOST_LOG_TRIVIAL(info) << "GPU Slicing Bridge: GPU not available, using CPU slicing";
    }
}

void shutdown_gpu_slicing_bridge()
{
    if (!s_gpu_bridge_initialized.exchange(false)) {
        return; // Not initialized
    }

    BOOST_LOG_TRIVIAL(info) << "GPU Slicing Bridge: Shutting down...";

    // Clear the callback
    set_gpu_slicing_callback(nullptr);

    // Shutdown GPU slicer
    if (s_gpu_available) {
        GPUMeshSlicer::instance().shutdown();
    }

    s_gpu_available = false;
}

std::vector<GPUIntersectionLines> gpu_slice_mesh(
    const indexed_triangle_set& mesh,
    const std::vector<float>& zs,
    const Transform3d& trafo,
    const std::vector<Vec3i32>& face_edge_ids)
{
    BOOST_LOG_TRIVIAL(warning) << "gpu_slice_mesh: Called with " << mesh.indices.size() << " triangles, "
                                << zs.size() << " slices, gpu_available=" << s_gpu_available.load();

    if (!s_gpu_available || mesh.indices.empty() || zs.empty()) {
        BOOST_LOG_TRIVIAL(warning) << "gpu_slice_mesh: Returning empty - gpu_available=" << s_gpu_available.load()
                                    << ", mesh empty=" << mesh.indices.empty() << ", zs empty=" << zs.empty();
        return {};
    }

    // Check if we're on the main thread
    bool is_main = wxThread::IsMain();
    BOOST_LOG_TRIVIAL(warning) << "gpu_slice_mesh: Is main thread=" << is_main;

    if (is_main) {
        // We're on the main thread, can execute directly
        BOOST_LOG_TRIVIAL(warning) << "gpu_slice_mesh: Executing directly on main thread";
        return perform_gpu_slicing(mesh, zs, trafo, face_edge_ids);
    }

    // We're on a background thread, need to schedule on main thread
    // Use a timeout to prevent deadlock if main thread is blocked
    std::vector<GPUIntersectionLines> result;
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;

    BOOST_LOG_TRIVIAL(warning) << "gpu_slice_mesh: Scheduling on main thread via CallAfter";

    // Schedule on main thread
    wxTheApp->CallAfter([&]() {
        BOOST_LOG_TRIVIAL(warning) << "gpu_slice_mesh: CallAfter executing on main thread";
        result = perform_gpu_slicing(mesh, zs, trafo, face_edge_ids);

        std::lock_guard<std::mutex> lock(mutex);
        done = true;
        cv.notify_one();
    });

    // Wait for completion with timeout (5 seconds)
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (!cv.wait_for(lock, std::chrono::seconds(5), [&done]{ return done; })) {
            BOOST_LOG_TRIVIAL(warning) << "gpu_slice_mesh: Timeout waiting for main thread - falling back to CPU";
            return {}; // Return empty to trigger CPU fallback
        }
    }

    BOOST_LOG_TRIVIAL(warning) << "gpu_slice_mesh: Completed with " << result.size() << " slices";
    return result;
}

bool is_gpu_slicing_available()
{
    return s_gpu_available;
}

} // namespace GUI
} // namespace Slic3r
