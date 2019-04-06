#ifndef PTGL_CORE_CAMERA_H_
#define PTGL_CORE_CAMERA_H_

#include <memory>
#include <array>
#include <Eigen/Core>

namespace ptgl {

class MouseEvent;
class WheelEvent;

class Camera;
typedef std::shared_ptr<Camera> CameraPtr;

class Camera {
public:

    enum ViewMode {
        Perspective,
        Ortho,
    };

    Camera();
    virtual ~Camera();

    // view mode
    void setViewMode(ViewMode mode) { viewMode_ = mode; }
    ViewMode viewMode() const { return viewMode_; }

    // window size
    int width() const { return viewport_[2]; }
    int height() const { return viewport_[3]; }

    // perspective
    void setPerspective(double fovy, double aspect, double zNear, double zFar);
    double perspectiveFovy() const { return perspectiveFovy_; }
    double perspectiveAspect() const { return perspectiveAspect_; }
    double perspectiveZnear() const { return perspectiveZnear_; }
    double perspectiveZfar() const { return perspectiveZfar_; }

    // position
    void setLookAt(const Eigen::Vector3d& eye, const Eigen::Vector3d& center);
    void setPosition(const Eigen::Vector3d& p);
    void setRotation(const Eigen::Matrix3d& R);

    const Eigen::Vector3d& position() const { return p_; }
    const Eigen::Matrix3d& rotation() const { return R_; }
    const Eigen::Matrix3d& inverseRotation() const { return Rt_; }

    const Eigen::Vector3d& direction() const { return direction_; }

#if 0
    // mvp
    const Eigen::Matrix4d& projection() const { return projection_; }
    const Eigen::Matrix4d& modelview() const { return modelview_; }
    const Eigen::Vector4d& viewport() const { return viewport_; }

    // Projection * View * Model Matrix
    const Eigen::Matrix4d& mvp() const { return mvp_; }
#else
    // mvp
    const Eigen::Matrix<double, 4, 4, Eigen::DontAlign>& projection() const { return projection_; }
    const Eigen::Matrix<double, 4, 4, Eigen::DontAlign>& modelview() const { return modelview_; }
    const Eigen::Matrix<double, 4, 1, Eigen::DontAlign>& viewport() const { return viewport_; }

    // Projection * View * Model Matrix
    const Eigen::Matrix<double, 4, 4, Eigen::DontAlign>& mvp() const { return mvp_; }
#endif

#if 0    // buggy
    Eigen::Matrix3d getBillboardMatrix();
#endif

    // object -> window
    Eigen::Vector3d project(double x, double y, double z);

    // window -> object    z=[0.0, 1.0]
    Eigen::Vector3d unProject(double x, double y, double z);

    // object -> window
    Eigen::Vector3d project2D(double x, double y, double z);

    // window -> object    z=[0.0, 1.0]
    Eigen::Vector3d unProject2D(double x, double y, double z);

    virtual void initializeConfiguration();

    virtual void update();

    virtual void mousePressEvent(MouseEvent* e);
    virtual void mouseMoveEvent(MouseEvent* e);
    virtual void mouseReleaseEvent(MouseEvent* e);
    virtual void wheelEvent(WheelEvent* e);

    void updateViewport();

protected:
    ViewMode viewMode_  = ViewMode::Perspective;;

    Eigen::Vector3d p_ = Eigen::Vector3d::Zero();
    Eigen::Matrix3d R_ = Eigen::Matrix3d::Identity();
    Eigen::Matrix3d Rt_ = Eigen::Matrix3d::Identity();    // inverse R

    Eigen::Vector3d direction_ = Eigen::Vector3d(1,0,0);

    // perspective
    double perspectiveFovy_   = 45;     // perspective fovy (deg)
    double perspectiveAspect_ = 1.0;    // perspective aspect
    double perspectiveZnear_  = 0.01;   // perspective z near
    double perspectiveZfar_   = 100;    // perspective z far

#if 0
    Eigen::Matrix4d modelview_ = Eigen::Matrix4d::Identity();
    Eigen::Matrix4d projection_ = Eigen::Matrix4d::Identity();
    Eigen::Vector4d viewport_ = Eigen::Vector4d::Zero();

    Eigen::Matrix4d mvp_ = Eigen::Matrix4d::Identity();    // Projection * View * Model Matrix
#else
    Eigen::Matrix<double, 4, 4, Eigen::DontAlign> modelview_ = Eigen::Matrix4d::Identity();
    Eigen::Matrix<double, 4, 4, Eigen::DontAlign> projection_ = Eigen::Matrix4d::Identity();
    Eigen::Matrix<double, 4, 1, Eigen::DontAlign> viewport_ = Eigen::Vector4d::Zero();

    Eigen::Matrix<double, 4, 4, Eigen::DontAlign> mvp_ = Eigen::Matrix4d::Identity();    // Projection * View * Model Matrix
#endif
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

} /* namespace ptgl */

#endif /* PTGL_CORE_CAMERA_H_ */
