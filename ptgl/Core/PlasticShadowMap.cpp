#include "PlasticShadowMap.h"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include "ptgl/Util/MathUtil.h"

namespace ptgl {
namespace detail {
namespace {

// RGBA8 plus a depth renderbuffer avoids requiring depth-texture extensions.
const char* vertexSource = R"GLSL(
#ifdef GL_ES
precision highp float;
#endif
attribute vec3 position;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
void main() {
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);
    gl_PointSize = 1.0;
}
)GLSL";

const char* fragmentSource = R"GLSL(
#ifdef GL_ES
precision highp float;
#endif
uniform vec4 color;
uniform float lightEffectRate;
void main() {
    // Lines, helpers and transparent surfaces do not cast opaque shadows.
    if (lightEffectRate <= 0.0 || color.a < 0.999) discard;
    float depth = clamp(gl_FragCoord.z, 0.0, 1.0) * 255.0;
    gl_FragColor = vec4(floor(depth) / 255.0, fract(depth), 0.0, 1.0);
}
)GLSL";

void setEnabled(GLenum cap, GLboolean on)
{
    if (on) glEnable(cap); else glDisable(cap);
}

// Preserve state touched by the offscreen pass, including callers' nonzero
// framebuffers and viewport. Only the draw framebuffer is changed on GL 3+.
class PassState {
public:
    PassState(GLenum target, GLenum binding, bool es) : target_(target), es_(es)
    {
        glGetIntegerv(binding, &framebuffer_);
        glGetIntegerv(GL_VIEWPORT, viewport_);
        glGetIntegerv(GL_CURRENT_PROGRAM, &program_);
        glGetIntegerv(GL_DEPTH_FUNC, &depthFunction_);
        glGetIntegerv(GL_CULL_FACE_MODE, &cullMode_);
        glGetBooleanv(GL_COLOR_WRITEMASK, colorMask_);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask_);
        glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor_);
        glGetFloatv(GL_DEPTH_CLEAR_VALUE, &clearDepth_);
        glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &offsetFactor_);
        glGetFloatv(GL_POLYGON_OFFSET_UNITS, &offsetUnits_);
        for (int i = 0; i < 6; ++i) enabled_[i] = glIsEnabled(caps_[i]);
    }
    ~PassState()
    {
        glBindFramebuffer(target_, framebuffer_);
        glViewport(viewport_[0], viewport_[1], viewport_[2], viewport_[3]);
        glUseProgram(program_);
        glDepthFunc(depthFunction_);
        glCullFace(cullMode_);
        glColorMask(colorMask_[0], colorMask_[1], colorMask_[2], colorMask_[3]);
        glDepthMask(depthMask_);
        glClearColor(clearColor_[0], clearColor_[1], clearColor_[2], clearColor_[3]);
        if (!es_) glClearDepth(clearDepth_);
        else if (glClearDepthf) glClearDepthf(clearDepth_);
        glPolygonOffset(offsetFactor_, offsetUnits_);
        for (int i = 0; i < 6; ++i) setEnabled(caps_[i], enabled_[i]);
    }
private:
    GLenum target_;
    bool es_;
    GLint framebuffer_, viewport_[4], program_, depthFunction_, cullMode_;
    GLboolean colorMask_[4], depthMask_, enabled_[6];
    GLfloat clearColor_[4], clearDepth_, offsetFactor_, offsetUnits_;
    const GLenum caps_[6] = {GL_BLEND, GL_DITHER, GL_SCISSOR_TEST, GL_DEPTH_TEST, GL_CULL_FACE, GL_POLYGON_OFFSET_FILL};
};

void configureTexture()
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

} // namespace

void PlasticShadowMap::initialize()
{
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    es_ = version && std::string(version).compare(0, 9, "OpenGL ES") == 0;
    int major = 0, minor = 0;
    if (version) std::sscanf(version, es_ ? "OpenGL ES %d.%d" : "%d.%d", &major, &minor);
    samplerObjects_ = glBindSampler && (es_ ? major >= 3 :
        major > 3 || (major == 3 && minor >= 3) || GLEW_ARB_sampler_objects);
    if (major >= 3) {
        framebufferTarget_ = GL_DRAW_FRAMEBUFFER;
        framebufferBinding_ = GL_DRAW_FRAMEBUFFER_BINDING;
    }

    GLint oldTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
    glGenTextures(1, &whiteTexture_);
    glBindTexture(GL_TEXTURE_2D, whiteTexture_);
    const GLubyte white[4] = {255, 0, 0, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    configureTexture();
    glBindTexture(GL_TEXTURE_2D, oldTexture);

    if (!glGenFramebuffers || !glBindFramebuffer || !glGenRenderbuffers) {
        std::cerr << "PlasticGraphicsView: framebuffer objects unavailable; keeping plastic lighting without shadows.\n";
        return;
    }
    // Some desktop GLEW builds omit this GLES entry point. In that case the
    // highp shader's compilation below performs the capability check.
    if (es_ && glGetShaderPrecisionFormat) {
        GLint range[2], precision = 0;
        glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range, &precision);
        if (precision == 0) {
            std::cerr << "PlasticGraphicsView: shadows require fragment highp; keeping plastic lighting without shadows.\n";
            return;
        }
    }
    auto vertex = Shader::loadFromSource(Shader::VertexShader, vertexSource);
    auto fragment = Shader::loadFromSource(Shader::FragmentShader, fragmentSource);
    auto program = std::make_shared<ShaderProgram>();
    if (vertex && fragment && program->linkShaders({vertex, fragment})) program_ = program;
    else std::cerr << "PlasticGraphicsView: shadow shader unavailable; keeping plastic lighting without shadows.\n";
}

