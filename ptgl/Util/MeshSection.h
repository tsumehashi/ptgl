#ifndef PTGL_UTIL_MESHSECTION_H_
#define PTGL_UTIL_MESHSECTION_H_
#include "ptgl/Core/SectionSettings.h"
#include "ptgl/Core/Vertex.h"
#include <Eigen/Geometry>

namespace ptgl
{
struct MeshSection {
    VertexSet surface;
    VertexSet cap;
    SectionStatus status = SectionStatus::Disabled;
};
// CPU-only half-space clipping. Plane uses the mesh's coordinates. Caps require
// an oriented, closed mesh; an invalid cap leaves the clipped surface available.
// Inputs are never modified. Explicit feature edges are clipped as well.
MeshSection sectionMesh(const VertexSet &mesh, const SectionSettings &plane);
} // namespace ptgl
#endif
