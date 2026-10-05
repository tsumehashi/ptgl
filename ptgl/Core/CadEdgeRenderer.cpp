#include "CadEdgeRenderer.h"
#include <cmath>
#include <chrono>
#include <limits>
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
    float radius = max(1.0, edgeWidth * 0.5);
    float low = floor(radius), high = ceil(radius);
    float alpha = mix(edgeCoverage(low,center,depth,p,pixelSize),
                      edgeCoverage(high,center,depth,p,pixelSize), radius-low);
    alpha *= min(edgeWidth * 0.5, 1.0);
    gl_FragColor = vec4(edgeColor * alpha, alpha);
}
)GLSL");
    resolve_ = sceneProgram(R"GLSL(
attribute vec3 position;
varying vec2 uv;
void main() { gl_Position=vec4(position,1.0); uv=position.xy*0.5+0.5; }
)GLSL",
                            R"GLSL(
#ifdef GL_ES
precision highp float;
#endif
varying vec2 uv;
uniform sampler2D edgeMask;
uniform vec2 texel;
uniform float scale;
void main() {
    vec4 sum=vec4(0.0);
    for(int y=0;y<3;++y) for(int x=0;x<3;++x) {
        if(float(x)<scale && float(y)<scale)
            sum+=texture2D(edgeMask,uv+(vec2(float(x),float(y))-(scale-1.0)*0.5)*texel);
    }
    gl_FragColor=sum/(scale*scale);
}
)GLSL");
    feature_ = sceneProgram(R"GLSL(
#ifdef GL_ES
precision highp float;
#endif
attribute vec3 position, normal;
attribute vec2 corner;
uniform mat4 mvp;
uniform vec2 viewport;
uniform float width;
varying float side;
void main() {
    vec4 a=mvp*vec4(position,1.0), b=mvp*vec4(normal,1.0);
    float da=a.z+a.w, db=b.z+b.w;
    if(da<=0.0 && db<=0.0) { gl_Position=vec4(2,2,2,1); side=0.0; return; }
    if(da<0.0) a=mix(a,b,da/(da-db));
    if(db<0.0) b=mix(b,a,db/(db-max(da,0.0)));
    a/=max(a.w,0.000001); b/=max(b.w,0.000001);
    vec2 d=(b.xy-a.xy)*viewport;
    vec2 n=vec2(-d.y,d.x)/max(length(d),0.000001);
    gl_Position=mix(a,b,corner.x);
    gl_Position.xy+=n*corner.y*(width+1.0)/viewport;
    gl_Position.z-=0.00001;
    side=corner.y;
}
)GLSL",
                            R"GLSL(
#ifdef GL_ES
precision highp float;
#endif
varying float side;
uniform vec3 edgeColor;
uniform float width;
void main() {
    float alpha=clamp((width+1.0)*0.5*(1.0-abs(side)),0.0,1.0);
    gl_FragColor=vec4(edgeColor*alpha,alpha);
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
    capture_.reset();
    composite_.reset();
    resolve_.reset();
    feature_.reset();
    mask_.release();
    featureBuffer_.release();
    features_.clear();
    cached_ = false;
    requestedWidth_ = requestedHeight_ = requestedScale_ = actualScale_ = 0;
    timer_.release();
}

