#include "SceneStyleRenderer.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include "GraphicsDriver.h"
#include "PlasticShaderSource.h"
#include "PlasticShadowMap.h"
#include "PlasticAmbientOcclusion.h"
#include "CadEdgeRenderer.h"
#include "SurfaceRenderer.h"
#include "ptgl/Util/MathUtil.h"

namespace ptgl
{
namespace detail
{
namespace
{

constexpr int slices = 64;
constexpr double pi = 3.14159265358979323846;

void upload(VertexBufferObject &vbo, const VertexSet &mesh)
{
    vbo.loadVertices(mesh.vertices.data(), mesh.vertices.size());
    vbo.loadIndices(mesh.indices.data(), mesh.indices.size());
}

VertexSet sphereMesh(double latitudeStart, double latitudeEnd, int stacks)
{
    VertexSet mesh;
    for (int i = 0; i <= stacks; ++i) {
        double latitude = latitudeStart + (latitudeEnd - latitudeStart) * i / stacks;
        for (int j = 0; j <= slices; ++j) {
            double longitude = 2.0 * pi * j / slices;
            float x = static_cast<float>(std::cos(latitude) * std::cos(longitude));
            float y = static_cast<float>(std::cos(latitude) * std::sin(longitude));
            float z = static_cast<float>(std::sin(latitude));
            mesh.vertices.emplace_back(x, y, z, x, y, z);
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            GLuint a = i * (slices + 1) + j;
            GLuint b = a + 1, c = a + slices + 1, d = c + 1;
            if (i != 0 || latitudeStart > -pi / 2.0) {
                mesh.indices.insert(mesh.indices.end(), {a, b, c});
            }
            if (i != stacks - 1 || latitudeEnd < pi / 2.0) {
                mesh.indices.insert(mesh.indices.end(), {b, d, c});
            }
        }
    }
    return mesh;
}

VertexSet cylinderMesh(bool caps)
{
    VertexSet mesh;
    for (int j = 0; j <= slices; ++j) {
        double angle = 2.0 * pi * j / slices;
        float x = static_cast<float>(std::cos(angle));
        float y = static_cast<float>(std::sin(angle));
        mesh.vertices.emplace_back(x, y, -0.5f, x, y, 0.0f);
        mesh.vertices.emplace_back(x, y, 0.5f, x, y, 0.0f);
        if (j < slices) {
            GLuint a = 2 * j;
            mesh.indices.insert(mesh.indices.end(), {a, a + 2, a + 1, a + 1, a + 2, a + 3});
        }
    }
    if (caps) {
        for (int sign : {-1, 1}) {
            GLuint center = static_cast<GLuint>(mesh.vertices.size());
            float z = 0.5f * sign;
            mesh.vertices.emplace_back(0.0f, 0.0f, z, 0.0f, 0.0f, static_cast<float>(sign));
            for (int j = 0; j <= slices; ++j) {
                double angle = sign * 2.0 * pi * j / slices;
                mesh.vertices.emplace_back(static_cast<float>(std::cos(angle)),
                                           static_cast<float>(std::sin(angle)), z, 0.0f, 0.0f,
                                           static_cast<float>(sign));
                if (j < slices) {
                    GLuint a = center + 1 + j;
                    mesh.indices.insert(mesh.indices.end(), {center, a, a + 1});
                }
            }
        }
    }
    return mesh;
}

// Owns only the extra shading/geometry. GraphicsView still owns all passes,
// input, picking, overlays and UI. No driver-specific code is needed here.
class StyledRenderer3D final : public SceneStyleRenderer
{
  public:
    explicit StyledRenderer3D(GraphicsView *view) : SceneStyleRenderer(view) {}

