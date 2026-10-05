#include "SurfaceRenderer.h"
#include "SceneRenderSupport.h"
#include "PlasticShaderSource.h"
#include "TransparencyRenderer.h"
namespace ptgl
{
namespace detail
{
namespace
{
constexpr double pi = 3.14159265358979323846;
}
void PlasticSurfaceRenderer::initialize()
{
    program_ =
        sceneProgram(PlasticShaderSource::VertexShaderSource, PlasticShaderSource::FragmentShaderSource);
}
void PlasticSurfaceRenderer::bind(const Camera &camera, const PlasticLighting &lighting, const CadSettings &,
                                  const PlasticGraphicsView::EnvironmentSettings &environment,
                                  const PlasticGraphicsView::AmbientOcclusionSettings &occlusion)
{
    auto program = program_;
    Eigen::Matrix3d viewRotation = camera.modelview().block<3, 3>(0, 0);
    auto setDirection = [&](const char *name, const std::array<double, 3> &direction) {
        Eigen::Vector3d value(direction[0], direction[1], direction[2]);
        value = viewRotation * (value / value.stableNorm());
        program->setParameter(name, value);
    };
    auto setColor = [&](const char *name, const std::array<double, 3> &c) {
        program->setParameter(name, c[0], c[1], c[2]);
    };
    setDirection("keyDirection", lighting.keyDirection);
    setDirection("fillDirection", lighting.fillDirection);
    setColor("keyColor", lighting.keyColor);
    setColor("fillColor", lighting.fillColor);
    setColor("skyColor", lighting.skyColor);
    setColor("groundColor", lighting.groundColor);
    program->setParameter("exposure", lighting.exposure);
    program->setParameter("orthographicCamera", camera.viewMode() == Camera::Ortho ? 1.0 : 0.0);
    program->setParameter("viewToWorld", Eigen::Matrix3d(viewRotation.transpose()));
    program->setParameter("environmentStrength", environment.enabled ? environment.strength : 0.0);
    program->setParameter("environmentRotation", environment.rotation * pi / 180.0);
    program->setParameter("inverseProjection", Eigen::Matrix4d(camera.projection().inverse()));
    program->setParameter("occlusionRadius", occlusion.radius);
    program->setParameter("occlusionStrength", occlusion.strength);
    program->setParameter("occlusionBias", occlusion.bias);
    program->setParameter("projectionScale", camera.projection()(0, 0), camera.projection()(1, 1));
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    program->setParameter("occlusionViewport", double(viewport[0]), double(viewport[1]), double(viewport[2]),
                          double(viewport[3]));
}
void CadSurfaceRenderer::initialize()
{
    program_ = sceneProgram(PlasticShaderSource::VertexShaderSource, std::string(R"GLSL(
#ifdef GL_ES
precision mediump float;
#ifdef GL_FRAGMENT_PRECISION_HIGH
#define PTGL_SHADOW_PRECISION highp
#else
#define PTGL_SHADOW_PRECISION mediump
#endif
#else
#define PTGL_SHADOW_PRECISION
#endif
varying vec3 vNormal;
uniform vec4 color;
uniform float lightEffectRate;
uniform float cadBrightness, cadAmbient, cadKey, cadFill, cadSpecular, cadShininess;
uniform vec3 cadFillDirection;
)GLSL") + transparencyShaderFunctions() + PlasticShaderSource::ShadowFragmentShaderSource +
                                                                         R"GLSL(
void main() {
    if (lightEffectRate <= 0.0 || dot(vNormal, vNormal) < 0.000001) {
        ptglOutput(color); return;
    }
    vec3 n = normalize(vNormal);
    if (!gl_FrontFacing) n = -n;
    // A brighter ambient base keeps faces readable even in shadow. The key
    // direction is shared with the world-space shadow map; fill stays in view space.
    float visible = keyVisibility(n);
    float diffuse = cadAmbient + cadKey * max(dot(n, keyDirection), 0.0) * visible
                               + cadFill * max(dot(n, cadFillDirection), 0.0);
    vec3 halfVector = keyDirection + vec3(0,0,1);
    halfVector /= max(length(halfVector), 0.0001);
    float highlight = cadSpecular * pow(max(dot(n, halfVector), 0.0), cadShininess) * visible;
    ptglOutput(vec4(mix(color.rgb, clamp((color.rgb * diffuse + highlight) * cadBrightness, 0.0, 1.0),
                       clamp(lightEffectRate, 0.0, 1.0)), color.a));
}
)GLSL");
}
void CadSurfaceRenderer::bind(const Camera &camera, const PlasticLighting &lighting, const CadSettings &cad,
                              const PlasticGraphicsView::EnvironmentSettings &,
                              const PlasticGraphicsView::AmbientOcclusionSettings &)
{
    Eigen::Vector3d key(lighting.keyDirection.data());
    key = camera.modelview().block<3, 3>(0, 0) * (key / key.stableNorm());
    program_->setParameter("keyDirection", key);
    Eigen::Vector3d fill(cad.fillDirection.data());
    program_->setParameter("cadFillDirection", Eigen::Vector3d(fill / fill.stableNorm()));
    program_->setParameter("cadBrightness", cad.brightness);
    program_->setParameter("cadAmbient", cad.ambient);
    program_->setParameter("cadKey", cad.key);
    program_->setParameter("cadFill", cad.fill);
    program_->setParameter("cadSpecular", cad.specular);
    program_->setParameter("cadShininess", cad.shininess);
}
} // namespace detail
} // namespace ptgl
