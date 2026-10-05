#ifndef PTGL_CORE_PLASTICAMBIENTOCCLUSION_H_
#define PTGL_CORE_PLASTICAMBIENTOCCLUSION_H_
#include <functional>
#include "SceneRenderSupport.h"
namespace ptgl
{
namespace detail
{
class PlasticAmbientOcclusion
{
  public:
    void initialize();
    void release();
    bool render(int width, int height, const std::function<void()> &draw);
    void bind(const ShaderProgramPtr &program, bool enabled);
    void unbind();
    ShaderProgramPtr program() const { return program_; }

  private:
    SceneRenderTarget target_;
    ShaderProgramPtr program_;
    GLint texture_ = 0, sampler_ = 0;
    GLuint fallback_ = 0;
    bool bound_ = false, samplers_ = false;
};
} // namespace detail
} // namespace ptgl
#endif
