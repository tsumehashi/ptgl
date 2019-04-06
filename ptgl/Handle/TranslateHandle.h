#ifndef PTGL_HANDLE_TRANSLATEHANDLE_H_
#define PTGL_HANDLE_TRANSLATEHANDLE_H_

#include <memory>
#include <array>
#include <functional>
#include "ptgl/Core/GraphicsItem.h"
#include "ptgl/Util/Transform.h"

namespace ptgl {
namespace handle {

class Translate1DHandle;
class Translate2DHandle;
class Translate3DHandle;
class TranslateHandle;
typedef std::shared_ptr<Translate1DHandle> Translate1DHandlePtr;
typedef std::shared_ptr<Translate2DHandle> Translate2DHandlePtr;
typedef std::shared_ptr<Translate3DHandle> Translate3DHandlePtr;
typedef std::shared_ptr<TranslateHandle> TranslateHandlePtr;

class TranslateHandle : public GraphicsItem {
public:
    TranslateHandle();
    virtual ~TranslateHandle();

    void setTransform(TransformPtr transform);
    void setHandleScale(double scale);
    void setAutoHandleScale(bool on);

    void setGlobalCoordinateMode(bool on);

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

    Translate1DHandlePtr xHandle_;
    Translate1DHandlePtr yHandle_;
    Translate1DHandlePtr zHandle_;

    Translate2DHandlePtr xyHandle_;
    Translate2DHandlePtr yzHandle_;
    Translate2DHandlePtr zxHandle_;

    Translate3DHandlePtr xyzHandle_;

    TransformPtr transform_;
    double scale_ = 1.0;
    bool autoScale_ = true;

    std::function<void ()> stateChangedStartFunc_;
    std::function<void ()> stateChangedFunc_;
    std::function<void ()> stateChangedEndFunc_;

    void slotPositionChangedStart(Eigen::Vector3d) {
        // emit
        if (stateChangedStartFunc_) {
            stateChangedStartFunc_();
        }
    }

    void slotPositionChanged(Eigen::Vector3d) {
        // emit
        if (stateChangedFunc_) {
            stateChangedFunc_();
        }
    }

    void slotPositionChangedEnd(Eigen::Vector3d) {
        // emit
        if (stateChangedEndFunc_) {
            stateChangedEndFunc_();
        }
    }
};

class Translate1DHandle : public GraphicsItem {
public:
    Translate1DHandle();
    virtual ~Translate1DHandle();

    void setTransform(TransformPtr transform);
    void setHandleScale(double scale) { scale_ = scale; }

    void setAxis(double x, double y, double z);

    const TransformPtr transform() const { return transform_; }

    void setGlobalCoordinateMode(bool on) { globalCoordinateMode_ = on; }

    void setColor(double r, double g, double b, double a = 1.0) {
        color_ = {{r, g, b, a}};
    }

    void setPickingColor(double r, double g, double b, double a = 1.0) {
        pickingColor_ = {{r, g, b, a}};
    }

    void setDragingColor(double r, double g, double b, double a = 1.0) {
        draggingColor_ = {{r, g, b, a}};
    }

    void setHoverColor(double r, double g, double b, double a = 1.0) {
        hoverColor_ = {{r, g, b, a}};
    }

    void setPositionChangedStartFunction(const std::function<void (Eigen::Vector3d)>& func) {
        positionChangedStartFunc_ = func;
    }

    void setPositionChangedFunction(const std::function<void (Eigen::Vector3d)>& func) {
        positionChangedFunc_ = func;
    }

    void setPositionChangedEndFunction(const std::function<void (Eigen::Vector3d)>& func) {
        positionChangedEndFunc_ = func;
    }
protected:

    // Hover event
    virtual void hoverEnterEvent(ptgl::GraphicsItemHoverEvent* e) override;
    virtual void hoverLeaveEvent(ptgl::GraphicsItemHoverEvent* e) override;
    virtual void hoverMoveEvent(ptgl::GraphicsItemHoverEvent* e) override;

    virtual void renderOverlayScene(ptgl::Renderer3D* r) override;

    virtual void renderPickingOverlayScene(ptgl::Renderer3D* r) override;

    virtual void mousePressEvent(ptgl::GraphicsItemMouseEvent* e) override;
    virtual void mouseMoveEvent(ptgl::GraphicsItemMouseEvent* e) override;
    virtual void mouseReleaseEvent(ptgl::GraphicsItemMouseEvent* e) override;


    TransformPtr transform_;
    double scale_ = 1.0;
    bool autoScale_ = true;

    bool globalCoordinateMode_ = false;

    double* drawColor_ = nullptr;
    std::array<double, 4> color_;
    std::array<double, 4> hoverColor_;
    std::array<double, 4> pickingColor_;
    std::array<double, 4> draggingColor_;

    Eigen::Vector3d axis_;
    double coneLength_ = 0.2;
    double coneRadius_ = 0.08;

    bool hoverd_ = false;
    bool dragged_ = false;

    Eigen::Vector3d cp_;
    Eigen::Matrix3d aixsR_;

    Eigen::Vector3d dcp_;

    std::function<void (Eigen::Vector3d)> positionChangedStartFunc_;
    std::function<void (Eigen::Vector3d)> positionChangedFunc_;
    std::function<void (Eigen::Vector3d)> positionChangedEndFunc_;
};

class Translate2DHandle : public GraphicsItem
{
public:

    enum PlaneType {
        XYPlane,
        YZPlane,
        ZXPlane,
    };

    Translate2DHandle();
    virtual ~Translate2DHandle();

    void setTransform(TransformPtr transform);
    void setPlaneType(PlaneType type);
    void setHandleScale(double scale) { scale_ = scale; }

