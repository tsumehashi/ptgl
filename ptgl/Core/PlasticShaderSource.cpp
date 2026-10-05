#include "PlasticShaderSource.h"

namespace ptgl {

const std::string PlasticShaderSource::VertexShaderSource = R"GLSL(
#ifdef GL_ES
precision highp float;
#define PTGL_MEDIUMP mediump
#ifdef GL_FRAGMENT_PRECISION_HIGH
#define PTGL_SHADOW_PRECISION highp
#else
#define PTGL_SHADOW_PRECISION mediump
#endif
#else
#define PTGL_MEDIUMP
#define PTGL_SHADOW_PRECISION
#endif
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat3 normalMatrix;
uniform mat4 shadowMatrix;
uniform float shadowNormalBias;
uniform PTGL_MEDIUMP float pointSize;
attribute vec3 position;
attribute vec3 normal;
varying PTGL_MEDIUMP vec3 vPosition;
varying PTGL_MEDIUMP vec3 vNormal;
varying PTGL_MEDIUMP float vSkyWeight;
varying PTGL_SHADOW_PRECISION vec4 vShadowPosition;

void main() {
    vec4 viewPosition = viewMatrix * modelMatrix * vec4(position, 1.0);
    vec3 worldNormal = normalMatrix * normal;
    vec3 offset = worldNormal / max(length(worldNormal), 0.0001) * shadowNormalBias;
    vShadowPosition = shadowMatrix * (modelMatrix * vec4(position, 1.0) + vec4(offset, 0.0));
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
#ifdef GL_FRAGMENT_PRECISION_HIGH
#define PTGL_SHADOW_PRECISION highp
#else
#define PTGL_SHADOW_PRECISION mediump
#endif
#else
#define PTGL_SHADOW_PRECISION
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
uniform PTGL_SHADOW_PRECISION sampler2D shadowMap;
uniform float shadowEnabled;
uniform PTGL_SHADOW_PRECISION float shadowTexelSize;
uniform PTGL_SHADOW_PRECISION float shadowBias;
uniform float shadowSoftness;
uniform float shadowStrength;
varying vec3 vPosition;
varying vec3 vNormal;
varying float vSkyWeight;
varying PTGL_SHADOW_PRECISION vec4 vShadowPosition;
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

float keyVisibility(vec3 n) {
    if (shadowEnabled < 0.5) return 1.0;
    PTGL_SHADOW_PRECISION vec3 p = vShadowPosition.xyz / vShadowPosition.w * 0.5 + 0.5;
    if (p.x <= 0.0 || p.y <= 0.0 || p.z <= 0.0 || p.x >= 1.0 || p.y >= 1.0 || p.z >= 1.0) return 1.0;
    PTGL_SHADOW_PRECISION float bias = shadowBias * (1.0 + 2.0 * (1.0 - max(dot(n, keyDirection), 0.0)));
    float visibility = 0.0;
    // Tent-weighted 5x5 PCF compares depths before filtering (never blur encoded depth).
    for (int y = -2; y <= 2; ++y) {
        for (int x = -2; x <= 2; ++x) {
            PTGL_SHADOW_PRECISION vec2 uv = p.xy + vec2(float(x), float(y)) * shadowTexelSize * shadowSoftness * 0.5;
            float weight = (3.0 - abs(float(x))) * (3.0 - abs(float(y)));
            PTGL_SHADOW_PRECISION vec2 encoded = texture2D(shadowMap, uv).rg;
            PTGL_SHADOW_PRECISION float depth = encoded.r + encoded.g / 255.0;
            float lit = step(p.z - bias, depth);
            float inside = step(0.0, uv.x) * step(0.0, uv.y) * step(uv.x, 1.0) * step(uv.y, 1.0);
            lit = mix(1.0, lit, inside);
            visibility += weight * lit;
        }
    }
    // Fade the last few texels of a finite shadow volume, avoiding a hard seam.
    PTGL_SHADOW_PRECISION vec2 edge = min(p.xy, vec2(1.0) - p.xy);
    float fade = smoothstep(0.0, shadowTexelSize * (shadowSoftness + 1.0), min(edge.x, edge.y));
    return mix(1.0, visibility / 81.0, shadowStrength * fade);
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
    lit += keyVisibility(n) * illuminate(n, v, keyDirection, base, keyColor);
    lit += illuminate(n, v, fillDirection, base, fillColor);
    // Equivalent to (lit * exposure)/(1 + lit * exposure), without overflow.
    vec3 mapped = lit / (vec3(1.0 / max(exposure, 0.0001)) + lit);
    if (exposure <= 0.0) mapped = vec3(0.0);
    vec3 displayColor = toSRGB(mapped);
    gl_FragColor = vec4(mix(color.rgb, displayColor, clamp(lightEffectRate, 0.0, 1.0)), color.a);
}
)GLSL";

} // namespace ptgl
