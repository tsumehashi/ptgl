#ifndef PTGL_CORE_SCENESTYLERENDERER_H_
#define PTGL_CORE_SCENESTYLERENDERER_H_
#include "StyledGraphicsView.h"

namespace ptgl
{
namespace detail
{
// Coordinates common geometry and passes; style-specific shading lives in
// SurfaceRenderer and edge composition in CadEdgeRenderer.
class SceneStyleRenderer : public Renderer3D
{
  public:
    explicit SceneStyleRenderer(GraphicsView *view) : Renderer3D(view) {}
    StyledGraphicsView::RenderStyle style = StyledGraphicsView::RenderStyle::Plastic;
    Material defaultMaterial;
    PlasticLighting lighting;
    StyledGraphicsView::ShadowSettings shadows;
    StyledGraphicsView::EnvironmentSettings environment;
    StyledGraphicsView::AmbientOcclusionSettings occlusion;
    StyledGraphicsView::EdgeSettings edges;
    std::atomic<bool> available{false};
    std::atomic<bool> shadowsActive{false};
    std::atomic<bool> occlusionActive{false};
    std::atomic<bool> cadAvailable{false};
    std::atomic<bool> edgesActive{false};

    CadSettings cad;
    RenderQualitySettings quality;
    RenderStatistics statistics;
    std::uint64_t edgeRevision = 0;
    virtual void renderShadows(const std::function<void()> &draw) = 0;
    virtual void renderOcclusion(const std::function<void()> &draw) = 0;
    virtual void renderEdges(const std::function<void()> &draw) = 0;
};
std::unique_ptr<SceneStyleRenderer> makeSceneStyleRenderer(GraphicsView *view);
} // namespace detail
} // namespace ptgl
#endif
