#ifndef PTGL_CORE_PRIMITIVESHAPEVERTEX_H_
#define PTGL_CORE_PRIMITIVESHAPEVERTEX_H_

#include <array>
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

    // Centered box with smooth rounded edges. Create once and reuse the mesh.
    // Positive finite sides, radius in [0, min(sides)/2], segments in [1, 64].
    static VertexSet generateRoundedBox(const std::array<double, 3>& sides, double radius, int segments = 6);

    // Closed cylinder along Z, centered at the origin. Fillet radius is in
    // [0,min(radius,length/2)]. Segments [1,64] controls each quarter-circle.
    // Generated meshes include CAD tangent-boundary edges.
    static VertexSet generateRoundedCylinder(double length, double radius, double filletRadius, int segments = 6);
    // Fillets the base rim and tip inside the original sharp cone envelope:
    // base z=-length/2, tip z=length/2. The rounded tip is lower than the sharp tip.
    // Fillet radius <= radius*length/(hypot(length,radius)+radius), the inradius.
    static VertexSet generateRoundedCone(double length, double radius, double filletRadius, int segments = 6);

    static void setVertexBothSide(VertexSet& vertexSet);
};

} /* namespace ptgl */

#endif /* PTGL_CORE_PRIMITIVESHAPEVERTEX_H_ */
