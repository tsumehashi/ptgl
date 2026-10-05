#include "PlasticAmbientOcclusion.h"
namespace ptgl
{
namespace detail
{
void PlasticAmbientOcclusion::initialize()
{
    if (!glGenFramebuffers || !glBindFramebuffer || !glGenRenderbuffers)
        return;
    SceneGLState state;
    samplers_ = state.features.samplers;
    glActiveTexture(GL_TEXTURE6);
    glGenTextures(1, &fallback_);
    glBindTexture(GL_TEXTURE_2D, fallback_);
    const unsigned char white[] = {255, 255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if (state.features.es && glGetShaderPrecisionFormat) {
        GLint range[2], precision = 0;
        glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range, &precision);
        if (!precision)
            return;
    }
    program_ = sceneProgram(R"GLSL(
        #ifdef GL_ES
        precision highp float;
        #endif
        attribute vec3 position;
        uniform mat4 modelMatrix,viewMatrix,projectionMatrix;
        void main(){gl_Position=projectionMatrix*viewMatrix*modelMatrix*vec4(position,1.0);}
    )GLSL",
                            R"GLSL(
        #ifdef GL_ES
        precision highp float;
        #endif
        uniform float lightEffectRate;
        uniform vec4 color;
        void main(){
            if(lightEffectRate<=0.0||color.a<0.999)discard;
            vec3 enc=fract(min(gl_FragCoord.z,0.9999999)*vec3(1.0,255.0,65025.0));
            enc-=enc.yzz*vec3(1.0/255.0,1.0/255.0,0.0);
            gl_FragColor=vec4(enc,1.0);
        }
    )GLSL");
}
void PlasticAmbientOcclusion::release()
{
    unbind();
    target_.release();
    program_.reset();
    if (fallback_)
        glDeleteTextures(1, &fallback_);
    fallback_ = 0;
}
bool PlasticAmbientOcclusion::render(int width, int height, const std::function<void()> &draw)
{
    if (!program_)
        return false;
    SceneGLState state;
    if (state.features.es && !glClearDepthf) {
        GLfloat d;
        glGetFloatv(GL_DEPTH_CLEAR_VALUE, &d);
        if (d != 1)
            return false;
    }
    if (!target_.allocate(width, height))
        return false;
    glBindFramebuffer(GL_FRAMEBUFFER, target_.framebuffer);
    glViewport(0, 0, width, height);
    state.rasterDefaults();
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw();
    return true;
}
void PlasticAmbientOcclusion::bind(const ShaderProgramPtr &program, bool enabled)
{
    GLint active;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glActiveTexture(GL_TEXTURE6);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture_);
    if (samplers_) {
        glGetIntegerv(GL_SAMPLER_BINDING, &sampler_);
        glBindSampler(6, 0);
    }
    glBindTexture(GL_TEXTURE_2D, enabled ? target_.texture : fallback_);
    glActiveTexture(active);
    bound_ = true;
    glUniform1i(program->uniform("occlusionDepth"), 6);
    program->setParameter("occlusionEnabled", enabled ? 1.0 : 0.0);
}
void PlasticAmbientOcclusion::unbind()
{
    if (!bound_)
        return;
    GLint active;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, texture_);
    if (samplers_)
        glBindSampler(6, sampler_);
    glActiveTexture(active);
    bound_ = false;
}
} // namespace detail
} // namespace ptgl
