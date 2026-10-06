#include "MeshSection.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace ptgl
{
SectionSettings validatedSectionSettings(const SectionSettings &settings)
{
    SectionSettings result = settings;
    Eigen::Vector3d n(settings.normal.data()), p(settings.point.data());
    const double length = n.stableNorm();
    if (!n.allFinite() || !p.allFinite() || !std::isfinite(length) || length <= 0 ||
        !std::all_of(settings.capColor.begin(), settings.capColor.end(),
                     [](double c) { return std::isfinite(c) && c >= 0 && c <= 1; }))
        throw std::invalid_argument("Section plane needs a finite point, nonzero normal and RGB in [0,1]");
    n /= length;
    std::copy(n.data(), n.data() + 3, result.normal.begin());
    return result;
}

const char *sectionStatusMessage(SectionStatus status)
{
    switch (status) {
    case SectionStatus::Disabled:
        return "Section disabled";
    case SectionStatus::Unchanged:
        return "Plane does not cut the mesh";
    case SectionStatus::Empty:
        return "Mesh lies outside the retained half-space";
    case SectionStatus::Clipped:
        return "Clipped without a cap";
    case SectionStatus::Capped:
        return "Clipped and capped";
    case SectionStatus::OpenMesh:
        return "Cap unavailable: mesh is open, nonmanifold or inconsistently wound";
    case SectionStatus::InvalidContour:
        return "Cap unavailable: section contour is not a set of closed simple loops";
    case SectionStatus::InvalidTransform:
        return "Section unavailable: model transform is singular or nonfinite";
    }
    return "Unknown section status";
}

namespace
{
Eigen::Vector3d position(const Vertex &v)
{
    return {v.x, v.y, v.z};
}
// Neighbor-cell lookup avoids cracks where near-coincident seam vertices fall
// on opposite sides of a quantization boundary.
struct Weld {
    Eigen::Vector3d origin;
    double epsilon;
    std::vector<Eigen::Vector3d> points;
    std::map<std::array<long long, 3>, std::vector<int>> cells;
    int add(const Eigen::Vector3d &p)
    {
        const Eigen::Vector3d q = (p - origin) / epsilon;
        std::array<long long, 3> cell{{std::llround(q.x()), std::llround(q.y()), std::llround(q.z())}};
        for (int x = -1; x <= 1; ++x)
            for (int y = -1; y <= 1; ++y)
                for (int z = -1; z <= 1; ++z) {
                    auto found = cells.find({cell[0] + x, cell[1] + y, cell[2] + z});
                    if (found != cells.end())
                        for (int id : found->second)
                            if ((points[id] - p).norm() <= epsilon)
                                return id;
                }
        int id = static_cast<int>(points.size());
        cells[cell].push_back(id);
        points.push_back(p);
        return id;
    }
};
using Edge = std::pair<int, int>;
struct Count {
    int total = 0, orientation = 0;
};
void edge(std::map<Edge, Count> &edges, int a, int b)
{
    if (a == b)
        return;
    auto &e = edges[std::minmax(a, b)];
    ++e.total;
    e.orientation += a < b ? 1 : -1;
}
void triangle(VertexSet &mesh, const Vertex &a, const Vertex &b, const Vertex &c)
{
    if ((position(b) - position(a)).cross(position(c) - position(a)).squaredNorm() == 0)
        return;
    const GLuint start = static_cast<GLuint>(mesh.vertices.size());
    mesh.vertices.insert(mesh.vertices.end(), {a, b, c});
    mesh.indices.insert(mesh.indices.end(), {start, start + 1, start + 2});
}
Vertex mix(const Vertex &a, const Vertex &b, double t)
{
    Vertex out;
    for (int i = 0; i < 8; ++i)
        out.data[i] = float(double(a.data[i]) + t * (double(b.data[i]) - a.data[i]));
    Eigen::Vector3d normal(out.nx, out.ny, out.nz);
    if (normal.squaredNorm() > 0) {
        normal.normalize();
        out.nx = float(normal.x());
        out.ny = float(normal.y());
        out.nz = float(normal.z());
    }
    return out;
}
} // namespace

MeshSection sectionMesh(const VertexSet &source, const SectionSettings &settings)
{
    const auto plane = validatedSectionSettings(settings);
    MeshSection result;
    if (!plane.enabled) {
        result.surface = source;
        return result;
    }
    const Eigen::Vector3d p(plane.point.data());
    const Eigen::Vector3d n = Eigen::Vector3d(plane.normal.data()) * (plane.keepPositiveSide ? -1 : 1);
    Eigen::AlignedBox3d bounds;
    for (const auto &v : source.vertices) {
        for (float f : v.data)
            if (!std::isfinite(f))
                throw std::invalid_argument("Nonfinite section mesh vertex");
        bounds.extend(position(v));
    }
    IndexList indices = source.indices;
    if (indices.empty()) {
        indices.resize(source.vertices.size());
        std::iota(indices.begin(), indices.end(), 0);
    }
    if (indices.size() % 3 || source.edges.size() % 2)
        throw std::invalid_argument("Invalid section mesh indices");
    for (auto i : indices)
        if (i >= source.vertices.size())
            throw std::invalid_argument("Section triangle index out of range");
    for (auto i : source.edges)
        if (i >= source.vertices.size())
            throw std::invalid_argument("Section edge index out of range");
    if (indices.empty()) {
        result.status = SectionStatus::Empty;
        return result;
    }
    const double extent = bounds.sizes().maxCoeff();
    if (extent <= 0) {
        result.status = SectionStatus::Empty;
        return result;
    }
    const double epsilon = extent * 1e-6;
    auto distance = [&](const Vertex &v) {
        double d = n.dot(position(v) - p);
        return std::abs(d) <= epsilon ? 0.0 : d;
    };
    bool inside = false, outside = false;
    for (auto i : indices) {
        const double d = distance(source.vertices[i]);
        inside |= d < 0;
        outside |= d > 0;
    }
    if (!outside) {
        result.surface = source;
        result.status = SectionStatus::Unchanged;
        return result;
    }
    if (!inside) {
        result.status = SectionStatus::Empty;
        return result;
    }
    Weld weld{bounds.min(), epsilon};
    std::map<Edge, Count> originalEdges;
    std::vector<int> ids(source.vertices.size(), -1);
    for (auto i : indices)
        if (ids[i] < 0)
            ids[i] = weld.add(position(source.vertices[i]));
    for (size_t i = 0; i < indices.size(); i += 3) {
        std::vector<Vertex> polygon;
        int a = ids[indices[i]], b = ids[indices[i + 1]], c = ids[indices[i + 2]];
        if (a == b || b == c || c == a)
            continue;
        edge(originalEdges, a, b);
        edge(originalEdges, b, c);
        edge(originalEdges, c, a);
        for (int j = 0; j < 3; ++j) {
            const Vertex &va = source.vertices[indices[i + j]];
            const Vertex &vb = source.vertices[indices[i + (j + 1) % 3]];
            const double da = distance(va), db = distance(vb);
            if (da <= 0)
                polygon.push_back(va);
            if ((da < 0 && db > 0) || (da > 0 && db < 0))
                polygon.push_back(mix(va, vb, da / (da - db)));
        }
        for (size_t j = 1; j + 1 < polygon.size(); ++j)
            triangle(result.surface, polygon[0], polygon[j], polygon[j + 1]);
    }
    // Keep input CAD feature lines on the retained side.
    for (size_t i = 0; i < source.edges.size(); i += 2) {
        Vertex a = source.vertices[source.edges[i]], b = source.vertices[source.edges[i + 1]];
        double da = distance(a), db = distance(b);
        if (da > 0 && db > 0)
            continue;
        if (da > 0)
            a = mix(a, b, da / (da - db));
        else if (db > 0)
            b = mix(a, b, da / (da - db));
        GLuint start = static_cast<GLuint>(result.surface.vertices.size());
        result.surface.vertices.insert(result.surface.vertices.end(), {a, b});
        result.surface.edges.insert(result.surface.edges.end(), {start, start + 1});
    }
    result.status = SectionStatus::Clipped;
    // Extract only unmatched plane edges, excluding triangulation diagonals and
    // coplanar faces already supplied by the original mesh.
    Weld boundary{bounds.min(), epsilon * 2};
    std::map<Edge, Count> planeEdges;
    for (size_t i = 0; i < result.surface.indices.size(); i += 3)
        for (int j = 0; j < 3; ++j) {
            const auto &a = result.surface.vertices[result.surface.indices[i + j]];
            const auto &b = result.surface.vertices[result.surface.indices[i + (j + 1) % 3]];
            if (std::abs(n.dot(position(a) - p)) <= epsilon * 2 &&
                std::abs(n.dot(position(b) - p)) <= epsilon * 2)
                edge(planeEdges, boundary.add(position(a)), boundary.add(position(b)));
        }
    std::vector<Edge> segments;
    std::vector<int> degree(boundary.points.size());
    bool validContour = true;
    for (const auto &entry : planeEdges) {
        if (entry.second.total == 2 && entry.second.orientation == 0)
            continue;
        if (entry.second.total != 1) {
            validContour = false;
            continue;
        }
        const auto [a, b] = entry.first;
        ++degree[a];
        ++degree[b];
        segments.push_back({a, b});
        GLuint base = static_cast<GLuint>(result.surface.vertices.size());
        for (int id : {a, b}) {
            const auto &q = boundary.points[id];
            result.surface.vertices.emplace_back(float(q.x()), float(q.y()), float(q.z()));
        }
        result.surface.edges.insert(result.surface.edges.end(), {base, base + 1});
    }
    if (!plane.capEnabled)
        return result;
    for (const auto &e : originalEdges)
        if (e.second.total != 2 || e.second.orientation != 0) {
            result.status = SectionStatus::OpenMesh;
            return result;
        }
    for (int d : degree)
        if (d != 0 && d != 2)
            validContour = false;
    if (!validContour) {
        result.status = SectionStatus::InvalidContour;
        return result;
    }
    // A plane can separate disconnected closed solids without cutting a face.
    // The retained components are already closed and need no additional cap.
    if (segments.empty())
        return result;
    // Tessellate parallel slabs with the even/odd rule. This handles concave
    // loops, disconnected islands and nested holes without filling cavities.
    const Eigen::Vector3d u = n.unitOrthogonal(), v = n.cross(u);
    std::vector<Eigen::Vector2d> points;
    std::vector<double> heights;
    for (const auto &q : boundary.points) {
        points.emplace_back(u.dot(q - p), v.dot(q - p));
        heights.push_back(points.back().y());
    }
    std::sort(heights.begin(), heights.end());
    // Keep every distinct endpoint height. Merging nearby heights can extrapolate
    // steep edges past their endpoints and invert narrow cap triangles.
    heights.erase(std::unique(heights.begin(), heights.end()), heights.end());
    auto vertex = [&](double x, double y) {
        Eigen::Vector3d q = p + u * x + v * y;
        return Vertex(float(q.x()), float(q.y()), float(q.z()), float(n.x()), float(n.y()), float(n.z()));
    };
    auto capTriangle = [&](const Vertex &a, const Vertex &b, const Vertex &c) {
        const double signedArea = (position(b) - position(a)).cross(position(c) - position(a)).dot(n);
        // Float conversion can collapse very thin slabs near a contour vertex.
        if (std::abs(signedArea) <= epsilon * epsilon)
            return;
        if (signedArea > 0)
            triangle(result.cap, a, b, c);
        else
            triangle(result.cap, a, c, b);
    };
    for (size_t row = 1; row < heights.size(); ++row) {
        const double y0 = heights[row - 1], y1 = heights[row], mid = (y0 + y1) / 2;
        if (mid <= y0 || mid >= y1)
            continue; // No representable interior sample.
        struct Span {
            double middle, bottom, top;
        };
        std::vector<Span> spans;
        for (auto e : segments) {
            const auto &a = points[e.first];
            const auto &b = points[e.second];
            if (mid <= std::min(a.y(), b.y()) || mid >= std::max(a.y(), b.y()))
                continue;
            auto x = [&](double y) { return a.x() + (b.x() - a.x()) * (y - a.y()) / (b.y() - a.y()); };
            spans.push_back({x(mid), x(y0), x(y1)});
        }
        std::sort(spans.begin(), spans.end(),
                  [](const Span &a, const Span &b) { return a.middle < b.middle; });
        if (spans.size() % 2) {
            result.cap = {};
            result.status = SectionStatus::InvalidContour;
            return result;
        }
        for (size_t i = 1; i < spans.size(); ++i) {
            if (spans[i - 1].bottom > spans[i].bottom + epsilon ||
                spans[i - 1].top > spans[i].top + epsilon) {
                result.cap = {};
                result.status = SectionStatus::InvalidContour;
                return result;
            }
        }
        for (size_t i = 0; i < spans.size(); i += 2) {
            const auto &l = spans[i];
            const auto &r = spans[i + 1];
            auto a = vertex(l.bottom, y0), b = vertex(r.bottom, y0), c = vertex(r.top, y1),
                 d = vertex(l.top, y1);
            capTriangle(a, b, c);
            capTriangle(a, c, d);
        }
    }
    result.status = result.cap.indices.empty() ? SectionStatus::InvalidContour : SectionStatus::Capped;
    return result;
}
} // namespace ptgl
