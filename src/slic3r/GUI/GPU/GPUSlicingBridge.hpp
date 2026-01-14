#ifndef slic3r_GPUSlicingBridge_hpp_
#define slic3r_GPUSlicingBridge_hpp_

#include "libslic3r/TriangleMesh.hpp"
#include "libslic3r/TriangleMeshSlicer.hpp"
#include <vector>

namespace Slic3r {
namespace GUI {

// Initialize GPU slicing bridge - call this after OpenGL context is ready
void init_gpu_slicing_bridge();

// Shutdown GPU slicing bridge
void shutdown_gpu_slicing_bridge();

// GPU slicing function that can be called from background thread
// It schedules the work on the UI thread and waits for completion
// Returns GPUIntersectionLines with proper edge connectivity information
std::vector<GPUIntersectionLines> gpu_slice_mesh(
    const indexed_triangle_set& mesh,
    const std::vector<float>& zs,
    const Transform3d& trafo,
    const std::vector<Vec3i32>& face_edge_ids
);

// Check if GPU slicing is available
bool is_gpu_slicing_available();

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_GPUSlicingBridge_hpp_