void PlasticShadowMap::releaseMap()
{
    if (framebuffer_) glDeleteFramebuffers(1, &framebuffer_);
    if (texture_) glDeleteTextures(1, &texture_);
    if (depth_) glDeleteRenderbuffers(1, &depth_);
    framebuffer_ = texture_ = depth_ = 0;
    size_ = 0;
}

void PlasticShadowMap::release()
{
    unbind();
    releaseMap();
    if (whiteTexture_) glDeleteTextures(1, &whiteTexture_);
    whiteTexture_ = 0;
    program_.reset();
    failedSize_ = 0;
}

bool PlasticShadowMap::allocate(int size)
{
    if (size_ == size) return true;
    if (failedSize_ == size) return false; // Avoid repeating an unsupported allocation every frame.
    GLint oldFramebuffer, oldRenderbuffer, oldTexture;
    glGetIntegerv(framebufferBinding_, &oldFramebuffer);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &oldRenderbuffer);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
    releaseMap();
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    configureTexture();
    glGenRenderbuffers(1, &depth_);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, size, size);
    glGenFramebuffers(1, &framebuffer_);
    glBindFramebuffer(framebufferTarget_, framebuffer_);
    glFramebufferTexture2D(framebufferTarget_, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_, 0);
    glFramebufferRenderbuffer(framebufferTarget_, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_);
    bool complete = glCheckFramebufferStatus(framebufferTarget_) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(framebufferTarget_, oldFramebuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, oldRenderbuffer);
    glBindTexture(GL_TEXTURE_2D, oldTexture);
    if (!complete) {
        releaseMap();
        failedSize_ = size;
        std::cerr << "PlasticGraphicsView: shadow framebuffer unavailable; keeping plastic lighting without shadows.\n";
        return false;
    }
    size_ = size;
    failedSize_ = 0;
    return true;
}

bool PlasticShadowMap::prepare(const PlasticGraphicsView::ShadowSettings& settings, const PlasticLighting& lighting)
{
    if (!program_) return false;
    // Desktop GLEW can leave glClearDepthf unresolved in a GLES context.
    // The normal clear depth is already 1; never call the desktop double
    // entry point on GLES. Other clear values require a working GLES loader.
    if (es_ && !glClearDepthf) {
        GLfloat clearDepth;
        glGetFloatv(GL_DEPTH_CLEAR_VALUE, &clearDepth);
        if (clearDepth != 1.0f) return false;
    }
    GLint textureLimit = 0, renderbufferLimit = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &textureLimit);
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &renderbufferLimit);
    int size = settings.resolution;
    while (size > std::min(textureLimit, renderbufferLimit) && size > 1) size /= 2;
    if (!allocate(size)) return false;
    settings_ = settings;
    Eigen::Vector3d direction(lighting.keyDirection[0], lighting.keyDirection[1], lighting.keyDirection[2]);
    direction /= direction.stableNorm();
    Eigen::Vector3d center(settings.center[0], settings.center[1], settings.center[2]);
    Eigen::Vector3d eye = center + direction * (2.0 * settings.halfExtent);
    Eigen::Vector3d up = std::abs(direction.z()) < 0.95 ? Eigen::Vector3d::UnitZ() : Eigen::Vector3d::UnitY();
    view_ = ptgl::lookAt(eye, center, up);
    double extent = settings.halfExtent;
    projection_ = ptgl::ortho(-extent, extent, -extent, extent, extent * 0.01, extent * 4.0);
    return true;
}

void PlasticShadowMap::render(const std::function<void()>& draw)
{
    PassState restore(framebufferTarget_, framebufferBinding_, es_);
    glBindFramebuffer(framebufferTarget_, framebuffer_);
    glViewport(0, 0, size_, size_);
    glDisable(GL_BLEND);
    glDisable(GL_DITHER);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glClearColor(1, 0, 0, 1); // Encoded far depth; empty pixels never occlude.
    if (!es_) glClearDepth(1);
    else if (glClearDepthf) glClearDepthf(1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw();
}

void PlasticShadowMap::bind(const ShaderProgramPtr& program, bool enabled)
{
    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
    glActiveTexture(GL_TEXTURE7); // Reserved only for the plastic color pass.
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedTexture_);
    if (samplerObjects_) {
        glGetIntegerv(GL_SAMPLER_BINDING, &savedSampler_);
        glBindSampler(7, 0); // A caller's filtering/comparison sampler must not affect packed depth.
    }
    glBindTexture(GL_TEXTURE_2D, enabled ? texture_ : whiteTexture_);
    glActiveTexture(activeTexture);
    bound_ = true;
    glUniform1i(program->uniform("shadowMap"), 7);
    program->setParameter("shadowEnabled", enabled ? 1.0 : 0.0);
    program->setParameter("shadowMatrix", Eigen::Matrix4d(projection_ * view_));
    program->setParameter("shadowTexelSize", size_ > 0 ? 1.0 / size_ : 1.0);
    program->setParameter("shadowSoftness", settings_.softness);
    program->setParameter("shadowBias", settings_.bias);
    program->setParameter("shadowNormalBias", enabled ? settings_.normalBias : 0.0);
    program->setParameter("shadowStrength", settings_.strength);
}

void PlasticShadowMap::unbind()
{
    if (!bound_) return;
    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, savedTexture_);
    if (samplerObjects_) glBindSampler(7, savedSampler_);
    glActiveTexture(activeTexture);
    bound_ = false;
}

} // namespace detail
} // namespace ptgl
