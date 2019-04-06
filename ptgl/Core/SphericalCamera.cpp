#include "SphericalCamera.h"
#include "ptgl/Util/MathUtil.h"
#include "Event.h"

namespace ptgl {

SphericalCamera::SphericalCamera()
{

}

SphericalCamera::~SphericalCamera()
{

}

void SphericalCamera::setCenter(const Eigen::Vector3d& pos)
{
    center_ = pos;
}

void SphericalCamera::initializeConfiguration()
{
    center_.setZero();
    distance_ = 1.5;
    heading_ = 0.0;
    elevation_ = 45;

    // set camera smooth
    smoothAlpha_ = 0.6;
    prevCenter_ = center_;
    prevDistance_ = distance_;
    prevHeading_ = heading_;
    prevElevation_ = elevation_;
}

void SphericalCamera::update()
{
    auto calcSmoothAngle = [](double deg1, double deg2, double a) {
        if ((deg2 - deg1) > 180) { deg1 += 360;    }
        if ((deg1 - deg2) > 180) { deg2 += 360;    }
        return a * deg1 + (1.0-a) * deg2;
    };
    const double a = smoothAlpha_;
    const double b = (1.0 - smoothAlpha_);
    const Eigen::Vector3d center = a * center_ + b * prevCenter_;
    const double distance = calcSmoothAngle(distance_, prevDistance_, smoothAlpha_);
    const double heading = calcSmoothAngle(heading_, prevHeading_, smoothAlpha_);
    const double elevation = calcSmoothAngle(elevation_, prevElevation_, smoothAlpha_);

    Eigen::Matrix3d Rp = ptgl::rotationZ(ptgl::radian(-heading)) * ptgl::rotationY(ptgl::radian(-elevation));
    Eigen::Vector3d p = Rp * Eigen::Vector3d(distance, 0, 0) + center;

    setLookAt(p, center);

    prevCenter_ = center;
    prevDistance_ = distance;
    prevHeading_ = heading;
    prevElevation_ = elevation;

    Camera::update();
}

void SphericalCamera::mousePressEvent(MouseEvent* /*e*/)
{

}

void SphericalCamera::mouseMoveEvent(MouseEvent* e)
{
    int dx = e->dx();
    int dy = e->dy();

    switch (e->button()) {
    case MouseEvent::MouseButton::LeftButton:

        break;
    case MouseEvent::MouseButton::RightButton:
        viewMotionRotate(dx, dy);
        break;
    case MouseEvent::MouseButton::MiddleButton:
        viewMotionTranslate(dx, dy);
        break;
    default:
        break;
    }
}

void SphericalCamera::mouseReleaseEvent(MouseEvent* /*e*/)
{

}

void SphericalCamera::wheelEvent(WheelEvent* e)
{
    int numDegrees = e->delta() / 8;
    int numTicks = numDegrees * 2;

    double delta = 0.02;
    if (distance_ < 1.0) {
        delta = 0.005;
    }
    distance_ -= delta * numTicks;
    double distMin = 0.1;
    if (distance_ < distMin) {
        distance_ = distMin;
    }
}

double SphericalCamera::wrapAngle(double degree)
{
    return std::remainder(degree, 360.0);
}

void SphericalCamera::viewMotionRotate(int dx, int dy)
{
    double delta = 2.0;
    heading_   += delta * dx;
    elevation_ += delta * dy;

    heading_ = wrapAngle(heading_);
    elevation_ = wrapAngle(elevation_);

    // limit
    if (elevation_ >= 89) elevation_ = 89;
    if (elevation_ <= -89) elevation_ = -89;
}

void SphericalCamera::viewMotionTranslate(int dx, int dy)
{
    double side = -0.01 * dx;
    double fwd = 0.0;
    double s = sin(ptgl::radian(-heading_));
    double c = cos(ptgl::radian(-heading_));

    center_(0) += -s*side + c*fwd;
    center_(1) += c*side + s*fwd;
    center_(2) += 0.01 * dy;
}

} /* namespace ptgl */
