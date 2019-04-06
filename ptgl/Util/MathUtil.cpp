#include "MathUtil.h"
#include <Eigen/Geometry>
#include "Intersection.h"

namespace ptgl {

// https://github.com/s-nakaoka/choreonoid/src/Util/EigenUtil.cpp
Eigen::Matrix3d rotFromRpy(double r, double p, double y)
{
    const double cr = cos(r);
    const double sr = sin(r);
    const double cp = cos(p);
    const double sp = sin(p);
    const double cy = cos(y);
    const double sy = sin(y);

    Eigen::Matrix3d R;
    R << cp*cy, sr*sp*cy - cr*sy, cr*sp*cy + sr*sy,
         cp*sy, sr*sp*sy + cr*cy, cr*sp*sy - sr*cy,
         -sp  , sr*cp           , cr*cp;

    return R;
}

// https://github.com/s-nakaoka/choreonoid/src/Util/EigenUtil.cpp
Eigen::Vector3d omegaFromRot(const Eigen::Matrix3d& R)
{
    double alpha = (R(0,0) + R(1,1) + R(2,2) - 1.0) / 2.0;

    if(fabs(alpha - 1.0) < 1.0e-6) {   //th=0,2PI;
        return Eigen::Vector3d::Zero();

    } else {
        double th = acos(alpha);
        double s = sin(th);

        if (s < std::numeric_limits<double>::epsilon()) {   //th=PI
            return Eigen::Vector3d( sqrt((R(0,0)+1)*0.5)*th, sqrt((R(1,1)+1)*0.5)*th, sqrt((R(2,2)+1)*0.5)*th );
        }

        double k = -0.5 * th / s;

        return Eigen::Vector3d((R(1,2) - R(2,1)) * k,
                       (R(2,0) - R(0,2)) * k,
                       (R(0,1) - R(1,0)) * k);
    }
}

Eigen::Matrix3d rotFromAxisFromTo(const Eigen::Vector3d& from, const Eigen::Vector3d& to, const Eigen::Vector3d& axis)
{
    Eigen::Vector3d p0 = ptgl::calcIntersectionPointAndLine(from, Eigen::Vector3d::Zero(), axis);
    Eigen::Vector3d from0 = from - p0;
    Eigen::Vector3d to0 = to - p0;
    return rotFromTo(from0, to0);
}

Eigen::Matrix3d rotFromToSet(const Eigen::Vector3d& vecA0, const Eigen::Vector3d& vecB0, const Eigen::Vector3d& vecA1, const Eigen::Vector3d& vecB1)
{
    Eigen::Matrix3d R0 = ptgl::rotFromTo(vecA0, vecA1);
    Eigen::Vector3d tempVecB = R0 * vecB0;
    return rotFromAxisFromTo(tempVecB, vecB1, vecA1) * R0;
}

double angleFromAxisFromTo(const Eigen::Vector3d& from, const Eigen::Vector3d& to, const Eigen::Vector3d& axis) {
    Eigen::Vector3d p0 = ptgl::calcIntersectionPointAndLine(from, Eigen::Vector3d::Zero(), axis);
    Eigen::Vector3d from0 = from - p0;
    Eigen::Vector3d to0 = to - p0;

    Eigen::Quaterniond q;
    q.setFromTwoVectors(from0, to0);

    Eigen::AngleAxisd aa(q);
    Eigen::Vector3d c = aa.axis();
    double angle = aa.angle();

    Eigen::Vector3d d1 = c - axis;
    Eigen::Vector3d d2 = c + axis;
    return d1.squaredNorm() < d2.squaredNorm() ? angle : -angle;
};

double angleFromDirectionFromTo(const Eigen::Vector3d& from, const Eigen::Vector3d& to, const Eigen::Vector3d& direction) {
    Eigen::Quaterniond q;
    q.setFromTwoVectors(from, to);

    Eigen::AngleAxisd aa(q);
    Eigen::Vector3d c = aa.axis();
    double angle = aa.angle();

    Eigen::Vector3d d1 = c - direction;
    Eigen::Vector3d d2 = c + direction;
    return d1.squaredNorm() < d2.squaredNorm() ? angle : -angle;
};

// glm/gtc/matrix_transform.inl ortho
Eigen::Matrix4d ortho(double left, double right, double bottom, double top)
{
    Eigen::Matrix4d Result(Eigen::Matrix4d::Identity());
    Result(0,0) = 2.0 / (right - left);
    Result(1,1) = 2.0 / (top - bottom);
    Result(0,3) = - (right + left) / (right - left);
    Result(1,3) = - (top + bottom) / (top - bottom);
    return Result;
}

// glm/gtc/matrix_transform.inl  orthoRH_NO
Eigen::Matrix4d ortho(double left, double right, double bottom, double top, double zNear, double zFar)
{
    Eigen::Matrix4d Result(Eigen::Matrix4d::Identity());
    Result(0,0) = 2.0 / (right - left);
    Result(1,1) = 2.0 / (top - bottom);
    Result(2,2) = - 2.0 / (zFar - zNear);
    Result(0,3) = - (right + left) / (right - left);
    Result(1,3) = - (top + bottom) / (top - bottom);
    Result(2,3) = - (zFar + zNear) / (zFar - zNear);
    return Result;
}


// project/unproject

// glm/gtc/matrix_transform.inl  projectNO
// object(x,y,z) -> window(x,y,z)
Eigen::Vector3d project(const Eigen::Vector3d& object_p, const Eigen::Matrix4d& modelMatrix, const Eigen::Matrix4d& projMatrix, const Eigen::Vector4d& viewport)
{
    Eigen::Vector4d tmp = Eigen::Vector4d(object_p(0), object_p(1), object_p(2), 1.0);
    tmp = Eigen::Vector4d(modelMatrix * tmp);
    tmp = Eigen::Vector4d(projMatrix * tmp);

    tmp /= tmp.w();
    tmp = tmp.array() * 0.5 + 0.5;
    tmp[0] = tmp[0] * viewport[2] + viewport[0];
    tmp[1] = tmp[1] * viewport[3] + viewport[1];
    return Eigen::Vector3d(tmp[0], tmp[1], tmp[2]);
}

// glm/gtc/matrix_transform.inl  unProjectNO
// window(x,y,z) -> object(x,y,z)
Eigen::Vector3d unProject(const Eigen::Vector3d& wp, const Eigen::Matrix4d& modelMatrix, const Eigen::Matrix4d& projMatrix, const Eigen::Vector4d& viewport)
{
    Eigen::Matrix4d Inverse = Eigen::Matrix4d(projMatrix * modelMatrix).inverse();

    Eigen::Vector4d tmp = Eigen::Vector4d(wp(0), wp(1), wp(2), 1.0);
    tmp(0) = (tmp(0) - viewport[0]) / viewport[2];
    tmp(1) = (tmp(1) - viewport[1]) / viewport[3];
    tmp = Eigen::Vector4d(tmp.array() * 2.0 - 1.0);

    Eigen::Vector4d obj = Inverse * tmp;
    obj /= obj.w();

    return Eigen::Vector3d(obj[0], obj[1], obj[2]);
}

} /* namespace ptgl */
