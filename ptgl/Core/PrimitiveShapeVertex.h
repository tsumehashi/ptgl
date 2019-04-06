#ifndef PTGL_CORE_PRIMITIVESHAPEVERTEX_H_
#define PTGL_CORE_PRIMITIVESHAPEVERTEX_H_

#include "Vertex.h"

namespace ptgl {

class PrimitiveShapeVertex {
public:
    PrimitiveShapeVertex();
    virtual ~PrimitiveShapeVertex();

    // Point
    static const VertexList PointVertices;
    static const std::vector<GLuint> PointIndices;

    // Box
    static const VertexList BoxVertices;
    static const std::vector<GLuint> BoxIndices;

    // Sphere
    static const VertexList SphereVertices;
    static const std::vector<GLuint> SphereIndices;

    // Cylinder
    static const VertexList CylinderVertices;
    static const std::vector<GLuint> CylinderIndices;

    // Cylinder-Side
    static const VertexList CylinderSideVertices;
    static const std::vector<GLuint> CylinderSideIndices;

    // UpperSphere
    static const VertexList UpperSphereVertices;
    static const std::vector<GLuint> UpperSphereIndices;

    // LowerSphere
    static const VertexList LowerSphereVertices;
    static const std::vector<GLuint> LowerSphereIndices;

    // Cone
    static const VertexList ConeVertices;
    static const std::vector<GLuint> ConeIndices;

    // Ring

    // Circle
    static const VertexList CircleVertices;
    static const std::vector<GLuint> CircleIndices;

    static const VertexList CircleLineVertices;
    static const std::vector<GLuint> CircleLineIndices;

    // Rect
    static const VertexList RectVertices;
    static const std::vector<GLuint> RectIndices;

    // RingCircle
    static VertexSet generateRingCircle(double outer_radius, double inner_radius, bool bothSide = true);

    static void setVertexBothSide(VertexSet& vertexSet);
};

} /* namespace ptgl */

#endif /* PTGL_CORE_PRIMITIVESHAPEVERTEX_H_ */
