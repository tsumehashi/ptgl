#include "CadEdgeRenderer.h"
#include <cmath>
#include "TransparencyRenderer.h"
#include "PlasticShaderSource.h"

namespace ptgl
{
namespace detail
{
namespace
{
const char *meshVertex = R"GLSL(
#ifdef GL_ES
precision highp float;
#define MEDIUMP mediump
#else
#define MEDIUMP
#endif
attribute vec3 position, normal;
uniform mat4 modelMatrix, viewMatrix, projectionMatrix;
uniform mat3 normalMatrix;
uniform MEDIUMP float pointSize;
varying MEDIUMP vec3 viewNormal;
void main() {
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);
    gl_PointSize = pointSize;
    viewNormal = mat3(viewMatrix) * normalMatrix * normal;
}
)GLSL";
}

void CadEdgeRenderer::initialize()
{
    surface_ = sceneProgram(PlasticShaderSource::VertexShaderSource, std::string(R"GLSL(
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
)GLSL") + transparencyShaderFunctions() + PlasticShaderSource::ShadowFragmentShaderSource + R"GLSL(
void main() {
    if (lightEffectRate <= 0.0 || dot(vNormal, vNormal) < 0.000001) {
        ptglOutput(color); return;
    }
    vec3 n = normalize(vNormal);
    if (!gl_FrontFacing) n = -n;
    // A brighter ambient base keeps faces readable even in shadow. The key
    // direction is shared with the world-space shadow map; fill stays in view space.
    float visible = keyVisibility(n);
    float diffuse = 0.70 + 0.36 * max(dot(n, keyDirection), 0.0) * visible
                         + 0.10 * max(dot(n, normalize(vec3(0.8, -0.2, 0.4))), 0.0);
    vec3 halfVector = keyDirection + vec3(0,0,1);
    halfVector /= max(length(halfVector), 0.0001);
    float highlight = 0.06 * pow(max(dot(n, halfVector), 0.0), 32.0) * visible;
    ptglOutput(vec4(mix(color.rgb, color.rgb * diffuse + highlight,
                       clamp(lightEffectRate, 0.0, 1.0)), color.a));
}
)GLSL");
    if (!glGenFramebuffers || !glBindFramebuffer || !glGenRenderbuffers)
        return;
    SceneGLState state;
    if (state.features.es && glGetShaderPrecisionFormat) {
        GLint range[2], precision = 0;
        glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range, &precision);
        if (!precision)
            return;
    }
    capture_ = sceneProgram(meshVertex, R"GLSL(
#ifdef GL_ES
precision highp float;
#define MEDIUMP mediump
#else
#define MEDIUMP
#endif
varying MEDIUMP vec3 viewNormal;
uniform MEDIUMP vec4 color;
uniform MEDIUMP float lightEffectRate;
uniform float captureDepth;
void main() {
    // Leave unlit helpers and fully transparent geometry out of edge detection.
    if (lightEffectRate <= 0.0 || color.a <= 0.001 || dot(viewNormal, viewNormal) < 0.000001)
        discard;
    if (captureDepth > 0.5) {
        vec3 enc = fract(min(gl_FragCoord.z, 0.9999999) * vec3(1.0, 255.0, 65025.0));
        enc -= enc.yzz * vec3(1.0/255.0, 1.0/255.0, 0.0);
        gl_FragColor = vec4(enc, 1.0);
    } else {
        vec3 n = normalize(viewNormal);
        if (!gl_FrontFacing) n = -n;
        gl_FragColor = vec4(n * 0.5 + 0.5, 1.0);
    }
}
)GLSL");
    composite_ = sceneProgram(R"GLSL(
attribute vec3 position;
varying vec2 uv;
void main() { gl_Position = vec4(position, 1.0); uv = position.xy * 0.5 + 0.5; }
)GLSL",
                              R"GLSL(
#ifdef GL_ES
precision highp float;
#endif
varying vec2 uv;
uniform sampler2D surfaceDepth, surfaceNormals;
uniform mat4 inverseProjection;
uniform vec2 texel;
uniform vec3 edgeColor;
uniform float edgeWidth, normalThreshold;
float depthAt(vec2 p) {
    return dot(texture2D(surfaceDepth, p).rgb, vec3(1.0, 1.0/255.0, 1.0/65025.0));
}
vec3 positionAt(vec2 p, float depth) {
    vec4 q = inverseProjection * vec4(p * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    return q.xyz / q.w;
}
float edgeAt(vec2 offset, vec4 center, float depth, vec3 p, float pixelSize) {
    vec2 sampleUV = clamp(uv + offset, texel * 0.5, vec2(1.0) - texel * 0.5);
    vec4 other = texture2D(surfaceNormals, sampleUV);
    if (abs(center.a - other.a) > 0.5) return 1.0;
    if (center.a < 0.5) return 0.0;
    vec3 a = normalize(center.rgb * 2.0 - 1.0);
    vec3 b = normalize(other.rgb * 2.0 - 1.0);
    float bend = 1.0 - clamp(dot(a, b), -1.0, 1.0);
    float normalEdge = smoothstep(normalThreshold * 0.8, normalThreshold * 1.2, bend);
    float otherDepth = depthAt(sampleUV);
    vec3 delta = positionAt(sampleUV, otherDepth) - p;
    // Compare against the nearer surface's tangent plane: a sloping flat face
    // must not become an edge just because adjacent pixels have different depth.
    float separation = abs(dot(delta, depth < otherDepth ? a : b));
    float depthEdge = smoothstep(pixelSize * 1.5, pixelSize * 3.0, separation);
    return max(normalEdge, depthEdge);
}
float edgeCoverage(float radius, vec4 center, float depth, vec3 p, float pixelSize) {
    vec2 dx = vec2(texel.x * radius, 0.0), dy = vec2(0.0, texel.y * radius);
    float e = max(max(edgeAt(dx,center,depth,p,pixelSize), edgeAt(-dx,center,depth,p,pixelSize)),
                  max(edgeAt(dy,center,depth,p,pixelSize), edgeAt(-dy,center,depth,p,pixelSize)));
    // Diagonal samples soften staircase corners without outlining triangle edges.
    float diagonal = max(max(edgeAt(dx+dy,center,depth,p,pixelSize), edgeAt(-dx-dy,center,depth,p,pixelSize)),
                         max(edgeAt(dx-dy,center,depth,p,pixelSize), edgeAt(-dx+dy,center,depth,p,pixelSize)));
    return max(e, diagonal * 0.65);
}
void main() {
    vec4 center = texture2D(surfaceNormals, uv);
    float depth = depthAt(uv);
    vec3 p = positionAt(uv, depth);
    float pixelSize = max(0.000001,
        0.5 * (length(positionAt(uv + vec2(texel.x,0), depth) - p) +
               length(positionAt(uv + vec2(0,texel.y), depth) - p)));
    // Nearest-filtered depth/normal samples jump at whole pixels. Interpolate
    // edge coverage, not sample positions, so every fractional width step is
    // visible without interpolating packed depth across unrelated surfaces.
    float narrow = edgeCoverage(1.0, center, depth, p, pixelSize);
    float alpha = narrow * min(edgeWidth * 0.5, 1.0);
    if (edgeWidth > 2.0) {
        float wide = max(narrow, edgeCoverage(2.0, center, depth, p, pixelSize));
        alpha = mix(narrow, wide, edgeWidth * 0.5 - 1.0);
    }
    gl_FragColor = vec4(edgeColor * alpha, alpha);
}
)GLSL");
    if (!capture_ || !composite_)
        return;
    const Vertex vertices[] = {Vertex(-1, -1, 0), Vertex(1, -1, 0), Vertex(1, 1, 0), Vertex(-1, 1, 0)};
    const GLuint indices[] = {0, 1, 2, 0, 2, 3};
    quad_.loadVertices(vertices, 4);
    quad_.loadIndices(indices, 6);
}

