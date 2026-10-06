#include "PrimitiveShape.h"
#include <type_traits>

namespace ptgl
{
VertexSet generatePrimitiveMesh(const PrimitiveShape &shape)
{
    return std::visit(
        [](const auto &s) -> VertexSet {
            using T = std::decay_t<decltype(s)>;
            if constexpr (std::is_same_v<T, BoxShape>)
                return PrimitiveShapeVertex::generateRoundedBox(s.sides, 0);
            else if constexpr (std::is_same_v<T, SphereShape>)
                return PrimitiveShapeVertex::generateRoundedCylinder(2 * s.radius, s.radius, s.radius,
                                                                     s.segments);
            else if constexpr (std::is_same_v<T, CylinderShape>)
                return PrimitiveShapeVertex::generateRoundedCylinder(s.length, s.radius, 0, s.segments);
            else if constexpr (std::is_same_v<T, ConeShape>)
                return PrimitiveShapeVertex::generateRoundedCone(s.length, s.radius, 0, s.segments);
            else if constexpr (std::is_same_v<T, RoundedBoxShape>)
                return PrimitiveShapeVertex::generateRoundedBox(s.sides, s.radius, s.segments);
            else if constexpr (std::is_same_v<T, RoundedCylinderShape>)
                return PrimitiveShapeVertex::generateRoundedCylinder(s.length, s.radius, s.filletRadius,
                                                                     s.segments);
            else
                return PrimitiveShapeVertex::generateRoundedCone(s.length, s.radius, s.filletRadius,
                                                                 s.segments);
        },
        shape);
}
} // namespace ptgl
