#ifndef PTGL_CORE_MATERIAL_H_
#define PTGL_CORE_MATERIAL_H_

namespace ptgl {

// Non-metallic surface. Base color and opacity still come from setColor().
// The legacy shader ignores these properties.
struct Material {
    double roughness = 0.35;           // Perceptual roughness, clamped to [0.12, 1].
    double specularReflectance = 0.04; // Normal-incidence reflectance, clamped to [0, 1].
};

} // namespace ptgl

#endif
