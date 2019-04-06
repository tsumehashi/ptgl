#include "Intersection.h"

namespace ptgl {

Eigen::Vector3d calcIntersectionPointAndLine(const Eigen::Vector3d& p, const Eigen::Vector3d& line_p1, const Eigen::Vector3d& line_p2)
{
    Eigen::Vector3d d(line_p2 - line_p1);
    d.normalize();
    Eigen::Vector3d v(p - line_p1);
    double t = d.dot(v);

    return Eigen::Vector3d(line_p1 + t*d);
}

// determine the normal of the plane from the 3 -point
Eigen::Vector3d calcPlaneNorm(const Eigen::Vector3d& plane_p1, const Eigen::Vector3d& plane_p2, const Eigen::Vector3d& plane_p3)
{
    Eigen::Vector3d a = plane_p2 - plane_p1;
    Eigen::Vector3d b = plane_p3 - plane_p1;
    Eigen::Vector3d norm = a.cross(b);
    return norm.normalized();
}

// p : point, plane_n : normal of the plane, plane_p : points on the plane
Eigen::Vector3d calcIntersectionPointAndPlane(const Eigen::Vector3d& p, const Eigen::Vector3d& plane_n, const Eigen::Vector3d& plane_p)
{
    double d = -Eigen::Vector3d(p - plane_p).dot(plane_n);
    Eigen::Vector3d pp = plane_n*d + p;
    return pp;
}

// np : near point
bool calcIntersectionLineAndLine(Eigen::Vector3d& line1_np, Eigen::Vector3d& line2_np,
        const Eigen::Vector3d& line1_p1, const Eigen::Vector3d& line1_p2,
        const Eigen::Vector3d& line2_p1, const Eigen::Vector3d& line2_p2
        )
{

    const Eigen::Vector3d& p1 = line1_p1;
    Eigen::Vector3d d1(line1_p2 - line1_p1);

    const Eigen::Vector3d& p2 = line2_p1;
    Eigen::Vector3d d2(line2_p2 - line2_p1);

    Eigen::Vector3d c(d1.cross(d2));
    double a = c.squaredNorm();
    if (a < 0.000001) {    // 平行

        return false;
    } else {
        Eigen::Vector3d v1(Eigen::Vector3d(p2 - p1).cross(d2));
        Eigen::Vector3d v2(Eigen::Vector3d(p2 - p1).cross(d1));
        double t1 = v1.dot(c) / a;
        double t2 = v2.dot(c) / a;

        line1_np = p1 + t1 * d1;
        line2_np = p2 + t2 * d2;

        return true;
    }
}

Eigen::Vector3d calcIntersectionLineAndPlane(const Eigen::Vector3d& line_p1, const Eigen::Vector3d& line_p2,
        const Eigen::Vector3d& plane_p1, const Eigen::Vector3d& plane_p2, const Eigen::Vector3d& plane_p3
        )
{
    const Eigen::Vector3d& p1 = line_p1;
    Eigen::Vector3d d1(line_p2 - line_p1);

    Eigen::Vector3d n = Eigen::Vector3d(plane_p1 - plane_p2).cross(Eigen::Vector3d(plane_p1 - plane_p3));
    n.normalize();
    double d = n.dot(plane_p1);

    double t;
    double dn = d1.dot(n);
    if (std::abs(dn) < 0.0000001) {
        return Eigen::Vector3d(0,0,0);
    } else {
        t = (d - p1.dot(n)) / dn;
    }

    return Eigen::Vector3d(p1 + t * d1);
}

Eigen::Vector3d calcIntersectionLineAndPlane(const Eigen::Vector3d& line_p1, const Eigen::Vector3d& line_p2,
        const Eigen::Vector3d& plane_p, const Eigen::Vector3d& plane_norm
        )
{
    const Eigen::Vector3d& p1 = line_p1;
    Eigen::Vector3d d1(line_p2 - line_p1);

    const Eigen::Vector3d& n = plane_norm;
//    n.normalize();
    double d = n.dot(plane_p);

    double t;
    double dn = d1.dot(n);
    if (std::abs(dn) < 0.0000001) {
        return Eigen::Vector3d(0,0,0);
    } else {
        t = (d - p1.dot(n)) / dn;
    }

    return Eigen::Vector3d(p1 + t * d1);
}

bool calcIntersectionLineAndSphere(Eigen::Vector3d& intersection_p,
        const Eigen::Vector3d& line_p1, const Eigen::Vector3d& line_p2,
        const Eigen::Vector3d& sphere_p, double radius
        )
{
    Eigen::Vector3d d = line_p2 - line_p1;
    d.normalize();

    Eigen::Vector3d ev(sphere_p - line_p1);
    double a = ev.dot(d);
    double e2 = ev.dot(ev);

    double sq = sqrt(radius*radius - e2 + a*a);
    if (sq < 0) {
        return false;
    } else {
        double t = a - sq;
        intersection_p = line_p1 + t * d;
        return true;
    }
}

double calcPlanePointPerpendicular(Eigen::Vector3d& perpendicularPos, const Eigen::Vector3d& p, const Eigen::Vector3d& planePos, const Eigen::Vector3d& planeNorm)
{
    double a = planeNorm(0);
    double b = planeNorm(1);
    double c = planeNorm(2);
    double d = -a*planePos(0) -b*planePos(1) - c*planePos(2);
    double k = - (a*p(0) + b*p(1) + c*p(2) + d)/(a*a + b*b + c*c);
    double l = std::abs<double>(a*p(0) + b*p(1) + c*p(2) + d)/(a*a + b*b + c*c);    // the length of the perpendicular line
    perpendicularPos = Eigen::Vector3d(p(0) + k*a, p(1) + k*b, p(2) + k*c);    // the intersection of the plane and perpendicular

    return l;
}

} /* namespace ptgl */
