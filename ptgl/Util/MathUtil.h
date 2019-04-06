#ifndef PTGL_UTIL_MATHUTIL_H_
#define PTGL_UTIL_MATHUTIL_H_

#include <cmath>
#include <Eigen/Core>
#include <Eigen/Geometry>

namespace ptgl {

inline double degree(double rad) { return (180.0 * rad / M_PI); }
inline double radian(double deg) { return (M_PI * deg / 180.0); }

// https://github.com/s-nakaoka/choreonoid/src/Util/EigenUtil.cpp
template<typename Derived>
inline Eigen::Matrix<typename Eigen::MatrixBase<Derived>::Scalar, 3, 1>
rpyFromRot(const Eigen::MatrixBase<Derived>& R) {
    Eigen::Vector3d ea = R.eulerAngles(2, 1, 0);
    return Eigen::Vector3d(ea[2], ea[1], ea[0]); // exchange element order to be our conventional one !
}

Eigen::Matrix3d rotFromRpy(double r, double p, double y);

inline Eigen::Matrix3d rotFromRpy(const Eigen::Vector3d& rpy) {
    return rotFromRpy((double)rpy[0], (double)rpy[1], (double)rpy[2]);
}

inline Eigen::Matrix3d rotationX(double rad)
{
    return rotFromRpy(rad, 0.0, 0.0);
}

inline Eigen::Matrix3d rotationY(double rad)
{
    return rotFromRpy(0.0, rad, 0.0);
}

inline Eigen::Matrix3d rotationZ(double rad)
{
    return rotFromRpy(0.0, 0.0, rad);
}

inline Eigen::Matrix3d rodrigues(const Eigen::Vector3d& axis, double q)
{
    Eigen::AngleAxisd axisd(q, axis);
    return axisd.toRotationMatrix();
}

inline Eigen::Matrix3d rodorigues(const Eigen::Vector3d& w)
{
    double a = w.norm();
    if (a < 1.0e-6) { // identity
        return Eigen::Matrix3d::Identity();
    } else {
        return rodrigues(w.normalized(), a);
    }
}

Eigen::Vector3d omegaFromRot(const Eigen::Matrix3d& R);

inline Eigen::Matrix3d rotFromTo(const Eigen::Vector3d& vec0, const Eigen::Vector3d& vec1) {
    if (vec0.norm() < 0.0000001 || vec1.norm() < 0.0000001) {
        return Eigen::Matrix3d::Identity();
    }
    Eigen::Quaterniond q;
    q.setFromTwoVectors(vec0, vec1);
    return q.toRotationMatrix();
}

Eigen::Matrix3d rotFromAxisFromTo(const Eigen::Vector3d& from, const Eigen::Vector3d& to, const Eigen::Vector3d& axis);
Eigen::Matrix3d rotFromToSet(const Eigen::Vector3d& vecA0, const Eigen::Vector3d& vecB0, const Eigen::Vector3d& vecA1, const Eigen::Vector3d& vecB1);

// t = [0.0, 1.0]
inline Eigen::Matrix3d rotInterpolate(const Eigen::Matrix3d& R0, const Eigen::Matrix3d& R1, double t) {
    Eigen::Quaterniond q0(R0);
    Eigen::Quaterniond qf(R1);

    return q0.slerp(t, qf).toRotationMatrix();
}

// Rotate around axis
double angleFromAxisFromTo(const Eigen::Vector3d& from, const Eigen::Vector3d& to, const Eigen::Vector3d& axis);

// Rotate in direction about orthogonal axis of from and to
double angleFromDirectionFromTo(const Eigen::Vector3d& from, const Eigen::Vector3d& to, const Eigen::Vector3d& direction);

inline Eigen::Vector3d axisFromTo(const Eigen::Vector3d& from, const Eigen::Vector3d& to) {
    Eigen::Quaterniond q;
    q.setFromTwoVectors(from, to);

    Eigen::AngleAxisd aa(q);
    return aa.axis();
}

inline double calcAngleFromTo(const Eigen::Vector3d& from, const Eigen::Vector3d& to)
{
    double angle = acos(from.dot(to)/(from.norm()*to.norm()));
    return angle;
}

// https://bitbucket.org/eigen/eigen/pull-requests/43/openglsupport-tweaks-for-opengl-30/diff#Lunsupported/Eigen/OpenGLSupportT240
template<typename Scalar>
inline Eigen::Matrix<Scalar,4,4> perspective(Scalar fovy, Scalar aspect, Scalar zNear, Scalar zFar)
{
    Eigen::Transform<Scalar, 3, Eigen::Projective> tr;
    tr.matrix().setZero();
//    assert(aspect > 0);
//    assert(zFar > zNear);
    double radf = M_PI * fovy / 180.0;
    double tan_half_fovy = std::tan(radf / 2.0);
    tr(0,0) = 1.0 / (aspect * tan_half_fovy);
    tr(1,1) = 1.0 / (tan_half_fovy);
    tr(2,2) = - (zFar + zNear) / (zFar - zNear);
    tr(3,2) = - 1.0;
    tr(2,3) = - (2.0 * zFar * zNear) / (zFar - zNear);
    return tr.matrix();
}

template<typename Derived>
inline Eigen::Matrix<typename Derived::Scalar,4,4> lookAt(Derived const & eye, Derived const & center, Derived const & up)
{
    typedef Eigen::Matrix<typename Derived::Scalar,4,4> Matrix4;
    typedef Eigen::Matrix<typename Derived::Scalar,3,1> Vector3;
    Vector3 f = (center - eye).normalized();
    Vector3 u = up.normalized();
    Vector3 s = f.cross(u).normalized();
    u = s.cross(f);
    Matrix4 mat = Matrix4::Zero();
    mat(0,0) = s.x();
    mat(0,1) = s.y();
    mat(0,2) = s.z();
    mat(0,3) = -s.dot(eye);
    mat(1,0) = u.x();
    mat(1,1) = u.y();
    mat(1,2) = u.z();
    mat(1,3) = -u.dot(eye);
    mat(2,0) = -f.x();
    mat(2,1) = -f.y();
    mat(2,2) = -f.z();
    mat(2,3) = f.dot(eye);
    mat.row(3) << 0,0,0,1;
    return mat;
}

// ortho
Eigen::Matrix4d ortho(double left, double right, double bottom, double top);
Eigen::Matrix4d ortho(double left, double right, double bottom, double top, double zNear, double zFar);

// transformation
inline Eigen::Affine3d transformation(const Eigen::Vector3d& p, const Eigen::Matrix3d& R)
{
    Eigen::Affine3d t(Eigen::Affine3d::Identity());
    t.matrix().block<3,3>(0,0) = R;
    t.matrix().block<3,1>(0,3) = p;
    return t;
}

inline Eigen::Affine3d transformation(const double* p, const double* R)
{
    Eigen::Affine3d t;
    Eigen::Matrix4d& m = t.matrix();
    m << R[0], R[3], R[6], p[0],
         R[1], R[4], R[7], p[1],
         R[2], R[5], R[8], p[2],
          0.0,  0.0,  0.0,  1.0;
    return t;
}

// project/unproject
// object(x,y,z) -> window(x,y,z)
Eigen::Vector3d project(const Eigen::Vector3d& object_p, const Eigen::Matrix4d& modelMatrix, const Eigen::Matrix4d& projMatrix, const Eigen::Vector4d& viewport);

// window(x,y,z) -> object(x,y,z)
Eigen::Vector3d unProject(const Eigen::Vector3d& window_p, const Eigen::Matrix4d& modelMatrix, const Eigen::Matrix4d& projMatrix, const Eigen::Vector4d& viewport);

} /* namespace ptgl */

#endif /* PTGL_UTIL_MATHUTIL_H_ */
