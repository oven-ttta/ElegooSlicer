#ifndef slic3r_SceneRaycasterManager_hpp_
#define slic3r_SceneRaycasterManager_hpp_

#include "SceneRaycaster.hpp"
#include "libslic3r/Point.hpp"

#include <vector>

namespace Slic3r {

class GLVolume;

namespace GUI {

struct Camera;
class GPUColorPicker;

/**
 * @brief SceneRaycaster 的 GPU 拾取管理器
 *
 * - 当 MeshRaycaster 尚未就绪时，使用 GPU Color Picking 兜底
 * - MeshRaycaster 构建完成后自动切换回 CPU 精确射线
 * - 暴露测试开关以便强制使用 GPU/CPU 方案
 */
class SceneRaycasterManager
{
public:
    enum class PickingSolution
    {
        None = 0,
        CpuRaycaster,
        GpuColorPicking
    };

    /**
     * @brief 增强版拾取接口，自动选择 GPU 或 CPU 方案
     *
     * @param raycaster 原始 SceneRaycaster
     * @param volumes   所有 GLVolume 列表（用于 GPU 拾取）
     * @param mouse_pos 鼠标坐标（窗口坐标）
     * @param camera    当前相机
     * @param gpu_picker GPU Color Picker 实例
     * @param clipping_plane 可选裁剪平面
     */
    static SceneRaycaster::HitResult hit_with_fallback(
        const SceneRaycaster& raycaster,
        const std::vector<Slic3r::GLVolume*>& volumes,
        const Vec2d& mouse_pos,
        const Camera& camera,
        GPUColorPicker* gpu_picker,
        const ClippingPlane* clipping_plane = nullptr
    );

    static PickingSolution get_last_picking_solution();

    /**
     * @brief 强制选择拾取方案（测试开关）
     *
     * - None：自动模式
     * - CpuRaycaster：强制使用 MeshRaycaster
     * - GpuColorPicking：强制使用 GPU Color Picking
     */
    static void set_forced_picking_solution(PickingSolution solution);
    static PickingSolution get_forced_picking_solution();

private:
    static bool is_volume_raycaster_ready(const Slic3r::GLVolume* volume);

    static SceneRaycaster::HitResult gpu_pick(
        const std::vector<Slic3r::GLVolume*>& volumes,
        const Vec2d& mouse_pos,
        const Camera& camera,
        GPUColorPicker* gpu_picker
    );

    /**
     * @brief 根据 volume 索引与深度值构造 HitResult
     *
     * @param depth 来自 GPU FBO 的深度值，用于反投影到世界坐标
     */
    static SceneRaycaster::HitResult create_hit_result_from_volume_idx(
        const std::vector<Slic3r::GLVolume*>& volumes,
        int volume_idx,
        const Vec2d& mouse_pos,
        const Camera& camera,
        float depth
    );
};

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_SceneRaycasterManager_hpp_

