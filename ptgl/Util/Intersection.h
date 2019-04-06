#ifndef PTGL_UTIL_INTERSECTION_H_
#define PTGL_UTIL_INTERSECTION_H_

#include <Eigen/Core>
#include <Eigen/Geometry>

namespace ptgl {

Eigen::Vector3d calcIntersectionPointAndLine(const Eigen::Vector3d& p, const Eigen::Vector3d& line_p1, const Eigen::Vector3d& line_p2);

// determine the normal of the plane from the 3 -point
Eigen::Vector3d calcPlaneNorm(const Eigen::Vector3d& plane_p1, const Eigen::Vector3d& plane_p2, const Eigen::Vector3d& plane_p3);

// p : point, plane_n : normal of the plane, plane_p : points on the plane
Eigen::Vector3d calcIntersectionPointAndPlane(const Eigen::Vector3d& p, const Eigen::Vector3d& plane_n, const Eigen::Vector3d& plane_p);

// np : near point
bool calcIntersectionLineAndLine(Eigen::Vector3d& line1_np, Eigen::Vector3d& line2_np,
        const Eigen::Vector3d& line1_p1, const Eigen::Vector3d& line1_p2,
        const Eigen::Vector3d& line2_p1, const Eigen::Vector3d& line2_p2
        );

Eigen::Vector3d calcIntersectionLineAndPlane(const Eigen::Vector3d& line_p1, const Eigen::Vector3d& line_p2,
        const Eigen::Vector3d& plane_p1, const Eigen::Vector3d& plane_p2, const Eigen::Vector3d& plane_p3
        );

Eigen::Vector3d calcIntersectionLineAndPlane(const Eigen::Vector3d& line_p1, const Eigen::Vector3d& line_p2,
        const Eigen::Vector3d& plane_p, const Eigen::Vector3d& plane_norm
        );

bool calcIntersectionLineAndSphere(Eigen::Vector3d& intersection_p,
        const Eigen::Vector3d& line_p1, const Eigen::Vector3d& line_p2,
        const Eigen::Vector3d& sphere_p, double radius
        );

// determining the length and perpendicular with the intersection of the plane of the perpendicular point on the plane and space
double calcPlanePointPerpendicular(Eigen::Vector3d& perpendicularPos, const Eigen::Vector3d& p, const Eigen::Vector3d& planePos, const Eigen::Vector3d& planeNorm);

} /* namespace ptgl */

#endif /* PTGL_UTIL_INTERSECTION_H_ */
