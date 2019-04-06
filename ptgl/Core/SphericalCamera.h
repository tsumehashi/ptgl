#ifndef PTGL_CORE_SPHERICALCAMERA_H_
#define PTGL_CORE_SPHERICALCAMERA_H_

#include "Camera.h"

namespace ptgl {

class MouseEvent;
class WheelEvent;

class SphericalCamera;
typedef std::shared_ptr<SphericalCamera> SphericalCameraPtr;

class SphericalCamera : public Camera {
public:
    SphericalCamera();
    virtual ~SphericalCamera();

    void setCenter(const Eigen::Vector3d& pos);
    const Eigen::Vector3d& center() const { return center_; }

    void setDistance(double distance) { distance_ = distance; }
    double distance() const { return distance_; }

    void setHeading(double azimuth) { heading_ = azimuth; }
    double heading() const { return heading_; }

    void setElevation(double elevation) { elevation_ = elevation; }
    double elevation() const { return elevation_; }

    virtual void initializeConfiguration() override;
    virtual void update() override;

    virtual void mousePressEvent(MouseEvent* e) override;
    virtual void mouseMoveEvent(MouseEvent* e) override;
    virtual void mouseReleaseEvent(MouseEvent* e) override;
    virtual void wheelEvent(WheelEvent* e) override;

protected:
    double wrapAngle(double degree);
    void viewMotionRotate(int dx, int dy);
    void viewMotionTranslate(int dx, int dy);

    bool allowThrow_ = false;
    bool thrown_ = false;

    Eigen::Vector3d center_ = Eigen::Vector3d::Zero();
    double distance_ = 1.0;
    double heading_ = 0.0;
    double elevation_ = 0.0;

    double smoothAlpha_ = 1.0;
    Eigen::Vector3d prevCenter_ = Eigen::Vector3d::Zero();
    double prevDistance_ = 1.0;
    double prevHeading_ = 0.0;
    double prevElevation_ = 0.0;
};

} /* namespace ptgl */

#endif /* PTGL_CORE_SPHERICALCAMERA_H_ */