    void initializeConfiguration() override
    {
        Renderer3D::initializeConfiguration();
        legacyShader_ = shaderProgram();
        cadSurface_.initialize();
        cadRenderer_.initialize();
        cadAvailable.store(bool(cadSurface_.program()));
        shadowMap_.initialize();
        upload(sphere_, sphereMesh(-pi / 2.0, pi / 2.0, 32));
        upload(upperSphere_, sphereMesh(0.0, pi / 2.0, 16));
        upload(lowerSphere_, sphereMesh(-pi / 2.0, 0.0, 16));
        upload(cylinder_, cylinderMesh(true));
        upload(cylinderSide_, cylinderMesh(false));
        plasticSurface_.initialize();
        if (!plasticSurface_.program()) {
            std::cerr << "PlasticGraphicsView: plastic shader unavailable; using legacy rendering.\n";
            return;
        }
        occlusionMap_.initialize();
        available.store(true);
    }

    void finalizeConfiguration() override
    {
        shadowMap_.release();
        occlusionMap_.release();
        cadRenderer_.release();
        cadAvailable.store(false);
        edgesActive.store(false);
        occlusionActive.store(false);
        shadowsActive.store(false);
        available.store(false);
        plasticSurface_.release();
        cadSurface_.release();
        legacyShader_.reset();
        for (auto *vbo : {&sphere_, &upperSphere_, &lowerSphere_, &cylinder_, &cylinderSide_})
            vbo->release();
        Renderer3D::finalizeConfiguration();
    }

    void renderShadows(const std::function<void()> &draw)
    {
        shadowsActive.store(false);
        const bool supportedStyle =
            (style == PlasticGraphicsView::RenderStyle::Plastic && available.load()) ||
            (style == PlasticGraphicsView::RenderStyle::CAD && cadAvailable.load());
        if (!supportedStyle || !shadowMap_.program() || !shadows.enabled || shadows.strength == 0)
            return;
        if (shadows.autoFit) {
            beginBoundsCollection();
            try {
                draw();
            } catch (...) {
                endBoundsCollection();
                throw;
            }
            auto bounds = endBoundsCollection();
            if (!bounds.isEmpty()) {
                Eigen::Vector3d center = bounds.center();
                double radius = bounds.sizes().stableNorm() * 0.5;
                if (!center.allFinite() || !std::isfinite(radius) || radius > 1000000.0)
                    return;
                // Reserve room for receiver bias and center quantization even
                // when the caller requests zero extra padding.
                shadows.halfExtent = std::max(0.05, (radius + shadows.normalBias) * (1.0 + shadows.padding)) /
                                     (1.0 - 2.0 / shadows.resolution);
                // Quantize size and center to reduce shadow shimmer during dragging.
                shadows.halfExtent = std::ceil(shadows.halfExtent * 16.0) / 16.0;
                Eigen::Vector3d direction(lighting.keyDirection.data());
                direction.normalize();
                Eigen::Vector3d up =
                    std::abs(direction.z()) < .95 ? Eigen::Vector3d::UnitZ() : Eigen::Vector3d::UnitY();
                Eigen::Vector3d origin = Eigen::Vector3d::Zero();
                Eigen::Matrix3d basis = ptgl::lookAt(direction, origin, up).block<3, 3>(0, 0);
                double texel = 2.0 * shadows.halfExtent / shadows.resolution;
                center = basis.transpose() * ((basis * center) / texel).array().round().matrix() * texel;
                for (int i = 0; i < 3; ++i)
                    shadows.center[i] = center[i];
            }
        }
        if (!shadowMap_.prepare(shadows, lighting))
            return;
        auto previousForce = forceUseShaderProgram_;
        Eigen::Matrix4d previousProjectionView = projectionViewMatrix();
        shadowPass_ = true;
        setForceUseShaderProgram(shadowMap_.program());
        setProjectionViewMatrix(shadowMap_.projection() * shadowMap_.view());
        try {
            shadowMap_.render(draw);
        } catch (...) {
            endRender(RenderShadowState);
            shadowPass_ = false;
            setForceUseShaderProgram(previousForce);
            setProjectionViewMatrix(previousProjectionView);
            throw;
        }
        shadowPass_ = false;
        setForceUseShaderProgram(previousForce);
        setProjectionViewMatrix(previousProjectionView);
        shadowsActive.store(true);
    }

