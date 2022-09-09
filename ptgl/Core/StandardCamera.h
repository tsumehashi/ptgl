#ifndef PTGL_CORE_STANDARDCAMERA_H_
#define PTGL_CORE_STANDARDCAMERA_H_

#include <array>
#include "Camera.h"

namespace ptgl {

class MouseEvent;
class WheelEvent;

class StandardCamera;
typedef std::shared_ptr<StandardCamera> StandardCameraPtr;

class StandardCamera : public Camera {
public:
    StandardCamera();
    virtual ~StandardCamera();

    void set(double x, double y, double z, double h, double p, double r);

    virtual void update() override;

    virtual void mousePressEvent(MouseEvent* e) override;
    virtual void mouseMoveEvent(MouseEvent* e) override;
    virtual void mouseReleaseEvent(MouseEvent* e) override;
    virtual void wheelEvent(WheelEvent* e) override;

    void setScrollScale(double s);
    double scrollScale() const { return scrollScale_; }

    void setMoveScale(double s);
    double moveScale() const { return moveScale_; }

    void setDirectionScale(double s);
    double directionScale() const { return directionScale_; }

protected:
    void setCamera();
    void setCamera(double x, double y, double z, double h, double p, double r);

    void wrapCameraAngles();
    void viewMotion1(double dx, double dy);
    void viewMotion2(double dx, double dy);
    void viewMotion3(double dx, double dy);

    int px_ = 0;
    int py_ = 0;

    int dx_ = 0;
    int dy_ = 0;

    bool allowThrow_ = false;
    bool thrown_ = false;
    int motion_ = 0;

    double scrollScale_ = 1.0;
    double moveScale_ = 1.0;
    double directionScale_ = 1.0;

    Eigen::Vector3d view_hpr_ = Eigen::Vector3d::Zero();    // heading(yaw), pitch, roll (degrees)
    Eigen::Vector3d view_xyz_ = Eigen::Vector3d::Zero();    // position x,y,z
};

} /* namespace ptgl */

#endif /* PTGL_CORE_STANDARDCAMERA_H_ */
