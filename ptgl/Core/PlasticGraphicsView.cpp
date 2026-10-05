#include "PlasticGraphicsView.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include "GraphicsDriver.h"
#include "PlasticShaderSource.h"
#include "PlasticShadowMap.h"
#include "PlasticAmbientOcclusion.h"
#include "CadEdgeRenderer.h"
#include "ptgl/Util/MathUtil.h"

namespace ptgl {
namespace {

constexpr int slices = 64;
constexpr double pi = 3.14159265358979323846;

void upload(VertexBufferObject& vbo, const VertexSet& mesh)
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
                    static_cast<float>(std::sin(angle)), z, 0.0f, 0.0f, static_cast<float>(sign));
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
class PlasticRenderer3D : public Renderer3D {
public:
    explicit PlasticRenderer3D(GraphicsView* view) : Renderer3D(view) {}

    PlasticGraphicsView::RenderStyle style = PlasticGraphicsView::RenderStyle::Plastic;
    Material defaultMaterial;
    PlasticLighting lighting;
    PlasticGraphicsView::ShadowSettings shadows;
    PlasticGraphicsView::EnvironmentSettings environment;
    PlasticGraphicsView::AmbientOcclusionSettings occlusion;
    PlasticGraphicsView::EdgeSettings edges;
    std::atomic<bool> available{false};
    std::atomic<bool> shadowsActive{false};
    std::atomic<bool> occlusionActive{false};
    std::atomic<bool> cadAvailable{false};
    std::atomic<bool> edgesActive{false};

    void initializeConfiguration() override
    {
        Renderer3D::initializeConfiguration();
        legacyShader_ = shaderProgram();
        cadRenderer_.initialize();
        cadAvailable.store(bool(cadRenderer_.surfaceProgram()));
        shadowMap_.initialize();
        upload(sphere_, sphereMesh(-pi / 2.0, pi / 2.0, 32));
        upload(upperSphere_, sphereMesh(0.0, pi / 2.0, 16));
        upload(lowerSphere_, sphereMesh(-pi / 2.0, 0.0, 16));
        upload(cylinder_, cylinderMesh(true));
        upload(cylinderSide_, cylinderMesh(false));
        auto vertex = Shader::loadFromSource(Shader::VertexShader, PlasticShaderSource::VertexShaderSource);
        auto fragment = Shader::loadFromSource(Shader::FragmentShader, PlasticShaderSource::FragmentShaderSource);
        auto program = std::make_shared<ShaderProgram>();
        if (!vertex || !fragment || !program->linkShaders({vertex, fragment})) {
            std::cerr << "PlasticGraphicsView: plastic shader unavailable; using legacy rendering.\n";
            return;
        }
        plasticShader_ = program;
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
        plasticShader_.reset();
        legacyShader_.reset();
        for (auto* vbo : {&sphere_, &upperSphere_, &lowerSphere_, &cylinder_, &cylinderSide_}) vbo->release();
        Renderer3D::finalizeConfiguration();
    }