    void setGlobalCoordinateMode(bool on) { globalCoordinateMode_ = on; }

    const TransformPtr transform() const { return transform_; }

    void setColor(double r, double g, double b, double a = 1.0) {
        color_ = {{r, g, b, a}};
    }

    void setPickingColor(double r, double g, double b, double a = 1.0) {
        pickingColor_ = {{r, g, b, a}};
    }

    void setDragingColor(double r, double g, double b, double a = 1.0) {
        draggingColor_ = {{r, g, b, a}};
    }

    void setHoverColor(double r, double g, double b, double a = 1.0) {
        hoverColor_ = {{r, g, b, a}};
    }

    void setPositionChangedStartFunction(const std::function<void (Eigen::Vector3d)>& func) {
        positionChangedStartFunc_ = func;
    }

    void setPositionChangedFunction(const std::function<void (Eigen::Vector3d)>& func) {
        positionChangedFunc_ = func;
    }

    void setPositionChangedEndFunction(const std::function<void (Eigen::Vector3d)>& func) {
        positionChangedEndFunc_ = func;
    }

protected:

    // Hover event
    virtual void hoverEnterEvent(ptgl::GraphicsItemHoverEvent* /*e*/) override;
    virtual void hoverLeaveEvent(ptgl::GraphicsItemHoverEvent* /*e*/) override;
    virtual void hoverMoveEvent(ptgl::GraphicsItemHoverEvent* /*e*/) override;

    virtual void renderOverlayScene(ptgl::Renderer3D* r) override;

    virtual void mousePressEvent(ptgl::GraphicsItemMouseEvent* e) override;
    virtual void mouseMoveEvent(ptgl::GraphicsItemMouseEvent* e) override;
    virtual void mouseReleaseEvent(ptgl::GraphicsItemMouseEvent* /*e*/) override;

    TransformPtr transform_;

    PlaneType planeType_ = XYPlane;

    double scale_ = 1.0;
    bool globalCoordinateMode_ = false;

    double* drawColor_ = nullptr;
    std::array<double, 4> color_;
    std::array<double, 4> hoverColor_;
    std::array<double, 4> pickingColor_;
    std::array<double, 4> draggingColor_;

    bool hoverd_ = false;
    bool dragged_ = false;

    double length_ = 1.0;

    Eigen::Vector3d axis1_;
    Eigen::Vector3d axis2_;

    Eigen::Vector3d preHp_;

    std::function<void (Eigen::Vector3d)> positionChangedStartFunc_;
    std::function<void (Eigen::Vector3d)> positionChangedFunc_;
    std::function<void (Eigen::Vector3d)> positionChangedEndFunc_;
};

class Translate3DHandle : public GraphicsItem
{
public:
    Translate3DHandle();
    virtual ~Translate3DHandle();

    void setTransform(TransformPtr transform);
    void setHandleScale(double scale) { scale_ = scale; }

    const TransformPtr transform() const { return transform_; }

    void setGlobalCoordinateMode(bool on) { globalCoordinateMode_ = on; }

    void setColor(double r, double g, double b, double a = 1.0) {
        color_ = {{r, g, b, a}};
    }

    void setPickingColor(double r, double g, double b, double a = 1.0) {
        pickingColor_ = {{r, g, b, a}};
    }

    void setDragingColor(double r, double g, double b, double a = 1.0) {
        draggingColor_ = {{r, g, b, a}};
    }

    void setHoverColor(double r, double g, double b, double a = 1.0) {
        hoverColor_ = {{r, g, b, a}};
    }

    void setPositionChangedStartFunction(const std::function<void (Eigen::Vector3d)>& func) {
        positionChangedStartFunc_ = func;
    }

    void setPositionChangedFunction(const std::function<void (Eigen::Vector3d)>& func) {
        positionChangedFunc_ = func;
    }

    void setPositionChangedEndFunction(const std::function<void (Eigen::Vector3d)>& func) {
        positionChangedEndFunc_ = func;
    }

protected:

    // Hover event
    virtual void hoverEnterEvent(ptgl::GraphicsItemHoverEvent* /*e*/) override;
    virtual void hoverLeaveEvent(ptgl::GraphicsItemHoverEvent* /*e*/) override;
    virtual void hoverMoveEvent(ptgl::GraphicsItemHoverEvent* /*e*/) override;

    virtual void renderOverlayScene(ptgl::Renderer3D* r) override;

    virtual void mousePressEvent(ptgl::GraphicsItemMouseEvent* e) override;
    virtual void mouseMoveEvent(ptgl::GraphicsItemMouseEvent* e) override;
    virtual void mouseReleaseEvent(ptgl::GraphicsItemMouseEvent* /*e*/) override;

    TransformPtr transform_;

    double scale_ = 1.0;
    bool globalCoordinateMode_ = false;

    double* drawColor_ = nullptr;
    std::array<double, 4> color_;
    std::array<double, 4> hoverColor_;
    std::array<double, 4> pickingColor_;
    std::array<double, 4> draggingColor_;

    bool hoverd_ = false;
    bool dragged_ = false;

    double length_ = 1.0;

    Eigen::Vector3d axis1_;
    Eigen::Vector3d axis2_;

    Eigen::Vector3d preHp_;
    Eigen::Matrix3d cR_;
    Eigen::Vector3d dhp_;

    std::function<void (Eigen::Vector3d)> positionChangedStartFunc_;
    std::function<void (Eigen::Vector3d)> positionChangedFunc_;
    std::function<void (Eigen::Vector3d)> positionChangedEndFunc_;
};

}
} /* namespace ptgl */

#endif /* PTGL_HANDLE_TRANSLATEHANDLE_H_ */
