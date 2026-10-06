#include "PrimitiveShapeVertex.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <Eigen/Geometry>

namespace ptgl
{
namespace
{
constexpr double pi = 3.14159265358979323846;
struct ProfilePoint {
    double radius, z, nr, nz;
    bool edge = false;
};
void validate(double length, double radius, double fillet, int segments)
{
    for (double v : {length, radius})
        if (!std::isfinite(v) || v <= 0 || v > std::numeric_limits<float>::max() || float(v) == 0)
            throw std::invalid_argument(
                "Rounded primitive dimensions must be positive and representable as floats");
    if (!std::isfinite(fillet) || fillet < 0 || segments < 1 || segments > 64)
        throw std::invalid_argument("Invalid rounded primitive fillet radius or segment count");
}
void arc(std::vector<ProfilePoint> &profile, double cr, double cz, double radius, double start, double end,
         int segments, bool firstEdge, bool lastEdge)
{
    for (int i = 0; i <= segments; ++i) {
        const double angle = i == segments ? end : start + (end - start) * i / segments;
        const double nr = std::abs(angle) == pi / 2 ? 0 : std::cos(angle);
        const double nz = angle == 0 ? 0 : std::sin(angle);
        profile.push_back({std::max(0.0, cr + radius * nr), cz + radius * nz, nr, nz,
                           (i == 0 && firstEdge) || (i == segments && lastEdge)});
    }
}
VertexSet revolve(const std::vector<ProfilePoint> &profile, int segments)
{
    VertexSet mesh;
    const GLuint slices = std::max(64, segments * 8), stride = slices + 1;
    for (const auto &p : profile) {
        const GLuint base = static_cast<GLuint>(mesh.vertices.size());
        for (GLuint j = 0; j <= slices; ++j) {
            const double angle = j == slices ? 0 : 2 * pi * j / slices;
            const double x = std::cos(angle), y = std::sin(angle);
            mesh.vertices.emplace_back(float(p.radius * x), float(p.radius * y), float(p.z), float(p.nr * x),
                                       float(p.nr * y), float(p.nz));
            if (p.edge && p.radius > 0 && j < slices)
                mesh.edges.insert(mesh.edges.end(), {base + j, base + j + 1});
        }
    }
    auto triangle = [&](GLuint a, GLuint b, GLuint c) {
        auto point = [&](GLuint i) {
            const auto &v = mesh.vertices[i];
            return Eigen::Vector3d(v.x, v.y, v.z);
        };
        // Coincident pole vertices and normal-split rows need no zero-area faces.
        if ((point(b) - point(a)).cross(point(c) - point(a)).squaredNorm() > 0)
            mesh.indices.insert(mesh.indices.end(), {a, b, c});
    };
    for (GLuint row = 0; row + 1 < profile.size(); ++row)
        for (GLuint j = 0; j < slices; ++j) {
            const GLuint a = row * stride + j, b = a + 1, c = a + stride, d = c + 1;
            triangle(a, b, c);
            triangle(b, d, c);
        }
    return mesh;
}
} // namespace

VertexSet PrimitiveShapeVertex::generateRoundedCylinder(double length, double radius, double fillet,
                                                        int segments)
{
    validate(length, radius, fillet, segments);
    const double h = length / 2;
    if (fillet > std::min(radius, h))
        throw std::invalid_argument("Cylinder fillet exceeds radius or half-length");
    std::vector<ProfilePoint> profile{{0, -h, 0, -1}};
    if (fillet == 0) {
        profile.insert(profile.end(), {{radius, -h, 0, -1, true},
                                       {radius, -h, 1, 0},
                                       {radius, h, 1, 0, true},
                                       {radius, h, 0, 1},
                                       {0, h, 0, 1}});
    } else {
        arc(profile, radius - fillet, -h + fillet, fillet, -pi / 2, 0, segments, radius > fillet, h > fillet);
        // In the disk/sphere limit both arcs meet at the equator; that is not a
        // tangent boundary, so only mark a side when it has positive length.
        arc(profile, radius - fillet, h - fillet, fillet, 0, pi / 2, segments, h > fillet, radius > fillet);
        profile.push_back({0, h, 0, 1});
    }
    return revolve(profile, segments);
}

VertexSet PrimitiveShapeVertex::generateRoundedCone(double length, double radius, double fillet, int segments)
{
    validate(length, radius, fillet, segments);
    const double slant = std::hypot(length, radius), nr = length / slant, nz = radius / slant;
    const double limit = radius * length / (slant + radius), h = length / 2;
    if (fillet > limit)
        throw std::invalid_argument("Cone fillet exceeds its inradius");
    std::vector<ProfilePoint> profile;
    if (fillet == 0) {
        profile = {{0, -h, 0, -1}, {radius, -h, 0, -1, true}, {radius, -h, nr, nz}, {0, h, nr, nz}};
    } else if (fillet == limit) {
        // The maximum fillet leaves an inscribed sphere, with no planar face.
        arc(profile, 0, -h + fillet, fillet, -pi / 2, pi / 2, 2 * segments, false, false);
    } else {
        const double baseR = std::max(0.0, radius - fillet * (1 + nz) / nr);
        const double angle = std::atan2(nz, nr);
        profile.push_back({0, -h, 0, -1});
        arc(profile, baseR, -h + fillet, fillet, -pi / 2, angle, 2 * segments, true, true);
        arc(profile, 0, h - fillet / nz, fillet, angle, pi / 2, segments, true, false);
    }
    return revolve(profile, segments);
}
} // namespace ptgl
