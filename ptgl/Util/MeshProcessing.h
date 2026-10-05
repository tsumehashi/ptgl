#ifndef PTGL_UTIL_MESHPROCESSING_H_
#define PTGL_UTIL_MESHPROCESSING_H_
#include "ptgl/Core/Vertex.h"
namespace ptgl
{
struct MeshProcessingSettings {
    double creaseAngle = 35.0;   // Degrees, [0,180].
    double weldTolerance = 1e-6; // Fraction of the largest bounding-box dimension, [1e-12,1].
    bool smoothNormals = true;
};
// Preprocess triangle meshes once after loading. Supports indexed meshes and
// triangle soups. Removes degenerate triangles, preserves UVs and explicit edges,
// extracts boundary/crease/nonmanifold edges and repairs normals. Winding is
// retained; inconsistent winding must be corrected by the importer.
// Throws invalid_argument for malformed geometry or invalid settings.
VertexSet prepareCadMesh(const VertexSet &mesh, const MeshProcessingSettings &settings = {});
} // namespace ptgl
#endif