    void renderOcclusion(const std::function<void()> &draw)
    {
        occlusionActive.store(false);
        if (!available.load() || style != PlasticGraphicsView::RenderStyle::Plastic || !occlusion.enabled ||
            occlusion.strength == 0 || !occlusionMap_.program())
            return;
        auto previous = forceUseShaderProgram_;
        occlusionPass_ = true;
        setForceUseShaderProgram(occlusionMap_.program());
        try {
            occlusionActive.store(occlusionMap_.render(windowWidth(), windowHeight(), draw));
        } catch (...) {
            endRender(RenderOcclusionState);
            occlusionPass_ = false;
            setForceUseShaderProgram(previous);
            throw;
        }
        occlusionPass_ = false;
        setForceUseShaderProgram(previous);
    }

    void drawSphere(const double pos[3], const double rotation[9], double radius) override
    {
        if (!smoothPrimitives_)
            return Renderer3D::drawSphere(pos, rotation, radius);
        drawMesh(sphere_, tf_.transformation() * transformation(pos, rotation) * Eigen::Scaling(radius));
    }

    void renderEdges(const std::function<void()> &draw)
    {
        edgesActive.store(false);
        const auto frame = statistics.frame + 1;
        const auto gpu = statistics.edgeGpuMilliseconds;
        const auto sample = statistics.edgeGpuSampleFrame;
        statistics = {};
        statistics.frame = frame;
        if (quality.collectTimings) {
            statistics.edgeGpuMilliseconds = gpu;
            statistics.edgeGpuSampleFrame = sample;
        }
        if (style != PlasticGraphicsView::RenderStyle::CAD || !cadAvailable.load() || !edges.enabled ||
            !cadRenderer_.captureProgram())
            return;
        auto previous = forceUseShaderProgram_;
        edgePass_ = true;
        setForceUseShaderProgram(cadRenderer_.captureProgram());
        try {
            edgesActive.store(cadRenderer_.render(camera()->projection(), camera()->modelview(), edges,
                                                  quality, edgeRevision, statistics, [&](bool depth) {
                                                      edgeDepth_ = depth;
                                                      draw();
                                                  }));
        } catch (...) {
            endRender(RenderEdgeState);
            edgePass_ = false;
            setForceUseShaderProgram(previous);
            throw;
        }
        edgePass_ = false;
        setForceUseShaderProgram(previous);
    }

    void drawCylinder(const double pos[3], const double rotation[9], double length, double radius,
                      bool cap) override
    {
        if (!smoothPrimitives_)
            return Renderer3D::drawCylinder(pos, rotation, length, radius, cap);
        drawMesh(cap ? cylinder_ : cylinderSide_, tf_.transformation() * transformation(pos, rotation) *
                                                      Eigen::Scaling(radius, radius, length));
    }

    void drawCapsule(const double pos[3], const double rotation[9], double length, double radius) override
    {
        if (!smoothPrimitives_)
            return Renderer3D::drawCapsule(pos, rotation, length, radius);
        drawCylinder(pos, rotation, length, radius, false);
        Eigen::Affine3d base = tf_.transformation() * transformation(pos, rotation);
        drawMesh(upperSphere_, base * Eigen::Translation3d(0, 0, length / 2.0) * Eigen::Scaling(radius));
        drawMesh(lowerSphere_, base * Eigen::Translation3d(0, 0, -length / 2.0) * Eigen::Scaling(radius));
    }

