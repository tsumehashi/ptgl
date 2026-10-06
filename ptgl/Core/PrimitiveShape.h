#ifndef PTGL_CORE_PRIMITIVESHAPE_H_
#define PTGL_CORE_PRIMITIVESHAPE_H_
#include <variant>
#include "PrimitiveShapeVertex.h"

namespace ptgl
{
struct BoxShape {
    std::array<double, 3> sides{{1, 1, 1}};
};
struct SphereShape {
    double radius = 0.5;
    int segments = 16;
};
struct CylinderShape {
    double length = 1, radius = 0.5;
    int segments = 8;
};
struct ConeShape {
    double length = 1, radius = 0.5;
    int segments = 8;
};
struct RoundedBoxShape {
    std::array<double, 3> sides{{1, 1, 1}};
    double radius = 0.1;
    int segments = 6;
};
struct RoundedCylinderShape {
    double length = 1, radius = 0.5, filletRadius = 0.1;
    int segments = 6;
};
struct RoundedConeShape {
    double length = 1, radius = 0.5, filletRadius = 0.1;
    int segments = 6;
};
using PrimitiveShape = std::variant<BoxShape, SphereShape, CylinderShape, ConeShape, RoundedBoxShape,
                                    RoundedCylinderShape, RoundedConeShape>;
// Validates all dimensions before generating an immutable, reusable CPU mesh.
VertexSet generatePrimitiveMesh(const PrimitiveShape &shape);
} // namespace ptgl
#endif
