#include "TransformStack.h"
#include "ptgl/Util/MathUtil.h"

namespace ptgl {

TransformStack::TransformStack()
{
    stack_.push(Eigen::Affine3d::Identity());
}

TransformStack::~TransformStack()
{

}

const Eigen::Affine3d& TransformStack::transformation() const
{
    return stack_.top();    // never empty
}

Eigen::Affine3d& TransformStack::tf()
{
    return stack_.top();    // never empty
}

const Eigen::Matrix4d& TransformStack::matrix() const
{
    return transformation().matrix();
}

Eigen::Vector3d TransformStack::getTranslation() const
{
    return transformation().translation();
}

Eigen::Matrix3d TransformStack::getRotation() const
{
    return transformation().rotation();
}

Eigen::Vector3d TransformStack::getScale() const
{
    Eigen::Matrix3d m = transformation().linear();
    double sx = sqrt(m(0,0) * m(0,0) + m(1,0) * m(1,0) + m(2,0) * m(2,0));
    double sy = sqrt(m(0,1) * m(0,1) + m(1,1) * m(1,1) + m(2,1) * m(2,1));
    double sz = sqrt(m(0,2) * m(0,2) + m(1,2) * m(1,2) + m(2,2) * m(2,2));
    return Eigen::Vector3d(sx, sy, sz);
}

size_t TransformStack::size() const
{
    return stack_.size();
}

void TransformStack::clear()
{
    StackType s;
    stack_.swap(s);    // clear
    stack_.push(Eigen::Affine3d::Identity());
}

void TransformStack::push()
{
    stack_.push(stack_.top());
}

void TransformStack::pop()
{
    if (stack_.size() > 1) {
        stack_.pop();
    }
}

void TransformStack::identiry()
{
    stack_.top().Identity();
}

void TransformStack::scale(double s)
{
    tf().scale(s);
}

void TransformStack::scale(double sx, double sy, double sz)
{
    tf().scale(Eigen::Vector3d(sx, sy, sz));
}

void TransformStack::scale(const Eigen::Vector3d s)
{
    tf().scale(s);
}

void TransformStack::translate(double x, double y, double z)
{
    tf().translate(Eigen::Vector3d(x,y,z));
}

void TransformStack::translate(const Eigen::Vector3d t)
{
    tf().translate(t);
}

void TransformStack::rotate(const Eigen::Matrix3d& R)
{
    tf().rotate(R);
}

void TransformStack::rotate(double angle, const Eigen::Vector3d& axis)
{
    tf().rotate(Eigen::AngleAxisd(angle, axis));
}

void TransformStack::rotateRpy(double r, double p, double y)
{
    // todo optimize
#if 0
    rotate(r, Eigen::Vector3d::UnitX());
    rotate(p, Eigen::Vector3d::UnitY());
    rotate(y, Eigen::Vector3d::UnitZ());
#else
    rotate(ptgl::rotFromRpy(r, p, y));
#endif
}

void TransformStack::rotateX(double deg)
{
    rotate(ptgl::radian(deg), Eigen::Vector3d::UnitX());
}

void TransformStack::rotateY(double deg)
{
    rotate(ptgl::radian(deg), Eigen::Vector3d::UnitY());
}

void TransformStack::rotateZ(double deg)
{
    rotate(ptgl::radian(deg), Eigen::Vector3d::UnitZ());
}

void TransformStack::transform(const Eigen::Affine3d& t)
{
    tf().matrix() *= t.matrix();
}

void TransformStack::transform(const Eigen::Matrix4d& m)
{
    tf().matrix() *= m;
}

void TransformStack::transform(const Eigen::Vector3d& p, const Eigen::Matrix3d& R)
{
    Eigen::Matrix4d m(Eigen::Matrix4d::Identity());
    m.block(0,0,3,3) = R;
    m(0,3) = p(0);
    m(1,3) = p(1);
    m(2,3) = p(2);
    tf().matrix() *= m;
}

} /* namespace ptgl */
