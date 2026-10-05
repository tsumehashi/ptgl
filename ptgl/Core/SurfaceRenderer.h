#ifndef PTGL_CORE_SURFACERENDERER_H_
#define PTGL_CORE_SURFACERENDERER_H_
#include "PlasticGraphicsView.h"
namespace ptgl
{
namespace detail
{
// No scene traversal, input, framebuffers or style selection in surface shaders.
class SurfaceRenderer
{
  public:
    virtual ~SurfaceRenderer() = default;
    virtual void initialize() = 0;
    void release() { program_.reset(); }
    ShaderProgramPtr program() const { return program_; }
    virtual void bind(const Camera &, const PlasticLighting &, const CadSettings &,
                      const PlasticGraphicsView::EnvironmentSettings &,
                      const PlasticGraphicsView::AmbientOcclusionSettings &) = 0;

  protected:
    ShaderProgramPtr program_;
};
class PlasticSurfaceRenderer final : public SurfaceRenderer
{
  public:
    void initialize() override;
    void bind(const Camera &, const PlasticLighting &, const CadSettings &,
              const PlasticGraphicsView::EnvironmentSettings &,
              const PlasticGraphicsView::AmbientOcclusionSettings &) override;
};
class CadSurfaceRenderer final : public SurfaceRenderer
{
  public:
    void initialize() override;
    void bind(const Camera &, const PlasticLighting &, const CadSettings &,
              const PlasticGraphicsView::EnvironmentSettings &,
              const PlasticGraphicsView::AmbientOcclusionSettings &) override;
};
} // namespace detail
} // namespace ptgl
#endif
