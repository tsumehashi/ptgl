#ifndef PTGL_CORE_PLASTICGRAPHICSVIEW_H_
#define PTGL_CORE_PLASTICGRAPHICSVIEW_H_

#include <mutex>
#include "GraphicsView.h"

namespace ptgl {

struct PlasticLighting {
    // Directions point toward the lights in world coordinates (Z is up).
    std::array<double, 3> keyDirection{{1.0, -1.0, 2.0}};
    std::array<double, 3> keyColor{{4.0, 3.8, 3.5}};
    std::array<double, 3> fillDirection{{-1.0, -0.5, 1.0}};
    std::array<double, 3> fillColor{{0.8, 0.9, 1.1}};
    // Linear RGB irradiance for a simple hemispheric ambient light.
    std::array<double, 3> skyColor{{0.55, 0.60, 0.70}};
    std::array<double, 3> groundColor{{0.12, 0.10, 0.08}};
    double exposure = 1.0;
};

class PlasticGraphicsView : public GraphicsView {
public:
    enum class RenderStyle { Legacy, Plastic };

    explicit PlasticGraphicsView(GraphicsDriverPtr driver);
    ~PlasticGraphicsView() override;

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

    // False until the GL context has successfully compiled the plastic shader.
    // A compilation failure is logged and rendering falls back to Legacy.
    bool plasticRenderingAvailable() const;

protected:
    void executeRenderEvent() override;

private:
    mutable std::mutex settingsMutex_;
    RenderStyle renderStyle_ = RenderStyle::Plastic;
    Material defaultMaterial_;
    PlasticLighting lighting_;
};

} // namespace ptgl

#endif