void CadEdgeRenderer::addFeatures(std::shared_ptr<const VertexList> vertices, const Eigen::Matrix4d &model)
{
    if (vertices && !vertices->empty())
        features_.push_back({std::move(vertices), model});
}
bool CadEdgeRenderer::render(const Eigen::Matrix4d &projection, const Eigen::Matrix4d &view,
                             const PlasticGraphicsView::EdgeSettings &settings,
                             const RenderQualitySettings &quality, std::uint64_t revision,
                             RenderStatistics &stats, const std::function<void(bool)> &draw)
{
    if (!capture_ || !composite_ || !resolve_ || !feature_ || !quad_.isValid())
        return false;
    SceneGLState state;
    if (state.features.es && !glClearDepthf) {
        GLfloat d;
        glGetFloatv(GL_DEPTH_CLEAR_VALUE, &d);
        if (d != 1)
            return false;
    }
    const int w = state.viewport[2], h = state.viewport[3];
    if (w <= 0 || h <= 0)
        return false;
    const auto start = std::chrono::steady_clock::now();
    if (quality.collectTimings)
        timer_.begin(stats);
    struct EndTimer {
        RenderTimer &timer;
        ~EndTimer() { timer.end(); }
    } endTimer{timer_};
    const int requestedScale = quality.edges == EdgeQuality::High       ? 3
                               : quality.edges == EdgeQuality::Balanced ? 2
                                                                        : 1;
    // Keep a successful fallback until the requested size/quality changes. This
    // avoids retrying failed allocations and invalidating static captures every frame.
    int scale = requestedWidth_ == w && requestedHeight_ == h && requestedScale_ == requestedScale
                    ? actualScale_
                    : requestedScale;
    const int oldWidth = depth_.width, oldHeight = depth_.height;
    for (; scale > 0; --scale) {
        if (w > std::numeric_limits<int>::max() / scale || h > std::numeric_limits<int>::max() / scale)
            continue;
        if (depth_.width != w * scale || depth_.height != h * scale || normals_.width != w * scale ||
            normals_.height != h * scale)
            cached_ = false;
        if (depth_.allocate(w * scale, h * scale) && normals_.allocate(w * scale, h * scale) &&
            mask_.allocate(w * scale, h * scale, false, false))
            break;
    }
    if (!scale) {
        cached_ = false;
        return false;
    }
    const int tw = w * scale, th = h * scale;
    requestedWidth_ = w;
    requestedHeight_ = h;
    requestedScale_ = requestedScale;
    actualScale_ = scale;
    stats.edgeTargetWidth = tw;
    stats.edgeTargetHeight = th;
    stats.edgeSupersampling = scale;
    bool reuse = quality.cacheStaticEdges && cached_ && revision == revision_ && oldWidth == tw &&
                 oldHeight == th && projection == projection_ && view == view_;
    stats.edgeCaptureReused = reuse;
    if (!reuse) {
        cached_ = false;
        features_.clear();
        for (bool depth : {true, false}) {
            glBindFramebuffer(GL_FRAMEBUFFER, depth ? depth_.framebuffer : normals_.framebuffer);
            glViewport(0, 0, tw, th);
            state.rasterDefaults();
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            draw(depth);
        }
        cached_ = quality.cacheStaticEdges;
        revision_ = revision;
        projection_ = projection;
        view_ = view;
    }
    // The mask shares the nearest-surface depth buffer for explicit feature edges.
    glBindFramebuffer(GL_FRAMEBUFFER, mask_.framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, normals_.depth);
    glViewport(0, 0, tw, th);
    state.rasterDefaults();
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
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
    composite_->setParameter("texel", 1.0 / tw, 1.0 / th);
    composite_->setParameter("edgeColor", settings.color[0], settings.color[1], settings.color[2]);
    composite_->setParameter("edgeWidth", settings.width * scale);
    composite_->setParameter("normalThreshold",
                             1.0 - std::cos(settings.normalAngle * 3.141592653589793 / 180.0));
    Renderer3D::drawVertexBufferObject(quad_, composite_->attribute("position"), GL_TRIANGLES);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    if (!features_.empty()) {
        glEnable(GL_DEPTH_TEST);
        feature_->bind();
        feature_->setParameter("viewport", double(tw), double(th));
        feature_->setParameter("width", settings.width * scale);
        feature_->setParameter("edgeColor", settings.color[0], settings.color[1], settings.color[2]);
        const GLint attributes[] = {feature_->attribute("position"), feature_->attribute("normal"),
                                    feature_->attribute("corner")};
        for (const auto &command : features_) {
            feature_->setParameter("mvp", Eigen::Matrix4d(projection * view * command.model));
            const auto &vertices = *command.vertices;
            if (!featureBuffer_.vertexVBO())
                featureBuffer_.loadVertices(vertices.data(), vertices.size(), GL_DYNAMIC_DRAW);
            else
                featureBuffer_.updateVertices(vertices.data(), vertices.size());
            glBindBuffer(GL_ARRAY_BUFFER, featureBuffer_.vertexVBO());
            for (int i = 0; i < 3; ++i) {
                glEnableVertexAttribArray(attributes[i]);
                glVertexAttribPointer(attributes[i], i == 2 ? 2 : 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                      reinterpret_cast<const void *>(size_t(i) * 3 * sizeof(float)));
            }
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
            for (auto attr : attributes)
                glDisableVertexAttribArray(attr);
        }
        glDisable(GL_DEPTH_TEST);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, state.drawFbo);
    glViewport(state.viewport[0], state.viewport[1], w, h);
    resolve_->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mask_.texture);
    glUniform1i(resolve_->uniform("edgeMask"), 0);
    resolve_->setParameter("texel", 1.0 / tw, 1.0 / th);
    resolve_->setParameter("scale", double(scale));
    Renderer3D::drawVertexBufferObject(quad_, resolve_->attribute("position"), GL_TRIANGLES);
    if (quality.collectTimings)
        stats.edgeCpuMilliseconds =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    return true;
}
} // namespace detail
} // namespace ptgl
