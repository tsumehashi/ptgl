#ifndef PTGL_CORE_SCENERENDERSUPPORT_H_
#define PTGL_CORE_SCENERENDERSUPPORT_H_

#include <cstdio>
#include <string>
#include "ShaderProgram.h"

namespace ptgl
{
namespace detail
{

struct GLFeatures {
    bool es = false, modern = false, samplers = false;
    GLFeatures()
    {
        const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
        es = version && std::string(version).find("OpenGL ES") == 0;
        int major = 0, minor = 0;
        if (version)
            std::sscanf(version, es ? "OpenGL ES %d.%d" : "%d.%d", &major, &minor);
        modern = major >= 3;
        samplers = glBindSampler &&
                   (es ? modern : major > 3 || (major == 3 && minor >= 3) || GLEW_ARB_sampler_objects);
    }
};

// Offscreen helpers own no state outside their call. Resource deletion is
// explicit and occurs in GraphicsView's GL-context finalize event.
class SceneGLState
{
  public:
    GLFeatures features;
    GLint drawFbo = 0, readFbo = 0, viewport[4];
    SceneGLState()
    {
        glGetIntegerv(features.modern ? GL_DRAW_FRAMEBUFFER_BINDING : GL_FRAMEBUFFER_BINDING, &drawFbo);
        if (features.modern)
            glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFbo);
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetIntegerv(GL_CURRENT_PROGRAM, &program_);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &array_);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &element_);
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &renderbuffer_);
        glGetIntegerv(GL_DEPTH_FUNC, &depthFunc_);
        glGetIntegerv(GL_CULL_FACE_MODE, &cullMode_);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask_);
        glGetBooleanv(GL_COLOR_WRITEMASK, colorMask_);
        glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor_);
        glGetFloatv(GL_DEPTH_CLEAR_VALUE, &clearDepth_);
        glGetIntegerv(GL_BLEND_SRC_RGB, &srcRGB_);
        glGetIntegerv(GL_BLEND_DST_RGB, &dstRGB_);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha_);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha_);
        glGetIntegerv(GL_BLEND_EQUATION_RGB, &eqRGB_);
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &eqAlpha_);
        for (int i = 0; i < 6; ++i)
            enabled_[i] = glIsEnabled(caps_[i]);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &active_);
        for (int i = 0; i < 3; ++i) {
            glActiveTexture(GL_TEXTURE0 + units_[i]);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &textures_[i]);
            if (features.samplers)
                glGetIntegerv(GL_SAMPLER_BINDING, &samplers_[i]);
        }
        glActiveTexture(active_);
    }
    ~SceneGLState()
    {
        glBindFramebuffer(features.modern ? GL_DRAW_FRAMEBUFFER : GL_FRAMEBUFFER, drawFbo);
        if (features.modern)
            glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        glUseProgram(program_);
        glBindBuffer(GL_ARRAY_BUFFER, array_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_);
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer_);
        glDepthFunc(depthFunc_);
        glCullFace(cullMode_);
        glDepthMask(depthMask_);
        glColorMask(colorMask_[0], colorMask_[1], colorMask_[2], colorMask_[3]);
        glClearColor(clearColor_[0], clearColor_[1], clearColor_[2], clearColor_[3]);
        if (!features.es)
            glClearDepth(clearDepth_);
        else if (glClearDepthf)
            glClearDepthf(clearDepth_);
        glBlendFuncSeparate(srcRGB_, dstRGB_, srcAlpha_, dstAlpha_);
        glBlendEquationSeparate(eqRGB_, eqAlpha_);
        for (int i = 0; i < 6; ++i) {
            if (enabled_[i])
                glEnable(caps_[i]);
            else
                glDisable(caps_[i]);
        }
        for (int i = 0; i < 3; ++i) {
            glActiveTexture(GL_TEXTURE0 + units_[i]);
            glBindTexture(GL_TEXTURE_2D, textures_[i]);
            if (features.samplers)
                glBindSampler(units_[i], samplers_[i]);
        }
        glActiveTexture(active_);
    }
    void rasterDefaults() const
    {
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_DITHER);
        glDisable(GL_BLEND);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        if (!features.es)
            glClearDepth(1);
        else if (glClearDepthf)
            glClearDepthf(1);
    }

  private:
    GLint program_, array_, element_, renderbuffer_, depthFunc_, cullMode_, active_;
    GLint srcRGB_, dstRGB_, srcAlpha_, dstAlpha_, eqRGB_, eqAlpha_, textures_[3]{}, samplers_[3]{};
    GLfloat clearColor_[4], clearDepth_;
    GLboolean depthMask_, colorMask_[4], enabled_[6];
    const GLenum caps_[6] = {GL_BLEND,        GL_DEPTH_TEST, GL_CULL_FACE,
                             GL_SCISSOR_TEST, GL_DITHER,     GL_POLYGON_OFFSET_FILL};
    const int units_[3] = {0, 1, 6};
};

struct SceneRenderTarget {
    GLuint framebuffer = 0, texture = 0, depth = 0;
    int width = 0, height = 0;
    void release()
    {
        if (framebuffer)
            glDeleteFramebuffers(1, &framebuffer);
        if (texture)
            glDeleteTextures(1, &texture);
        if (depth)
            glDeleteRenderbuffers(1, &depth);
        framebuffer = texture = depth = 0;
        width = height = 0;
    }
    bool allocate(int w, int h, bool floating = false, bool withDepth = true)
    {
        if (w == width && h == height && framebuffer)
            return true;
        if (w <= 0 || h <= 0 || !glGenFramebuffers)
            return false;
        SceneGLState state;
        GLint limit = 0, renderbufferLimit = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &limit);
        glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &renderbufferLimit);
        if (w > limit || h > limit || (withDepth && (w > renderbufferLimit || h > renderbufferLimit)))
            return false;
        release();
        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, floating ? GL_RGBA16F : GL_RGBA, w, h, 0, GL_RGBA,
                     floating ? GL_FLOAT : GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
        if (withDepth) {
            glGenRenderbuffers(1, &depth);
            glBindRenderbuffer(GL_RENDERBUFFER, depth);
            glRenderbufferStorage(GL_RENDERBUFFER,
                                  state.features.modern ? GL_DEPTH_COMPONENT24 : GL_DEPTH_COMPONENT16, w, h);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
        }
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            release();
            return false;
        }
        width = w;
        height = h;
        return true;
    }
};

inline ShaderProgramPtr sceneProgram(const std::string &vertex, const std::string &fragment)
{
    auto v = Shader::loadFromSource(Shader::VertexShader, vertex);
    auto f = Shader::loadFromSource(Shader::FragmentShader, fragment);
    auto program = std::make_shared<ShaderProgram>();
    return v && f && program->linkShaders({v, f}) ? program : nullptr;
}

} // namespace detail
} // namespace ptgl
#endif
