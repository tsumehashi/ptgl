#include "TransparencyRenderer.h"
#include "Renderer3D.h"

namespace ptgl
{
namespace detail
{
const char *transparencyShaderFunctions()
{
    return R"GLSL(
uniform float objectOpacity;
uniform float transparencyPass;
void ptglOutput(vec4 c) {
    c.a = clamp(c.a * objectOpacity, 0.0, 1.0);
    if (transparencyPass > 1.5) { gl_FragColor = vec4(0.0,0.0,0.0,c.a); return; }
    if (transparencyPass > 0.5) {
        float w = clamp(pow(1.0-gl_FragCoord.z*0.9,3.0)*10.0,0.01,10.0);
        gl_FragColor = vec4(c.rgb*c.a,c.a)*w;
        return;
    }
    gl_FragColor=c;
}
)GLSL";
}

void TransparencyRenderer::initialize()
{
    GLFeatures features;
    supported_ = features.modern;
    if (features.es) {
        const char *ext = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
        std::string extensions = ext ? ext : "";
        supported_ = supported_ && extensions.find("GL_EXT_color_buffer_float") != std::string::npos &&
                     extensions.find("GL_EXT_float_blend") != std::string::npos;
    }
    if (!supported_)
        return;
    composite_ = sceneProgram(R"GLSL(
        attribute vec3 position; varying vec2 uv;
        void main(){gl_Position=vec4(position,1.0);uv=position.xy*.5+.5;}
    )GLSL",
                              R"GLSL(
        #ifdef GL_ES
        precision highp float;
        #endif
        varying vec2 uv; uniform sampler2D accumulation; uniform sampler2D revealage;
        void main(){
            vec4 accum=texture2D(accumulation,uv);
            float alpha=1.0-texture2D(revealage,uv).r;
            gl_FragColor=vec4(accum.rgb/max(accum.a,0.00001)*alpha,alpha);
        }
    )GLSL");
    if (!composite_) {
        supported_ = false;
        return;
    }
    const Vertex v[] = {Vertex(-1, -1, 0), Vertex(1, -1, 0), Vertex(1, 1, 0), Vertex(-1, 1, 0)};
    const GLuint i[] = {0, 1, 2, 0, 2, 3};
    quad_.loadVertices(v, 4);
    quad_.loadIndices(i, 6);
}
void TransparencyRenderer::release()
{
    accumulation_.release();
    revealage_.release();
    quad_.release();
    composite_.reset();
    supported_ = false;
}
bool TransparencyRenderer::render(int w, int h, const std::function<void()> &opaqueDepth,
                                  const std::function<void(int)> &transparent)
{
    if (!supported_)
        return false;
    SceneGLState state;
    if (!accumulation_.allocate(w, h, true) || !revealage_.allocate(w, h, false, false)) {
        supported_ = false;
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, revealage_.framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, accumulation_.depth);
    state.rasterDefaults();
    glViewport(0, 0, w, h);
    glBindFramebuffer(GL_FRAMEBUFFER, accumulation_.framebuffer);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    opaqueDepth();
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);
    transparent(1);
    glBindFramebuffer(GL_FRAMEBUFFER, revealage_.framebuffer);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
    transparent(2);
    glBindFramebuffer(GL_FRAMEBUFFER, state.drawFbo);
    glViewport(state.viewport[0], state.viewport[1], state.viewport[2], state.viewport[3]);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    composite_->bind();
    for (int unit = 0; unit < 2; ++unit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        if (state.features.samplers)
            glBindSampler(unit, 0);
        glBindTexture(GL_TEXTURE_2D, unit == 0 ? accumulation_.texture : revealage_.texture);
    }
    glUniform1i(composite_->uniform("accumulation"), 0);
    glUniform1i(composite_->uniform("revealage"), 1);
    Renderer3D::drawVertexBufferObject(quad_, composite_->attribute("position"), GL_TRIANGLES);
    composite_->unbind();
    return true;
}
} // namespace detail
} // namespace ptgl
