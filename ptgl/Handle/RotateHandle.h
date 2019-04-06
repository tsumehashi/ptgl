#ifndef PTGL_HANDLE_ROTATEHANDLE_H_
#define PTGL_HANDLE_ROTATEHANDLE_H_

#include <memory>
#include <array>
#include <functional>
#include "ptgl/Core/GraphicsItem.h"
#include "ptgl/Util/Transform.h"

namespace ptgl {
namespace handle {

class Rotate1DHandle;
class RotateHandle;
typedef std::shared_ptr<Rotate1DHandle> Rotate1DHandlePtr;
typedef std::shared_ptr<RotateHandle> RotateHandlePtr;

class RotateHandle : public GraphicsItem {
public:
    RotateHandle();
    virtual ~RotateHandle();

    void setTransform(TransformPtr transform);
    void setHandleScale(double scale);
    void setAutoHandleScale(bool on);

    void setGlobalCoordinateMode(bool on);

    Rotate1DHandlePtr xHandle() const { return xHandle_; }
    Rotate1DHandlePtr yHandle() const { return yHandle_; }
    Rotate1DHandlePtr zHandle() const { return zHandle_; }

    const TransformPtr transform() const { return transform_; }

    void setStateChangedStartFunction(const std::function<void ()>& func) {
        stateChangedStartFunc_ = func;
    }

    void setStateChangedFunction(const std::function<void ()>& func) {
        stateChangedFunc_ = func;
    }

    void setStateChangedEndFunction(const std::function<void ()>& func) {
        stateChangedEndFunc_ = func;
    }

protected:

    virtual void prevProcess() override;

    Rotate1DHandlePtr xHandle_;
    Rotate1DHandlePtr yHandle_;
    Rotate1DHandlePtr zHandle_;

    TransformPtr transform_;

    std::array<double, 4> xColor_;
    std::array<double, 4> yColor_;
    std::array<double, 4> zColor_;
    std::array<double, 4> hoverColor_;

    std::function<void ()> stateChangedStartFunc_;
    std::function<void ()> stateChangedFunc_;
    std::function<void ()> stateChangedEndFunc_;

    void slotRotationChangedStart(Eigen::Matrix3d) {
        // emit
        if (stateChangedStartFunc_) {
            stateChangedStartFunc_();
        }
    }

    void slotRotationChanged(Eigen::Matrix3d) {
        // emit
        if (stateChangedFunc_) {
            stateChangedFunc_();
        }
    }

    void slotRotationChangedEnd(Eigen::Matrix3d) {
        // emit
        if (stateChangedEndFunc_) {
            stateChangedEndFunc_();
        }
    }
};

class Rotate1DHandle : public GraphicsItem {
public:
    enum AxisType {
        XAxis,
        YAxis,
        ZAxis,
    };

    Rotate1DHandle();
    virtual ~Rotate1DHandle();

    void setTransform(TransformPtr transform);
    void setHandleScale(double scale);
    void setAutoHandleScale(bool on);

    void setGlobalCoordinateMode(bool on) { globalCoordinateMode_ = on; }

    void setAxis(AxisType type);

    const TransformPtr transform() const { return transform_; }

    void setColor(double r, double g, double b, double a = 1.0) {
        color_ = {{r, g, b, a}};
    }

    void setHoverColor(double r, double g, double b, double a = 1.0) {
        hoverColor_ = {{r, g, b, a}};
    }

    void setRotationChangedStartFunction(const std::function<void (Eigen::Matrix3d)>& func) {
        rotationChangedStartFunc_ = func;
    }

    void setRotationChangedFunction(const std::function<void (Eigen::Matrix3d)>& func) {
        rotationChangedFunc_ = func;
    }

    void setRotationChangedEndFunction(const std::function<void (Eigen::Matrix3d)>& func) {
        rotationChangedEndFunc_ = func;
    }

    void setAngleChangedFunction(const std::function<void (double)>& func) {
        angleChangedFunc_ = func;
    }

    void setStateChangedStartFunction(const std::function<void ()>& func) {
        stateChangedStartFunc_ = func;
    }

    void setStateChangedFunction(const std::function<void ()>& func) {
        stateChangedFunc_ = func;
    }

    void setStateChangedEndFunction(const std::function<void ()>& func) {
        stateChangedEndFunc_ = func;
    }

    bool hoverd() const { return hoverd_; }
    bool dragged() const { return dragged_; }

    double angle() const { return angle_; }
    void resetAngle(double q = 0.0);

protected:

    virtual void prevProcess() override;

    // render
    virtual void renderOverlayScene(ptgl::Renderer3D* r) override;

    // Hover event
    virtual void hoverEnterEvent(ptgl::GraphicsItemHoverEvent* /*e*/) override;
    virtual void hoverLeaveEvent(ptgl::GraphicsItemHoverEvent* /*e*/) override;
    virtual void hoverMoveEvent(ptgl::GraphicsItemHoverEvent* /*e*/) override;

    virtual void mousePressEvent(ptgl::GraphicsItemMouseEvent* e) override;
    virtual void mouseMoveEvent(ptgl::GraphicsItemMouseEvent* e) override;
    virtual void mouseReleaseEvent(ptgl::GraphicsItemMouseEvent* /*e*/) override;

    TransformPtr transform_;
    double scale_ = 1.0;
    double renderScale_ = 1.0;
    bool autoScale_ = true;

    bool globalCoordinateMode_ = false;

    double* drawColor_ = nullptr;
    std::array<double, 4> color_;
    std::array<double, 4> hoverColor_;

    bool hoverd_ = false;
    bool dragged_ = false;

    AxisType axisType_ = ZAxis;
    Eigen::Vector3d axis_;

    Eigen::Vector3d paxis1_;
    Eigen::Vector3d paxis2_;

    double radius_;

    Eigen::Vector3d preP_;
    Eigen::Matrix3d preR_;

    double angle_ = 0.0;
    double preAngle_ = 0.0;

    Eigen::Matrix3d axisR_;

    std::function<void (Eigen::Matrix3d)> rotationChangedStartFunc_;
    std::function<void (Eigen::Matrix3d)> rotationChangedFunc_;
    std::function<void (Eigen::Matrix3d)> rotationChangedEndFunc_;

    std::function<void (double)> angleChangedFunc_;

    std::function<void ()> stateChangedStartFunc_;
    std::function<void ()> stateChangedFunc_;
    std::function<void ()> stateChangedEndFunc_;
};

}
} /* namespace ptgl */

#endif /* PTGL_HANDLE_ROTATEHANDLE_H_ */
