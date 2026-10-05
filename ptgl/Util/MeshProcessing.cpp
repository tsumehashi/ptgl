#include "MeshProcessing.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <tuple>
#include <Eigen/Geometry>
namespace ptgl
{
namespace
{
struct Groups {
    std::vector<size_t> parents;
    explicit Groups(size_t n) : parents(n) { std::iota(parents.begin(), parents.end(), 0); }
    size_t root(size_t n)
    {
        while (parents[n] != n) {
            parents[n] = parents[parents[n]];
            n = parents[n];
        }
        return n;
    }
    void join(size_t a, size_t b) { parents[root(a)] = root(b); }
};
Eigen::Vector3d position(const Vertex &v)
{
    return {v.x, v.y, v.z};
}
} // namespace
VertexSet prepareCadMesh(const VertexSet &mesh, const MeshProcessingSettings &settings)
{
    auto invalid = [] { throw std::invalid_argument("prepareCadMesh: invalid mesh or settings"); };
    if (!std::isfinite(settings.creaseAngle) || settings.creaseAngle < 0 || settings.creaseAngle > 180 ||
        !std::isfinite(settings.weldTolerance) || settings.weldTolerance < 1e-12 ||
        settings.weldTolerance > 1)
        invalid();
    const size_t n = mesh.vertices.size();
    if (n > std::numeric_limits<GLuint>::max() || mesh.edges.size() % 2)
        invalid();
    IndexList indices = mesh.indices;
    if (indices.empty()) {
        indices.resize(n);
        std::iota(indices.begin(), indices.end(), 0);
    }
    if (indices.size() % 3 || indices.size() > std::numeric_limits<GLuint>::max())
        invalid();
    for (auto i : indices)
        if (i >= n)
            invalid();
    for (auto i : mesh.edges)
        if (i >= n)
            invalid();
    Eigen::AlignedBox3d bounds;
    for (const auto &v : mesh.vertices) {
        auto p = position(v);
        if (!p.allFinite())
            invalid();
        bounds.extend(p);
    }
    if (!n)
        return {};
    // Normalize before spatial hashing, keeping very small/large model units safe.
    const double extent = std::max(bounds.sizes().maxCoeff(), 1e-300);
    const double tolerance = std::max(settings.weldTolerance, 1e-12);
    using Cell = std::tuple<long long, long long, long long>;
    std::map<Cell, std::vector<GLuint>> cells;
    std::vector<Eigen::Vector3d> positions;
    IndexList welded(n);
    for (size_t i = 0; i < n; ++i) {
        Eigen::Vector3d p = (position(mesh.vertices[i]) - bounds.min()) / extent;
        long long x = static_cast<long long>(std::floor(p.x() / tolerance));
        long long y = static_cast<long long>(std::floor(p.y() / tolerance));
        long long z = static_cast<long long>(std::floor(p.z() / tolerance));
        GLuint found = static_cast<GLuint>(positions.size());
        for (int dx = -1; dx <= 1; ++dx)
            for (int dy = -1; dy <= 1; ++dy)
                for (int dz = -1; dz <= 1; ++dz) {
                    auto it = cells.find({x + dx, y + dy, z + dz});
                    if (it == cells.end())
                        continue;
                    for (auto j : it->second)
                        if ((positions[j] - p).norm() <= tolerance)
                            found = std::min(found, j);
                }
        if (found == positions.size()) {
            positions.push_back(p);
            cells[{x, y, z}].push_back(found);
        }
        welded[i] = found;
    }
    VertexSet out;
    IndexList corners, first(n, std::numeric_limits<GLuint>::max());
    std::vector<Eigen::Vector3d> areas;
    using Edge = std::pair<GLuint, GLuint>;
    struct Side {
        size_t a, b, face;
    };
    std::map<Edge, std::vector<Side>> adjacency;
    for (size_t i = 0; i < indices.size(); i += 3) {
        GLuint a = welded[indices[i]], b = welded[indices[i + 1]], c = welded[indices[i + 2]];
        Eigen::Vector3d area = (positions[b] - positions[a]).cross(positions[c] - positions[a]);
        if (a == b || b == c || c == a || area.squaredNorm() == 0)
            continue;
        size_t base = corners.size(), face = areas.size();
        areas.push_back(area);
        for (int k = 0; k < 3; ++k) {
            GLuint original = indices[i + k];
            corners.push_back(welded[original]);
            first[original] = static_cast<GLuint>(out.vertices.size());
            out.indices.push_back(static_cast<GLuint>(out.vertices.size()));
            out.vertices.push_back(mesh.vertices[original]);
        }
        for (int k = 0; k < 3; ++k) {
            size_t u = base + k, v = base + (k + 1) % 3;
            adjacency[std::minmax(corners[u], corners[v])].push_back({u, v, face});
        }
    }
    Groups groups(corners.size());
    const double threshold = std::cos(settings.creaseAngle * 3.141592653589793 / 180.0);
    std::map<Edge, bool> features;
    for (const auto &entry : adjacency) {
        const auto &sides = entry.second;
        bool soft = sides.size() == 2 &&
                    areas[sides[0].face].normalized().dot(areas[sides[1].face].normalized()) >= threshold;
        if (soft) {
            for (auto u : {sides[0].a, sides[0].b})
                for (auto v : {sides[1].a, sides[1].b})
                    if (corners[u] == corners[v])
                        groups.join(u, v);
        } else {
            const auto &side = sides.front();
            out.edges.insert(out.edges.end(), {static_cast<GLuint>(side.a), static_cast<GLuint>(side.b)});
            features[entry.first] = true;
        }
    }
    std::vector<Eigen::Vector3d> sums(corners.size(), Eigen::Vector3d::Zero());
    for (size_t i = 0; i < corners.size(); ++i)
        sums[groups.root(i)] += areas[i / 3];
    for (size_t i = 0; i < corners.size(); ++i) {
        Eigen::Vector3d normal = settings.smoothNormals ? sums[groups.root(i)] : areas[i / 3];
        if (normal.squaredNorm() == 0)
            normal = areas[i / 3];
        normal.normalize();
        auto &v = out.vertices[i];
        v.nx = float(normal.x());
        v.ny = float(normal.y());
        v.nz = float(normal.z());
    }
    // With no surviving surface, feature-only vertices must not be interpreted
    // by drawMesh as an unindexed triangle soup.
    if (out.indices.empty())
        return {};
    for (size_t i = 0; i < mesh.edges.size(); i += 2) {
        GLuint a = mesh.edges[i], b = mesh.edges[i + 1];
        Edge key = std::minmax(welded[a], welded[b]);
        if (key.first == key.second || features.count(key))
            continue;
        for (auto j : {a, b}) {
            if (first[j] == std::numeric_limits<GLuint>::max()) {
                first[j] = static_cast<GLuint>(out.vertices.size());
                out.vertices.push_back(mesh.vertices[j]);
            }
            out.edges.push_back(first[j]);
        }
        features[key] = true;
    }
    return out;
}
} // namespace ptgl
