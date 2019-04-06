#ifndef PTGL_CORE_VERTEX_H_
#define PTGL_CORE_VERTEX_H_

#include <memory>
#include <vector>
#include <Eigen/Core>
#include "GLPath.h"

namespace ptgl {

struct Vertex {
    union {
        struct {
            GLfloat x;    // vertex
            GLfloat y;
            GLfloat z;
            GLfloat nx;    // normal
            GLfloat ny;
            GLfloat nz;
            GLfloat u;    // uv
            GLfloat v;
        };
        GLfloat data[8];
    };

    Vertex() :
        x(0), y(0), z(0), nx(0), ny(0), nz(0), u(0), v(0)
    {

    }

    Vertex(GLfloat x, GLfloat y, GLfloat z, GLfloat nx = 0.0, GLfloat ny = 0.0, GLfloat nz = 0.0, GLfloat u = 0.0, GLfloat v = 0.0) :
        x(x), y(y), z(z), nx(nx), ny(ny), nz(nz), u(u), v(v)
    {

    }
};

typedef std::vector<Vertex> VertexList;
typedef std::shared_ptr<VertexList> VertexListPtr;

typedef std::vector<GLuint> IndexList;
typedef std::shared_ptr<IndexList> IndexListPtr;

struct VertexSet {
    VertexList vertices;
    IndexList indices;
};

inline void applyScale(VertexList& vertices, double sx, double sy, double sz)
{
    if (vertices.empty()) {
        return;
    }

    for (auto& v : vertices) {
        v.x *= sx;
        v.y *= sy;
        v.z *= sz;
    }
}

inline void applyScale(VertexList& vertices, double s)
{
    applyScale(vertices, s, s, s);
}

inline bool calcBoundingBox(Eigen::Vector3d& center, Eigen::Vector3d& sides, const VertexList& vertices)
{
    if (vertices.empty()) {
        return false;
    }

    double xmin = vertices[0].x;
    double xmax = vertices[0].x;
    double ymin = vertices[0].y;
    double ymax = vertices[0].y;
    double zmin = vertices[0].z;
    double zmax = vertices[0].z;

    for (const auto& v : vertices) {
        if (v.x < xmin) xmin = v.x;
        if (v.x > xmax) xmax = v.x;
        if (v.y < ymin) ymin = v.y;
        if (v.y > ymax) ymax = v.y;
        if (v.z < zmin) zmin = v.z;
        if (v.z > zmax) zmax = v.z;
    }

    center(0) = (xmax + xmin)/2.0;
    center(1) = (ymax + ymin)/2.0;
    center(2) = (zmax + zmin)/2.0;

    sides(0) = std::abs(xmax - xmin);
    sides(1) = std::abs(ymax - ymin);
    sides(2) = std::abs(zmax - zmin);

    return true;
}

} /* namespace ptgl */

#endif /* PTGL_CORE_VERTEX_H_ */
