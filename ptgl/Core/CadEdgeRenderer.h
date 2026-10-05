#ifndef PTGL_CORE_CADEDGERENDERER_H_
#define PTGL_CORE_CADEDGERENDERER_H_

#include <functional>
#include "PlasticGraphicsView.h"
#include "SceneRenderSupport.h"

namespace ptgl
{
namespace detail
{

// Screen-space prototype: capture the nearest lit surface twice (packed depth
// and view normals), then composite edges before the view's overlays and UI.
class CadEdgeRenderer
{
  public:
    void initialize();
    void release();
    ShaderProgramPtr surfaceProgram() const { return surface_; }
    ShaderProgramPtr captureProgram() const { return capture_; }
    bool render(const Eigen::Matrix4d &projection, const PlasticGraphicsView::EdgeSettings &settings,
                const std::function<void(bool depth)> &draw);

  private:
    SceneRenderTarget depth_, normals_;
    VertexBufferObject quad_;
    ShaderProgramPtr surface_, capture_, composite_;
};

} // namespace detail
} // namespace ptgl
#endif
