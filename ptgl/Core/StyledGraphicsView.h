#ifndef PTGL_CORE_STYLEDGRAPHICSVIEW_H_
#define PTGL_CORE_STYLEDGRAPHICSVIEW_H_

#include <mutex>
#include "GraphicsView.h"
#include "RenderSettings.h"

namespace ptgl {

// Switchable Plastic, CAD and Legacy rendering; Plastic is the default style.
class StyledGraphicsView : public GraphicsView {
public:
    using RenderStyle = ptgl::RenderStyle;
    using CadSettings = ptgl::CadSettings;
    using EdgeQuality = ptgl::EdgeQuality;
    using RenderQualitySettings = ptgl::RenderQualitySettings;
    using RenderStatistics = ptgl::RenderStatistics;

    explicit StyledGraphicsView(GraphicsDriverPtr driver);
    ~StyledGraphicsView() override;

    // These settings may be changed from another thread. One snapshot is
    // applied at the next frame, including its picking and depth passes.
    void setRenderStyle(RenderStyle style);
    RenderStyle renderStyle() const;
    void setDefaultMaterial(const Material& material);
    Material defaultMaterial() const;
    // Throws std::invalid_argument for nonfinite/zero directions or colors
    // and exposure outside [0, 16]. Color values are linear RGB, not sRGB.
    void setPlasticLighting(const PlasticLighting& lighting);
    PlasticLighting plasticLighting() const;

    struct ShadowSettings {
        bool enabled = true;
        int resolution = 1024; // Power of two, [256, 4096], limited by the GPU.
        std::array<double, 3> center{{0.0, 0.0, 0.0}};
        double halfExtent = 10.0; // World-space half-width of the light's ortho volume.
        double softness = 1.5;    // PCF radius in shadow texels, [0, 4].
        double bias = 0.0002;     // Normalized shadow depth, [0, 0.01].
        double normalBias = 0.015; // World-space receiver offset, [0, halfExtent].
        double strength = 1.0;    // [0, 1]; affects the key light only.
        bool autoFit = false;    // Fit visible mesh geometry, including view callbacks.
        double padding = 0.1;    // Fractional margin for autoFit, [0, 1].
    };
    // Thread safe; invalid settings throw std::invalid_argument.
    void setShadowSettings(const ShadowSettings& settings);
    ShadowSettings shadowSettings() const;
    ShadowSettings effectiveShadowSettings() const;
    // Whether the last prepared frame used a valid shadow map.
    bool shadowsActive() const;

    struct EnvironmentSettings {
        bool enabled = true;
        double strength = 0.7; // [0, 4], procedural studio environment.
        double rotation = 0.0; // Degrees about world Z, [-360, 360].
    };
    void setEnvironmentSettings(const EnvironmentSettings& settings);
    EnvironmentSettings environmentSettings() const;
    struct AmbientOcclusionSettings {
        bool enabled = true;
        double radius = 0.5; // World-space radius, (0, 100].
        double strength = 1.0; // [0, 4], ambient illumination only.
        double bias = 0.02; // World-space surface offset, [0, radius].
    };
    void setAmbientOcclusionSettings(const AmbientOcclusionSettings& settings);
    AmbientOcclusionSettings ambientOcclusionSettings() const;
    bool ambientOcclusionActive() const;

    // CAD uses bright surface shading and visible mesh/screen-space edges. Width is
    // measured in framebuffer pixels; angle controls sharp normal transitions.
    struct EdgeSettings {
        bool enabled = true;
        std::array<double, 3> color{{0.08, 0.10, 0.13}};
        double width = 1.0; // Framebuffer pixels, [0.5, 4]
        double normalAngle = 35.0; // Degrees, [5, 120]
    };
    void setEdgeSettings(const EdgeSettings& settings);
    EdgeSettings edgeSettings() const;
    bool cadRenderingAvailable() const;
    bool edgesActive() const;
    void setCadSettings(const CadSettings& settings);
    CadSettings cadSettings() const;
    void setRenderQualitySettings(const RenderQualitySettings& settings);
    RenderQualitySettings renderQualitySettings() const;
    // Needed after scene-content changes when cacheStaticEdges is enabled.
    // Thread safe; takes effect at the next frame boundary.
    void invalidateEdgeCache();
    void notifySceneChanged() override { invalidateEdgeCache(); }
    // Last completed frame; GPU samples can originate from an earlier frame.
    RenderStatistics renderStatistics() const;
    using SectionSettings = ptgl::SectionSettings;
    // One world-space plane; affects triangle meshes in all three styles, not overlays.
    void setSectionSettings(const SectionSettings& settings);
    SectionSettings sectionSettings() const;

    // False until the GL context has successfully compiled the plastic shader.
    // A compilation failure is logged and rendering falls back to Legacy.
    bool plasticRenderingAvailable() const;

protected:
    void executeRenderEvent() override;
    void executePrepareRenderScene(Renderer3D* r) override;
    void executeRenderScenePostProcess(Renderer3D* r) override;

private:
    mutable std::mutex settingsMutex_;
    RenderStyle renderStyle_ = RenderStyle::Plastic;
    Material defaultMaterial_;
    PlasticLighting lighting_;
    ShadowSettings shadows_;
    ShadowSettings effectiveShadows_;
    EnvironmentSettings environment_;
    AmbientOcclusionSettings occlusion_;
    EdgeSettings edges_;
    CadSettings cad_;
    RenderQualitySettings quality_;
    RenderStatistics statistics_;
    SectionSettings section_;
    std::uint64_t edgeRevision_ = 0;
};

} // namespace ptgl

#endif