  protected:
    bool acceptsFeatureEdges() const override { return edgePass_ && !edgeDepth_; }
    void submitFeatureEdges(std::shared_ptr<const VertexList> vertices) override
    {
        if (lightEffectRate_ > 0 && color_[3] > 0.001)
            cadRenderer_.addFeatures(std::move(vertices), modelTransform_.matrix());
    }
    void submitMesh(const VertexBufferObject &vbo, GLenum mode = GL_TRIANGLES) override
    {
        if (edgePass_) {
            ++statistics.edgeDrawCalls;
            if (mode == GL_TRIANGLES)
                statistics.edgeTriangles += vbo.numIndices() / 3;
        }
        Renderer3D::submitMesh(vbo, mode);
    }
    void beginRender(RenderState state) override
    {
        if (shadowPass_)
            state = RenderShadowState;
        if (occlusionPass_)
            state = RenderOcclusionState;
        if (edgePass_)
            state = RenderEdgeState;
        auto surface = style == PlasticGraphicsView::RenderStyle::CAD       ? cadSurface_.program()
                       : style == PlasticGraphicsView::RenderStyle::Plastic ? plasticSurface_.program()
                                                                            : nullptr;
        smoothPrimitives_ = surface && (state == RenderSceneState || state == RenderPickingState ||
                                        state == RenderShadowState || state == RenderTransparentState ||
                                        state == RenderOcclusionState || state == RenderEdgeState);
        setDefaultShaderProgram(shadowPass_         ? shadowMap_.program()
                                : occlusionPass_    ? occlusionMap_.program()
                                : edgePass_         ? cadRenderer_.captureProgram()
                                : smoothPrimitives_ ? surface
                                                    : legacyShader_);
        Renderer3D::beginRender(state);
        setMaterial(defaultMaterial);
        if (edgePass_) {
            shaderProgram()->setParameter("captureDepth", edgeDepth_ ? 1.0 : 0.0);
            return;
        }
        if (shadowPass_) {
            shaderProgram()->setParameter("viewMatrix", shadowMap_.view());
            shaderProgram()->setParameter("projectionMatrix", shadowMap_.projection());
            return;
        }
        // The forced picking/depth shaders must never receive color-processing uniforms.
        const bool cadSurface = shaderProgram() == cadSurface_.program();
        if (shaderProgram() != plasticSurface_.program() && !cadSurface)
            return;
        auto program = shaderProgram();
        if (cadSurface)
            cadSurface_.bind(*camera(), lighting, cad, environment, occlusion);
        else
            plasticSurface_.bind(*camera(), lighting, cad, environment, occlusion);
        if (!cadSurface)
            occlusionMap_.bind(program, occlusionActive.load());
        shadowMap_.bind(program, shadowsActive.load());
    }

    void endRender(RenderState state) override
    {
        shadowMap_.unbind();
        occlusionMap_.unbind();
        Renderer3D::endRender(state);
    }

  private:
    void drawMesh(const VertexBufferObject &mesh, const Eigen::Affine3d &transform)
    {
        shaderProgram()->setParameter(unifColorLocation_, color_[0], color_[1], color_[2], color_[3]);
        updateModelMatrixParameter(transform);
        submitMesh(mesh);
    }

    ShaderProgramPtr legacyShader_;
    PlasticSurfaceRenderer plasticSurface_;
    CadSurfaceRenderer cadSurface_;
    VertexBufferObject sphere_, upperSphere_, lowerSphere_, cylinder_, cylinderSide_;
    bool smoothPrimitives_ = false;
    bool shadowPass_ = false;
    bool occlusionPass_ = false;
    bool edgePass_ = false;
    bool edgeDepth_ = false;
    detail::PlasticShadowMap shadowMap_;
    detail::PlasticAmbientOcclusion occlusionMap_;
    detail::CadEdgeRenderer cadRenderer_;
};

} // namespace

std::unique_ptr<SceneStyleRenderer> makeSceneStyleRenderer(GraphicsView *view)
{
    return std::make_unique<StyledRenderer3D>(view);
}
} // namespace detail
} // namespace ptgl
