#include "StandardCamera.h"
#include "ptgl/Util/MathUtil.h"
#include "Event.h"

namespace ptgl {

StandardCamera::StandardCamera()
{
    double xyz[3] = {2.7,-3.0, 1.6};
    double hpr[3] = {120.0,-12.0, 0.0};
    set(xyz[0], xyz[1], xyz[2], hpr[0], hpr[1], hpr[2]);
}

StandardCamera::~StandardCamera()
{

}

void StandardCamera::set(double x, double y, double z, double h, double p, double r)
{
    view_xyz_ << x, y, z;
    view_hpr_ << h, p, r;
}

void StandardCamera::update()
{
    if (thrown_) {
        switch (motion_) {
        case 1: viewMotion1(dx_, dy_); break;
        case 2: viewMotion2(dx_, dy_); break;
        case 3: viewMotion2(dx_, dy_); break;
        default: break;
        }
    }

    setCamera();

    Camera::update();
}

void StandardCamera::setCamera(double x, double y, double z, double h, double p, double r)
{
    Eigen::Matrix3d R = ptgl::rotFromRpy(ptgl::radian(r), ptgl::radian(-p), ptgl::radian(h));
    setRotation(R);
    setPosition(Eigen::Vector3d(x, y, z));
}

void StandardCamera::setCamera()
{
    setCamera(view_xyz_(0), view_xyz_(1), view_xyz_(2), view_hpr_(0), view_hpr_(1), view_hpr_[2]);
}

void StandardCamera::mousePressEvent(MouseEvent* event)
{
    px_ = event->x();
    py_ = event->y();
    thrown_ = false;
}

void StandardCamera::mouseMoveEvent(MouseEvent* event)
{
    int dx = event->x() - px_;
    int dy = event->y() - py_;
    dx_ = event->x() - px_;
    dy_ = event->y() - py_;
    px_ = event->x();
    py_ = event->y();
    switch (event->button()) {
    case MouseEvent::MouseButton::LeftButton:

        break;
    case MouseEvent::MouseButton::RightButton:
        viewMotion1(dx, dy);
        motion_ = 1;
        break;
    case MouseEvent::MouseButton::MiddleButton:
        viewMotion3(dx, dy);
        motion_ = 3;
        break;
    default:
        break;
    }

    setCamera();

    thrown_ = false;
    thrown_ = allowThrow_;
}

void StandardCamera::mouseReleaseEvent(MouseEvent* /* event */)
{
    thrown_ = allowThrow_;
}

void StandardCamera::wheelEvent(WheelEvent* event)
{
    int numDegrees = event->delta() / 8;
    int numTicks = numDegrees * 2;
    viewMotion2(0, numTicks); // vertical scroll

    setCamera();
}

// left
void StandardCamera::viewMotion1(int dx, int dy)
{
    view_hpr_(0) += dx * 0.5;
    view_hpr_(1) += dy * 0.5;

    wrapCameraAngles();
}

// rigth
void StandardCamera::viewMotion2(int dx, int dy)
{
    double side = 0.01 * dx;
    double fwd  = 0.01 * dy;
    double s = sin(ptgl::radian(view_hpr_(0)));
    double c = cos(ptgl::radian(view_hpr_(0)));

    view_xyz_(0) += -s*side + c*fwd;
    view_xyz_(1) +=  c*side + s*fwd;

    wrapCameraAngles();
}

// mid
void StandardCamera::viewMotion3(int dx, int dy)
{
    double side = 0.01 * dx;
    double fwd  = 0.0;
    double s = sin(ptgl::radian(view_hpr_(0)));
    double c = cos(ptgl::radian(view_hpr_(0)));

    view_xyz_(0) += -s*side + c*fwd;
    view_xyz_(1) +=  c*side + s*fwd;
    view_xyz_(2) += 0.01 * dy;

    wrapCameraAngles();
}

void StandardCamera::wrapCameraAngles()
{
    view_hpr_(0) =  std::remainder(view_hpr_(0), 360.0);
    view_hpr_(1) =  std::remainder(view_hpr_(1), 360.0);
    view_hpr_(2) =  std::remainder(view_hpr_(2), 360.0);
}

} /* namespace ptgl */