    void renderShadows(const std::function<void()>& draw)
    {
        shadowsActive.store(false);
        const bool supportedStyle = (style == PlasticGraphicsView::RenderStyle::Plastic && available.load())
                                 || (style == PlasticGraphicsView::RenderStyle::CAD && cadAvailable.load());
        if (!supportedStyle || !shadowMap_.program() || !shadows.enabled || shadows.strength == 0) return;
        if (shadows.autoFit) {
            beginBoundsCollection();
            try { draw(); } catch (...) { endBoundsCollection(); throw; }
            auto bounds = endBoundsCollection();
            if (!bounds.isEmpty()) {
                Eigen::Vector3d center = bounds.center();
                double radius = bounds.sizes().stableNorm() * 0.5;
                if (!center.allFinite() || !std::isfinite(radius) || radius > 1000000.0) return;
                // Reserve room for receiver bias and center quantization even
                // when the caller requests zero extra padding.
                shadows.halfExtent = std::max(0.05, (radius + shadows.normalBias) * (1.0 + shadows.padding))
                    / (1.0 - 2.0 / shadows.resolution);
                // Quantize size and center to reduce shadow shimmer during dragging.
                shadows.halfExtent = std::ceil(shadows.halfExtent * 16.0) / 16.0;
                Eigen::Vector3d direction(lighting.keyDirection.data()); direction.normalize();
                Eigen::Vector3d up = std::abs(direction.z()) < .95 ? Eigen::Vector3d::UnitZ() : Eigen::Vector3d::UnitY();
                Eigen::Vector3d origin = Eigen::Vector3d::Zero();
                Eigen::Matrix3d basis = ptgl::lookAt(direction, origin, up).block<3,3>(0,0);
                double texel = 2.0 * shadows.halfExtent / shadows.resolution;
                center = basis.transpose() * ((basis * center) / texel).array().round().matrix() * texel;
                for (int i=0;i<3;++i) shadows.center[i]=center[i];
            }
        }
        if (!shadowMap_.prepare(shadows, lighting)) return;
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

    void renderOcclusion(const std::function<void()>& draw)
    {
        occlusionActive.store(false);
        if (!available.load() || style != PlasticGraphicsView::RenderStyle::Plastic || !occlusion.enabled || occlusion.strength == 0 || !occlusionMap_.program()) return;
        auto previous = forceUseShaderProgram_;
        occlusionPass_ = true;
        setForceUseShaderProgram(occlusionMap_.program());
        try {
            occlusionActive.store(occlusionMap_.render(windowWidth(),windowHeight(),draw));
        } catch (...) {
            endRender(RenderOcclusionState); occlusionPass_=false; setForceUseShaderProgram(previous); throw;
        }
        occlusionPass_ = false;setForceUseShaderProgram(previous);
    }

    void drawSphere(const double pos[3], const double rotation[9], double radius) override
    {
        if (!smoothPrimitives_) return Renderer3D::drawSphere(pos, rotation, radius);
        drawMesh(sphere_, tf_.transformation() * transformation(pos, rotation) * Eigen::Scaling(radius));
    }

    void renderEdges(const std::function<void()>& draw)
    {
        edgesActive.store(false);
        if (style != PlasticGraphicsView::RenderStyle::CAD || !cadAvailable.load() ||
            !edges.enabled || !cadRenderer_.captureProgram()) return;
        auto previous = forceUseShaderProgram_;
        edgePass_ = true;
        setForceUseShaderProgram(cadRenderer_.captureProgram());
        try {
            edgesActive.store(cadRenderer_.render(camera()->projection(), edges, [&](bool depth) {
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

    void drawCylinder(const double pos[3], const double rotation[9], double length, double radius, bool cap) override
    {
        if (!smoothPrimitives_) return Renderer3D::drawCylinder(pos, rotation, length, radius, cap);
        drawMesh(cap ? cylinder_ : cylinderSide_,
            tf_.transformation() * transformation(pos, rotation) * Eigen::Scaling(radius, radius, length));
    }

    void drawCapsule(const double pos[3], const double rotation[9], double length, double radius) override
    {
        if (!smoothPrimitives_) return Renderer3D::drawCapsule(pos, rotation, length, radius);
        drawCylinder(pos, rotation, length, radius, false);
        Eigen::Affine3d base = tf_.transformation() * transformation(pos, rotation);
        drawMesh(upperSphere_, base * Eigen::Translation3d(0, 0, length / 2.0) * Eigen::Scaling(radius));
        drawMesh(lowerSphere_, base * Eigen::Translation3d(0, 0, -length / 2.0) * Eigen::Scaling(radius));
    }

protected:
    void beginRender(RenderState state) override
    {
        if (shadowPass_) state = RenderShadowState;
        if (occlusionPass_) state = RenderOcclusionState;
        if (edgePass_) state = RenderEdgeState;
        auto surface = style == PlasticGraphicsView::RenderStyle::CAD ? cadRenderer_.surfaceProgram()
                     : style == PlasticGraphicsView::RenderStyle::Plastic ? plasticShader_ : nullptr;
        smoothPrimitives_ = surface &&
            (state == RenderSceneState || state == RenderPickingState || state == RenderShadowState ||
             state == RenderTransparentState || state == RenderOcclusionState || state == RenderEdgeState);
        setDefaultShaderProgram(shadowPass_ ? shadowMap_.program() : occlusionPass_ ? occlusionMap_.program()
                                : edgePass_ ? cadRenderer_.captureProgram()
                                : smoothPrimitives_ ? surface : legacyShader_);
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
        const bool cadSurface = shaderProgram() == cadRenderer_.surfaceProgram();
        if (shaderProgram() != plasticShader_ && !cadSurface) return;
        auto program = shaderProgram();
        Eigen::Matrix3d viewRotation = camera()->modelview().block<3, 3>(0, 0);
        auto setDirection = [&](const char* name, const std::array<double, 3>& direction) {
            Eigen::Vector3d value(direction[0], direction[1], direction[2]);
            value = viewRotation * (value / value.stableNorm());
            program->setParameter(name, value);
        };
        auto setColor = [&](const char* name, const std::array<double, 3>& c) {
            program->setParameter(name, c[0], c[1], c[2]);
        };
        setDirection("keyDirection", lighting.keyDirection);
        if (cadSurface) {
            shadowMap_.bind(program, shadowsActive.load());
            return;
        }
        setDirection("fillDirection", lighting.fillDirection);
        setColor("keyColor", lighting.keyColor);
        setColor("fillColor", lighting.fillColor);
        setColor("skyColor", lighting.skyColor);
        setColor("groundColor", lighting.groundColor);
        program->setParameter("exposure", lighting.exposure);
        program->setParameter("orthographicCamera", camera()->viewMode() == Camera::Ortho ? 1.0 : 0.0);
        program->setParameter("viewToWorld", Eigen::Matrix3d(viewRotation.transpose()));
        program->setParameter("environmentStrength", environment.enabled ? environment.strength : 0.0);
        program->setParameter("environmentRotation", environment.rotation * pi / 180.0);
        program->setParameter("inverseProjection", Eigen::Matrix4d(camera()->projection().inverse()));
        program->setParameter("occlusionRadius", occlusion.radius);
        program->setParameter("occlusionStrength", occlusion.strength);
        program->setParameter("occlusionBias", occlusion.bias);
        program->setParameter("projectionScale", camera()->projection()(0,0), camera()->projection()(1,1));
        GLint viewport[4];glGetIntegerv(GL_VIEWPORT,viewport);
        program->setParameter("occlusionViewport", double(viewport[0]),double(viewport[1]),double(viewport[2]),double(viewport[3]));
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
    void drawMesh(const VertexBufferObject& mesh, const Eigen::Affine3d& transform)
    {
        shaderProgram()->setParameter(unifColorLocation_, color_[0], color_[1], color_[2], color_[3]);
        updateModelMatrixParameter(transform);
        submitMesh(mesh);
    }

    ShaderProgramPtr legacyShader_, plasticShader_;
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

PlasticGraphicsView::PlasticGraphicsView(GraphicsDriverPtr driver) : GraphicsView(std::move(driver))
{
    renderer3D_ = std::make_unique<PlasticRenderer3D>(this);
}

PlasticGraphicsView::~PlasticGraphicsView() = default;

void PlasticGraphicsView::setRenderStyle(RenderStyle style)
{
    std::lock_guard<std::mutex> lock(settingsMutex_);
    renderStyle_ = style;
}

PlasticGraphicsView::RenderStyle PlasticGraphicsView::renderStyle() const
{
    std::lock_guard<std::mutex> lock(settingsMutex_);
    return renderStyle_;
}

void PlasticGraphicsView::setDefaultMaterial(const Material& material)
{
    std::lock_guard<std::mutex> lock(settingsMutex_);
    defaultMaterial_ = material;
}

Material PlasticGraphicsView::defaultMaterial() const
{
    std::lock_guard<std::mutex> lock(settingsMutex_);
    return defaultMaterial_;
}

void PlasticGraphicsView::setPlasticLighting(const PlasticLighting& lighting)
{
    auto validDirection = [](const std::array<double, 3>& d) {
        Eigen::Vector3d v(d[0], d[1], d[2]);
        return v.allFinite() && std::isfinite(v.stableNorm()) && v.stableNorm() > 0.0001;
    };
    auto validColor = [](const std::array<double, 3>& c) {
        return std::all_of(c.begin(), c.end(), [](double x) { return std::isfinite(x) && x >= 0 && x <= 16; });
    };
    if (!validDirection(lighting.keyDirection) || !validDirection(lighting.fillDirection)
        || !validColor(lighting.keyColor) || !validColor(lighting.fillColor)
        || !validColor(lighting.skyColor) || !validColor(lighting.groundColor)
        || !std::isfinite(lighting.exposure) || lighting.exposure < 0 || lighting.exposure > 16) {
        throw std::invalid_argument("PlasticLighting requires nonzero finite directions and colors/exposure in [0, 16]");
    }
    std::lock_guard<std::mutex> lock(settingsMutex_);
    lighting_ = lighting;
}

PlasticLighting PlasticGraphicsView::plasticLighting() const
{
    std::lock_guard<std::mutex> lock(settingsMutex_);
    return lighting_;
}

void PlasticGraphicsView::setShadowSettings(const ShadowSettings& settings)
{
    auto inRange = [](double value, double lo, double hi) {
        return std::isfinite(value) && value >= lo && value <= hi;
    };
    if (settings.resolution < 256 || settings.resolution > 4096
        || (settings.resolution & (settings.resolution - 1)) != 0
        || !std::all_of(settings.center.begin(), settings.center.end(), [](double v) { return std::isfinite(v); })
        || !inRange(settings.halfExtent, 0.01, 1000000.0)
        || !inRange(settings.softness, 0.0, 4.0) || !inRange(settings.bias, 0.0, 0.01)
        || !inRange(settings.normalBias, 0.0, settings.halfExtent) || !inRange(settings.strength, 0.0, 1.0)
        || !inRange(settings.padding, 0.0, 1.0)) {
        throw std::invalid_argument("Invalid plastic shadow settings (resolution, bounds, softness, bias or strength)");
    }
    std::lock_guard<std::mutex> lock(settingsMutex_);
    shadows_ = settings;
}

PlasticGraphicsView::ShadowSettings PlasticGraphicsView::shadowSettings() const
{
    std::lock_guard<std::mutex> lock(settingsMutex_);
    return shadows_;
}

PlasticGraphicsView::ShadowSettings PlasticGraphicsView::effectiveShadowSettings() const
{
    std::lock_guard<std::mutex> lock(settingsMutex_);
    return effectiveShadows_;
}

bool PlasticGraphicsView::shadowsActive() const
{
    return static_cast<PlasticRenderer3D*>(renderer3D_.get())->shadowsActive.load();
}

bool PlasticGraphicsView::plasticRenderingAvailable() const
{
    return static_cast<PlasticRenderer3D*>(renderer3D_.get())->available.load();
}

void PlasticGraphicsView::setEnvironmentSettings(const EnvironmentSettings &settings)
{
    if (!std::isfinite(settings.strength) || settings.strength < 0 || settings.strength > 4 ||
        !std::isfinite(settings.rotation) || std::abs(settings.rotation) > 360)
        throw std::invalid_argument("Invalid environment strength/rotation");
    std::lock_guard<std::mutex> lock(settingsMutex_);
    environment_ = settings;
}

PlasticGraphicsView::EnvironmentSettings PlasticGraphicsView::environmentSettings() const
{
    std::lock_guard<std::mutex> lock(settingsMutex_);
    return environment_;
}

void PlasticGraphicsView::setAmbientOcclusionSettings(const AmbientOcclusionSettings &settings)
{
    if (!std::isfinite(settings.radius) || settings.radius <= 0 || settings.radius > 100 ||
        !std::isfinite(settings.strength) || settings.strength < 0 || settings.strength > 4 ||
        !std::isfinite(settings.bias) || settings.bias < 0 || settings.bias > settings.radius)
        throw std::invalid_argument("Invalid ambient occlusion radius/strength/bias");
    std::lock_guard<std::mutex> lock(settingsMutex_);
    occlusion_ = settings;
}

PlasticGraphicsView::AmbientOcclusionSettings PlasticGraphicsView::ambientOcclusionSettings() const
{
    std::lock_guard<std::mutex> lock(settingsMutex_);
    return occlusion_;
}

bool PlasticGraphicsView::ambientOcclusionActive() const
{
    return static_cast<PlasticRenderer3D *>(renderer3D_.get())->occlusionActive.load();
}

void PlasticGraphicsView::setEdgeSettings(const EdgeSettings& settings)
{
    if (!std::isfinite(settings.width) || settings.width < 0.5 || settings.width > 4.0 ||
        !std::isfinite(settings.normalAngle) || settings.normalAngle < 5 || settings.normalAngle > 120 ||
        !std::all_of(settings.color.begin(), settings.color.end(),
                     [](double v) { return std::isfinite(v) && v >= 0 && v <= 1; })) {
        throw std::invalid_argument("Invalid CAD edge width, angle or color");
    }
    std::lock_guard<std::mutex> lock(settingsMutex_);
    edges_ = settings;
}

PlasticGraphicsView::EdgeSettings PlasticGraphicsView::edgeSettings() const
{
    std::lock_guard<std::mutex> lock(settingsMutex_);
    return edges_;
}

bool PlasticGraphicsView::cadRenderingAvailable() const
{
    return static_cast<PlasticRenderer3D*>(renderer3D_.get())->cadAvailable.load();
}

bool PlasticGraphicsView::edgesActive() const
{
    return static_cast<PlasticRenderer3D*>(renderer3D_.get())->edgesActive.load();
}

void PlasticGraphicsView::executeRenderEvent()
{
    auto* renderer = static_cast<PlasticRenderer3D*>(renderer3D_.get());
    {
        std::lock_guard<std::mutex> lock(settingsMutex_);
        renderer->style = renderStyle_;
        renderer->defaultMaterial = defaultMaterial_;
        renderer->lighting = lighting_;
        renderer->shadows = shadows_;
        renderer->environment = environment_;
        renderer->occlusion = occlusion_;
        renderer->edges = edges_;
    }
    GraphicsView::executeRenderEvent();
}

void PlasticGraphicsView::executePrepareRenderScene(Renderer3D* r)
{
    GraphicsView::executePrepareRenderScene(r);
    static_cast<PlasticRenderer3D*>(r)->renderShadows([&] { executeRenderScene(r); });
    static_cast<PlasticRenderer3D*>(r)->renderOcclusion([&] { executeRenderScene(r); });
    std::lock_guard<std::mutex> lock(settingsMutex_);
    effectiveShadows_ = static_cast<PlasticRenderer3D*>(r)->shadows;
}

void PlasticGraphicsView::executeRenderScenePostProcess(Renderer3D* r)
{
    static_cast<PlasticRenderer3D*>(r)->renderEdges([&] { executeRenderScene(r); });
    GraphicsView::executeRenderScenePostProcess(r);
}

} // namespace ptgl
