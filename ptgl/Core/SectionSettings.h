#ifndef PTGL_CORE_SECTIONSETTINGS_H_
#define PTGL_CORE_SECTIONSETTINGS_H_
#include <array>

namespace ptgl
{
struct SectionSettings {
    bool enabled = false;
    std::array<double, 3> point{{0, 0, 0}};
    std::array<double, 3> normal{{0, 0, 1}};
    // By default retain dot(position-point,normal) <= 0, in world coordinates.
    bool keepPositiveSide = false;
    bool capEnabled = true;
    std::array<double, 3> capColor{{1.0, 0.65, 0.2}};
};
enum class SectionStatus {
    Disabled,
    Unchanged,
    Empty,
    Clipped,
    Capped,
    OpenMesh,
    InvalidContour,
    InvalidTransform
};
const char *sectionStatusMessage(SectionStatus status);
// Rejects invalid/nonfinite values; returns a copy with a unit normal.
SectionSettings validatedSectionSettings(const SectionSettings &settings);
} // namespace ptgl
#endif
