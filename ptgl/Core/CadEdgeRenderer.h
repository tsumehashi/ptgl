#ifndef PTGL_CORE_CADEDGERENDERER_H_
#define PTGL_CORE_CADEDGERENDERER_H_

#include <functional>
#include "StyledGraphicsView.h"
#include "SceneRenderSupport.h"
#include "RenderTimer.h"

namespace ptgl
{
namespace detail
{

// Supersampled surface edges plus explicit mesh feature edges. Captures may be
// reused for static scenes; width/color/angle are always applied on this frame.
class CadEdgeRenderer
{
  public:
    void initialize();
    void release();
    ShaderProgramPtr captureProgram() const { return capture_; }
    bool render(const Eigen::Matrix4d &projection, const Eigen::Matrix4d &view,
                const StyledGraphicsView::EdgeSettings &settings, const RenderQualitySettings &quality,
                std::uint64_t revision, RenderStatistics &stats, const std::function<void(bool depth)> &draw);
    void addFeatures(std::shared_ptr<const VertexList> vertices, const Eigen::Matrix4d &model);

  private:
    SceneRenderTarget depth_, normals_, mask_;
    VertexBufferObject quad_, featureBuffer_;
    ShaderProgramPtr capture_, composite_, resolve_, feature_;
    struct Features {
        std::shared_ptr<const VertexList> vertices;
        Eigen::Matrix4d model;
    };
    std::vector<Features> features_;
    bool cached_ = false;
    int requestedWidth_ = 0, requestedHeight_ = 0, requestedScale_ = 0, actualScale_ = 0;
    std::uint64_t revision_ = 0;
    Eigen::Matrix4d projection_ = Eigen::Matrix4d::Zero(), view_ = Eigen::Matrix4d::Zero();
    RenderTimer timer_;
};

} // namespace detail
} // namespace ptgl
#endif
