#include "PrimitiveShapeVertex.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace ptgl {

VertexSet PrimitiveShapeVertex::generateRoundedBox(const std::array<double, 3>& sides, double radius, int segments)
{
    for (double side : sides) {
        if (!std::isfinite(side) || side <= 0 || side > std::numeric_limits<float>::max() || float(side) == 0)
            throw std::invalid_argument("Rounded box sides must be positive and representable as floats");
    }
    if (!std::isfinite(radius) || radius < 0 || radius > *std::min_element(sides.begin(), sides.end()) / 2
        || segments < 1 || segments > 64) {
        throw std::invalid_argument("Invalid rounded box radius or segment count");
    }
    VertexSet mesh;
    Eigen::Vector3d half(sides[0] / 2, sides[1] / 2, sides[2] / 2);
    Eigen::Vector3d core = half.array() - radius;
    std::array<std::vector<double>, 3> coordinates;
    for (int axis = 0; axis < 3; ++axis) {
        if (radius == 0) {
            coordinates[axis] = {-half[axis], half[axis]};
        } else {
            for (int i = 0; i <= segments; ++i) {
                coordinates[axis].push_back(-half[axis] + radius * i / segments);
            }
            // Avoid duplicate center rows when the radius reaches a half-side.
            for (int i = core[axis] == 0 ? 1 : 0; i <= segments; ++i) {
                coordinates[axis].push_back(core[axis] + radius * i / segments);
            }
        }
    }
    for (int axis = 0; axis < 3; ++axis) {
        int u = (axis + 1) % 3, v = (axis + 2) % 3;
        GLuint columns = static_cast<GLuint>(coordinates[u].size());
        GLuint rows = static_cast<GLuint>(coordinates[v].size());
        for (int sign : {-1, 1}) {
            GLuint start = static_cast<GLuint>(mesh.vertices.size());
            for (double y : coordinates[v]) {
                for (double x : coordinates[u]) {
                    Eigen::Vector3d p = Eigen::Vector3d::Zero();
                    p[axis] = sign * half[axis]; p[u] = x; p[v] = y;
                    Eigen::Vector3d n = Eigen::Vector3d::Zero();
                    n[axis] = sign;
                    if (radius > 0) {
                        Eigen::Vector3d center = p.cwiseMax(-core).cwiseMin(core);
                        n = (p - center).normalized();
                        p = center + radius * n;
                    }
                    mesh.vertices.emplace_back(static_cast<float>(p.x()), static_cast<float>(p.y()),
                        static_cast<float>(p.z()), static_cast<float>(n.x()), static_cast<float>(n.y()), static_cast<float>(n.z()));
                }
            }
            for (GLuint row = 0; row + 1 < rows; ++row) {
                for (GLuint col = 0; col + 1 < columns; ++col) {
                    GLuint a = start + row * columns + col, b = a + 1, c = a + columns, d = c + 1;
                    if (sign > 0) mesh.indices.insert(mesh.indices.end(), {a, b, c, b, d, c});
                    else mesh.indices.insert(mesh.indices.end(), {a, c, b, b, c, d});
                }
            }
        }
    }
    // Explicit flat-face/fillet boundaries for CAD mode. A face disappears when
    // either in-plane core dimension reaches zero (capsule/sphere limit).
    if (radius > 0) for (int axis = 0; axis < 3; ++axis) {
        const int u = (axis + 1) % 3, v = (axis + 2) % 3;
        if (core[u] == 0 || core[v] == 0) continue;
        for (int sign : {-1, 1}) {
            const GLuint base = static_cast<GLuint>(mesh.vertices.size());
            for (const auto& corner : {std::pair<int,int>{-1,-1}, {1,-1}, {1,1}, {-1,1}}) {
                Eigen::Vector3d p = Eigen::Vector3d::Zero(), n = Eigen::Vector3d::Zero();
                p[axis] = sign * half[axis]; p[u] = corner.first * core[u]; p[v] = corner.second * core[v];
                n[axis] = sign;
                mesh.vertices.emplace_back(float(p.x()), float(p.y()), float(p.z()), float(n.x()), float(n.y()), float(n.z()));
            }
            for (GLuint i = 0; i < 4; ++i) mesh.edges.insert(mesh.edges.end(), {base + i, base + (i + 1) % 4});
        }
    }
    return mesh;
}

} // namespace ptgl
