#include "PlasticGraphicsView.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include "GraphicsDriver.h"
#include "SceneStyleRenderer.h"

namespace ptgl {
PlasticGraphicsView::PlasticGraphicsView(GraphicsDriverPtr driver) : GraphicsView(std::move(driver))
{
    renderer3D_ = detail::makeSceneStyleRenderer(this);
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
    return static_cast<detail::SceneStyleRenderer*>(renderer3D_.get())->shadowsActive.load();
}

bool PlasticGraphicsView::plasticRenderingAvailable() const
{
    return static_cast<detail::SceneStyleRenderer*>(renderer3D_.get())->available.load();
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
    return static_cast<detail::SceneStyleRenderer *>(renderer3D_.get())->occlusionActive.load();
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
    return static_cast<detail::SceneStyleRenderer*>(renderer3D_.get())->cadAvailable.load();
}

bool PlasticGraphicsView::edgesActive() const
{
    return static_cast<detail::SceneStyleRenderer*>(renderer3D_.get())->edgesActive.load();
}

void PlasticGraphicsView::executeRenderEvent()
{
    auto* renderer = static_cast<detail::SceneStyleRenderer*>(renderer3D_.get());
    {
        std::lock_guard<std::mutex> lock(settingsMutex_);
        renderer->style = renderStyle_;
        renderer->defaultMaterial = defaultMaterial_;
        renderer->lighting = lighting_;
        renderer->shadows = shadows_;
        renderer->environment = environment_;
        renderer->occlusion = occlusion_;
        renderer->edges = edges_;
        renderer->cad = cad_;
        renderer->quality = quality_;
        renderer->edgeRevision = edgeRevision_;
    }
    GraphicsView::executeRenderEvent();
    std::lock_guard<std::mutex> lock(settingsMutex_);
    statistics_ = renderer->statistics;
}

void PlasticGraphicsView::setCadSettings(const CadSettings& settings)
{
    auto range = [](double v, double lo, double hi) { return std::isfinite(v) && v >= lo && v <= hi; };
    Eigen::Vector3d fill(settings.fillDirection.data());
    if (!range(settings.brightness, 0, 4) || !range(settings.ambient, 0, 2) ||
        !range(settings.key, 0, 2) || !range(settings.fill, 0, 2) ||
        !range(settings.specular, 0, 1) || !range(settings.shininess, 1, 256) ||
        !fill.allFinite() || !std::isfinite(fill.stableNorm()) || fill.stableNorm() < 0.0001)
        throw std::invalid_argument("Invalid CAD brightness, lighting or specular settings");
    std::lock_guard<std::mutex> lock(settingsMutex_);
    cad_ = settings;
}

PlasticGraphicsView::CadSettings PlasticGraphicsView::cadSettings() const
{
    std::lock_guard<std::mutex> lock(settingsMutex_);
    return cad_;
}

void PlasticGraphicsView::setRenderQualitySettings(const RenderQualitySettings& settings)
{
    if (settings.edges != EdgeQuality::Fast && settings.edges != EdgeQuality::Balanced &&
        settings.edges != EdgeQuality::High)
        throw std::invalid_argument("Invalid edge quality");
    std::lock_guard<std::mutex> lock(settingsMutex_);
    quality_ = settings;
}

PlasticGraphicsView::RenderQualitySettings PlasticGraphicsView::renderQualitySettings() const
{
    std::lock_guard<std::mutex> lock(settingsMutex_);
    return quality_;
}

void PlasticGraphicsView::invalidateEdgeCache()
{
    std::lock_guard<std::mutex> lock(settingsMutex_);
    ++edgeRevision_;
}

PlasticGraphicsView::RenderStatistics PlasticGraphicsView::renderStatistics() const
{
    std::lock_guard<std::mutex> lock(settingsMutex_);
    return statistics_;
}

void PlasticGraphicsView::executePrepareRenderScene(Renderer3D* r)
{
    GraphicsView::executePrepareRenderScene(r);
    static_cast<detail::SceneStyleRenderer*>(r)->renderShadows([&] { executeRenderScene(r); });
    static_cast<detail::SceneStyleRenderer*>(r)->renderOcclusion([&] { executeRenderScene(r); });
    std::lock_guard<std::mutex> lock(settingsMutex_);
    effectiveShadows_ = static_cast<detail::SceneStyleRenderer*>(r)->shadows;
}

void PlasticGraphicsView::executeRenderScenePostProcess(Renderer3D* r)
{
    static_cast<detail::SceneStyleRenderer*>(r)->renderEdges([&] { executeRenderScene(r); });
    GraphicsView::executeRenderScenePostProcess(r);
}

} // namespace ptgl
