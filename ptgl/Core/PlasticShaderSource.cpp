#include "PlasticShaderSource.h"

namespace ptgl {

const std::string PlasticShaderSource::VertexShaderSource = R"GLSL(
#ifdef GL_ES
precision highp float;
#define PTGL_MEDIUMP mediump
#else
#define PTGL_MEDIUMP
#endif
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat3 normalMatrix;
uniform PTGL_MEDIUMP float pointSize;
attribute vec3 position;
attribute vec3 normal;
varying PTGL_MEDIUMP vec3 vPosition;
varying PTGL_MEDIUMP vec3 vNormal;
varying PTGL_MEDIUMP float vSkyWeight;

void main() {
    vec4 viewPosition = viewMatrix * modelMatrix * vec4(position, 1.0);
    vec3 worldNormal = normalMatrix * normal;
    vPosition = viewPosition.xyz;
    vNormal = mat3(viewMatrix) * worldNormal;
    // Guard absent normals, which are normal for unlit lines and points.
    vSkyWeight = 0.5 + 0.5 * worldNormal.z / max(length(worldNormal), 0.0001);
    // Match legacy transform precision/order, including unlit line endpoints.
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);
    gl_PointSize = pointSize;
}
)GLSL";

// GGX distribution, correlated Smith visibility and Schlick Fresnel.
// Lighting is evaluated in linear RGB; the default framebuffer receives sRGB.
const std::string PlasticShaderSource::FragmentShaderSource = R"GLSL(
#ifdef GL_ES
precision mediump float;
#endif
uniform vec4 color;
uniform float lightEffectRate;
uniform float pointSize;
uniform float materialRoughness;
uniform float materialReflectance;
uniform float orthographicCamera;
uniform vec3 keyDirection;
uniform vec3 keyColor;
uniform vec3 fillDirection;
uniform vec3 fillColor;
uniform vec3 skyColor;
uniform vec3 groundColor;
uniform float exposure;
varying vec3 vPosition;
varying vec3 vNormal;
varying float vSkyWeight;
const float PI = 3.14159265;

vec3 toLinear(vec3 c) {
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)),
               step(vec3(0.04045), c));
}

vec3 toSRGB(vec3 c) {
    return mix(12.92 * c, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055,
               step(vec3(0.0031308), c));
}

vec3 illuminate(vec3 n, vec3 v, vec3 l, vec3 base, vec3 radiance) {
    float nl = max(dot(n, l), 0.0);
    float nv = max(dot(n, v), 0.0001);
    vec3 sum = v + l;
    vec3 h = sum / max(length(sum), 0.0001);
    float nh = max(dot(n, h), 0.0);
    float vh = max(dot(v, h), 0.0);
    float alpha = materialRoughness * materialRoughness;
    float alpha2 = alpha * alpha;
    // The cross product avoids cancellation near the highlight on mediump GPUs.
    vec3 crossNH = cross(n, h);
    float denominator = dot(crossNH, crossNH) + nh * nh * alpha2;
    float ratio = alpha / max(denominator, 0.0001);
    float distribution = ratio * ratio / PI;
    float visibilityDenominator = nl * sqrt(nv * nv * (1.0 - alpha2) + alpha2)
                                + nv * sqrt(nl * nl * (1.0 - alpha2) + alpha2);
    float visibility = 0.5 / max(visibilityDenominator, 0.0001);
    float fresnel = materialReflectance
                  + (1.0 - materialReflectance) * pow(1.0 - vh, 5.0);
    vec3 diffuse = (1.0 - fresnel) * base / PI;
    // Apply the cosine before multiplying by D to keep grazing highlights
    // within mediump range. Cap radiance before exposure/tone mapping as well.
    float specular = min(distribution * (visibility * nl) * fresnel, 1024.0);
    return min((diffuse * nl + vec3(specular)) * radiance, vec3(16384.0));
}

void main() {
    if (pointSize > 1.0) {
        vec2 p = gl_PointCoord * 2.0 - 1.0;
        if (dot(p, p) > 1.0) discard;
    }
    // Keep axes, lines, points and explicitly unlit colors unchanged.
    if (lightEffectRate <= 0.0 || dot(vNormal, vNormal) < 0.0001) {
        gl_FragColor = color;
        return;
    }
    vec3 n = normalize(vNormal);
    vec3 v = -vPosition / max(length(vPosition), 0.0001);
    v = mix(v, vec3(0.0, 0.0, 1.0), orthographicCamera);
    vec3 base = toLinear(clamp(color.rgb, 0.0, 1.0));
    vec3 ambient = mix(groundColor, skyColor, clamp(vSkyWeight, 0.0, 1.0));
    vec3 lit = base * ambient * (1.0 - materialReflectance);
    lit += illuminate(n, v, keyDirection, base, keyColor);
    lit += illuminate(n, v, fillDirection, base, fillColor);
    // Equivalent to (lit * exposure)/(1 + lit * exposure), without overflow.
    vec3 mapped = lit / (vec3(1.0 / max(exposure, 0.0001)) + lit);
    if (exposure <= 0.0) mapped = vec3(0.0);
    vec3 displayColor = toSRGB(mapped);
    gl_FragColor = vec4(mix(color.rgb, displayColor, clamp(lightEffectRate, 0.0, 1.0)), color.a);
}
)GLSL";

} // namespace ptgl
