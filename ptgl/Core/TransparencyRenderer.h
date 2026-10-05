#ifndef PTGL_CORE_TRANSPARENCYRENDERER_H_
#define PTGL_CORE_TRANSPARENCYRENDERER_H_
#include <functional>
#include "SceneRenderSupport.h"
#include "VertexBufferObject.h"
namespace ptgl
{
namespace detail
{
// Weighted blended OIT (McGuire/Bavoil, JCGT 2013). Two passes avoid requiring
// independent per-target blending. Intersecting meshes need no triangle sort.
const char *transparencyShaderFunctions();
class TransparencyRenderer
{
  public:
    void initialize();
    void release();
    bool render(int width, int height, const std::function<void()> &opaqueDepth,
                const std::function<void(int)> &transparent);

  private:
    bool supported_ = false;
    SceneRenderTarget accumulation_, revealage_;
    ShaderProgramPtr composite_;
    VertexBufferObject quad_;
};
} // namespace detail
} // namespace ptgl
#endif