void CadEdgeRenderer::release()
{
    depth_.release();
    normals_.release();
    quad_.release();
    surface_.reset();
    capture_.reset();
    composite_.reset();
}

bool CadEdgeRenderer::render(const Eigen::Matrix4d &projection,
                             const PlasticGraphicsView::EdgeSettings &settings,
                             const std::function<void(bool)> &draw)
{
    if (!capture_ || !composite_ || !quad_.isValid())
        return false;
    SceneGLState state;
    if (state.features.es && !glClearDepthf) {
        GLfloat d;
        glGetFloatv(GL_DEPTH_CLEAR_VALUE, &d);
        if (d != 1)
            return false;
    }
    const int w = state.viewport[2], h = state.viewport[3];
    if (!depth_.allocate(w, h) || !normals_.allocate(w, h))
        return false;
    for (bool depth : {true, false}) {
        glBindFramebuffer(GL_FRAMEBUFFER, depth ? depth_.framebuffer : normals_.framebuffer);
        glViewport(0, 0, w, h);
        state.rasterDefaults();
        // Alpha is the coverage mask. Transparent items are nearest surfaces
        // here, independent of their color-pass opacity and shadow-casting flag.
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        draw(depth);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, state.drawFbo);
    glViewport(state.viewport[0], state.viewport[1], w, h);
    state.rasterDefaults();
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    composite_->bind();
    for (int unit = 0; unit < 2; ++unit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        if (state.features.samplers)
            glBindSampler(unit, 0);
        glBindTexture(GL_TEXTURE_2D, unit == 0 ? depth_.texture : normals_.texture);
    }
    glUniform1i(composite_->uniform("surfaceDepth"), 0);
    glUniform1i(composite_->uniform("surfaceNormals"), 1);
    composite_->setParameter("inverseProjection", Eigen::Matrix4d(projection.inverse()));
    composite_->setParameter("texel", 1.0 / w, 1.0 / h);
    composite_->setParameter("edgeColor", settings.color[0], settings.color[1], settings.color[2]);
    composite_->setParameter("edgeWidth", settings.width);
    composite_->setParameter("normalThreshold",
                             1.0 - std::cos(settings.normalAngle * 3.141592653589793 / 180.0));
    Renderer3D::drawVertexBufferObject(quad_, composite_->attribute("position"), GL_TRIANGLES);
    composite_->unbind();
    return true;
}
} // namespace detail
} // namespace ptgl
