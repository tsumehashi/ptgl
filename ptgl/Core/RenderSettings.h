#ifndef PTGL_CORE_RENDERSETTINGS_H_
#define PTGL_CORE_RENDERSETTINGS_H_

#include <array>
#include <cstdint>

namespace ptgl
{

enum class RenderStyle { Legacy, Plastic, CAD };

struct PlasticLighting {
    // World-space directions toward the lights; linear RGB radiance.
    std::array<double, 3> keyDirection{{1.0, -1.0, 2.0}};
    std::array<double, 3> keyColor{{4.0, 3.8, 3.5}};
    std::array<double, 3> fillDirection{{-1.0, -0.5, 1.0}};
    std::array<double, 3> fillColor{{0.8, 0.9, 1.1}};
    std::array<double, 3> skyColor{{0.55, 0.60, 0.70}};
    std::array<double, 3> groundColor{{0.12, 0.10, 0.08}};
    double exposure = 1.0;
};

struct CadSettings {
    double brightness = 1.0; // [0, 4], applied to lit CAD surfaces only.
    double ambient = 0.70;   // [0, 2]
    double key = 0.36;       // [0, 2], uses PlasticLighting::keyDirection and shadows.
    double fill = 0.10;      // [0, 2]
    std::array<double, 3> fillDirection{{0.8, -0.2, 0.4}}; // View space, nonzero.
    double specular = 0.06;                                // [0, 1]
    double shininess = 32.0;                               // [1, 256]
};

enum class EdgeQuality { Fast, Balanced, High }; // 1x, 2x, 3x per axis.

struct RenderQualitySettings {
    EdgeQuality edges = EdgeQuality::Balanced;
    // Opt-in for static geometry. Camera/viewport/quality changes invalidate
    // automatically; call invalidateEdgeCache() after changing scene geometry,
    // visibility, sidedness or zero-opacity membership in rendering callbacks.
    bool cacheStaticEdges = false;
    bool collectTimings = false;
};

struct RenderStatistics {
    double edgeCpuMilliseconds = 0.0;
    double edgeGpuMilliseconds = -1.0; // Asynchronous sample; -1 = unavailable.
    std::uint64_t edgeGpuSampleFrame = 0;
    std::uint64_t frame = 0;
    std::uint64_t edgeDrawCalls = 0;
    std::uint64_t edgeTriangles = 0;
    int edgeTargetWidth = 0, edgeTargetHeight = 0;
    int edgeSupersampling = 0; // Actual scale after resource-limit fallback.
    bool edgeCaptureReused = false;
};

} // namespace ptgl
#endif
