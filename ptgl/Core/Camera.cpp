#include "Camera.h"
#include "ptgl/Util/MathUtil.h"
#include "GLPath.h"
#include "Event.h"

namespace ptgl {

Camera::Camera() {

}

Camera::~Camera() {

}

void Camera::initializeConfiguration()
{
    p_.setZero();
    R_.setIdentity();
    Rt_.setIdentity();
    direction_ << 1, 0, 0;
}

void Camera::setPerspective(double fovy, double aspect, double zNear, double zFar)
{
    perspectiveFovy_ = fovy;
    perspectiveAspect_ = aspect;
    perspectiveZnear_ = zNear;
    perspectiveZfar_ = zFar;

    // update projection
    projection_ = ptgl::perspective<double>(perspectiveFovy_, perspectiveAspect_, perspectiveZnear_, perspectiveZfar_);
}

void Camera::setLookAt(const Eigen::Vector3d& eye, const Eigen::Vector3d& center)
{
    Eigen::Vector3d from = Eigen::Vector3d(1,0,0);
    Eigen::Vector3d to = center - eye;
    setRotation(ptgl::rotFromTo(from, to));
    setPosition(eye);
}

void Camera::setPosition(const Eigen::Vector3d& p)
{
    p_ = p;
}

void Camera::setRotation(const Eigen::Matrix3d& R)
{
    R_ = R;
    Rt_ = R_.transpose();
    direction_ = R * Eigen::Vector3d(1,0,0);
}

void Camera::update()
{
    Eigen::Vector3d dir = R_ * Eigen::Vector3d(1,0,0) + p_;
    modelview_ = ptgl::lookAt(p_, dir, Eigen::Vector3d(0,0,1));
}

void Camera::mousePressEvent(MouseEvent* /* e */)
{

}

void Camera::mouseMoveEvent(MouseEvent* /* e */)
{

}

void Camera::mouseReleaseEvent(MouseEvent* /* e */)
{

}

void Camera::wheelEvent(WheelEvent* /* e */)
{

}

void Camera::updateViewport()
{
    //get view port
    int v[4];
    glGetIntegerv(GL_VIEWPORT, v);
    viewport_ << v[0], v[1], v[2], v[3];
}

#if 0
Eigen::Matrix3d Camera::getBillboardMatrix()
{
    Eigen::Matrix3d R = modelview_.block(0,0,3,3);
    return R.transpose()*ptgl::rotationX(-M_PI/2)*ptgl::rotationZ(-M_PI/2);
}
#endif

// object -> window
Eigen::Vector3d Camera::project(double x, double y, double z)
{
    return ptgl::project(Eigen::Vector3d(x,y,z), this->modelview(), this->projection(), this->viewport());
}

// window -> object
Eigen::Vector3d Camera::unProject(double x, double y, double z)
{
    return ptgl::unProject(Eigen::Vector3d(x, y, z), this->modelview(), this->projection(), this->viewport());
}

// object -> window
Eigen::Vector3d Camera::project2D(double x, double y, double z)
{
    Eigen::Vector3d win = this->project(x,y,z);
    return Eigen::Vector3d(win(0), height() - win(1), win(2));
}

// window -> object
Eigen::Vector3d Camera::unProject2D(double x, double y, double z)
{
    return this->unProject(x, height()-y, z);
}

} /* namespace ptgl */
