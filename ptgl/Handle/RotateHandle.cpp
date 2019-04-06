#include "RotateHandle.h"
#include "ptgl/Core/GraphicsView.h"
#include "ptgl/Util/Intersection.h"
#include "ptgl/Util/MathUtil.h"

namespace ptgl {
namespace handle {

RotateHandle::RotateHandle()
{
    setName("RotateHandle");
    transform_ = std::make_shared<Transform>();

    xHandle_ = std::make_shared<Rotate1DHandle>();
    yHandle_ = std::make_shared<Rotate1DHandle>();
    zHandle_ = std::make_shared<Rotate1DHandle>();

    // set axis
    xHandle_->setAxis(Rotate1DHandle::XAxis);
    yHandle_->setAxis(Rotate1DHandle::YAxis);
    zHandle_->setAxis(Rotate1DHandle::ZAxis);

    // set color
//    xColor_ = {{1.0, 0.2, 0.2, 1.0}};
//    yColor_ = {{0.2, 1.0, 0.2, 1.0}};
//    zColor_ = {{0.2, 0.2, 1.0, 1.0}};
    xColor_ = {{1.0, 0.2, 0.2, 0.8}};
    yColor_ = {{0.2, 1.0, 0.2, 0.8}};
    zColor_ = {{0.2, 0.2, 1.0, 0.8}};
    xHandle_->setColor(xColor_[0], xColor_[1], xColor_[2], xColor_[3]);
    yHandle_->setColor(yColor_[0], yColor_[1], yColor_[2], yColor_[3]);
    zHandle_->setColor(zColor_[0], zColor_[1], zColor_[2], zColor_[3]);

    // set hover color
    hoverColor_ = {{0.2, 0.9, 1.0, 0.9}};
    xHandle_->setHoverColor(hoverColor_[0], hoverColor_[1], hoverColor_[2], hoverColor_[3]);
    yHandle_->setHoverColor(hoverColor_[0], hoverColor_[1], hoverColor_[2], hoverColor_[3]);
    zHandle_->setHoverColor(hoverColor_[0], hoverColor_[1], hoverColor_[2], hoverColor_[3]);

    // set callback
    xHandle_->setRotationChangedStartFunction([&](Eigen::Matrix3d R){ this->slotRotationChangedStart(R); });
    xHandle_->setRotationChangedFunction([&](Eigen::Matrix3d R){ this->slotRotationChanged(R); });
    xHandle_->setRotationChangedEndFunction([&](Eigen::Matrix3d R){ this->slotRotationChangedEnd(R); });

    yHandle_->setRotationChangedStartFunction([&](Eigen::Matrix3d R){ this->slotRotationChangedStart(R); });
    yHandle_->setRotationChangedFunction([&](Eigen::Matrix3d R){ this->slotRotationChanged(R); });
    yHandle_->setRotationChangedEndFunction([&](Eigen::Matrix3d R){ this->slotRotationChangedEnd(R); });

    zHandle_->setRotationChangedStartFunction([&](Eigen::Matrix3d R){ this->slotRotationChangedStart(R); });
    zHandle_->setRotationChangedFunction([&](Eigen::Matrix3d R){ this->slotRotationChanged(R); });
    zHandle_->setRotationChangedEndFunction([&](Eigen::Matrix3d R){ this->slotRotationChangedEnd(R); });

    // set transform
    xHandle_->setTransform(transform_);
    yHandle_->setTransform(transform_);
    zHandle_->setTransform(transform_);

    addChild(xHandle_);
    addChild(yHandle_);
    addChild(zHandle_);
}

RotateHandle::~RotateHandle()
{

}

void RotateHandle::setTransform(TransformPtr transform)
{
    transform_ = transform;
    xHandle_->setTransform(transform);
    yHandle_->setTransform(transform);
    zHandle_->setTransform(transform);
}

void RotateHandle::setHandleScale(double scale)
{
    xHandle_->setHandleScale(scale);
    yHandle_->setHandleScale(scale);
    zHandle_->setHandleScale(scale);
}

void RotateHandle::setAutoHandleScale(bool on)
{
    xHandle_->setAutoHandleScale(on);
    yHandle_->setAutoHandleScale(on);
    zHandle_->setAutoHandleScale(on);
}

void RotateHandle::setGlobalCoordinateMode(bool on)
{
    xHandle_->setGlobalCoordinateMode(on);
    yHandle_->setGlobalCoordinateMode(on);
    zHandle_->setGlobalCoordinateMode(on);
}

void RotateHandle::prevProcess()
{
    Rotate1DHandlePtr draggedHandle = nullptr;
    if (xHandle_->dragged()) draggedHandle =  xHandle_;
    if (yHandle_->dragged()) draggedHandle =  yHandle_;
    if (zHandle_->dragged()) draggedHandle =  zHandle_;

    if (draggedHandle) {
        // set color
        xHandle_->setHoverColor(xColor_[0], xColor_[1], xColor_[2], xColor_[3]);
        yHandle_->setHoverColor(yColor_[0], yColor_[1], yColor_[2], yColor_[3]);
        zHandle_->setHoverColor(zColor_[0], zColor_[1], zColor_[2], zColor_[3]);

        draggedHandle->setHoverColor(hoverColor_[0], hoverColor_[1], hoverColor_[2], hoverColor_[3]);
    } else {
        // restore
        xHandle_->setHoverColor(hoverColor_[0], hoverColor_[1], hoverColor_[2], hoverColor_[3]);
        yHandle_->setHoverColor(hoverColor_[0], hoverColor_[1], hoverColor_[2], hoverColor_[3]);
        zHandle_->setHoverColor(hoverColor_[0], hoverColor_[1], hoverColor_[2], hoverColor_[3]);
    }
}

// Rotate1DHandle
Rotate1DHandle::Rotate1DHandle()
{
    setName("Rotate1DHandle");
    transform_ = std::make_shared<Transform>();
    drawColor_ = color_.data();

    color_ = {{1.0, 0.2, 0.2, 0.8}};    // default x color
    hoverColor_ = {{0.0, 0.8, 1.0, 1.0}};

    hoverd_ = false;
    dragged_ = false;

    scale_ = 1.0;

    axis_ << 1,0,0;

    radius_ = 0.7;

    axisR_.setIdentity();

    setAxis(XAxis);
}

Rotate1DHandle::~Rotate1DHandle()
{

}

void Rotate1DHandle::setTransform(TransformPtr transform)
{
    transform_ = transform;
    resetAngle();
}

void Rotate1DHandle::setHandleScale(double scale)
{
    scale_ = scale;
    renderScale_ = scale;
}

void Rotate1DHandle::setAutoHandleScale(bool on)
{
    autoScale_ = on;
}

void Rotate1DHandle::setAxis(AxisType type)
{
    axisType_ = type;
    if (type == XAxis) {
        axis_ << 1,0,0;
        paxis1_ << 0,1,0;
        paxis2_ << 0,0,1;
        color_ = {{1.0, 0.0, 0.0, 1.0}};
        axisR_ = rotationY(M_PI_2);
    } else if (type == YAxis) {
        axis_ << 0,1,0;
        paxis1_ << 1,0,0;
        paxis2_ << 0,0,1;
        color_ = {{0.0, 1.0, 0.0, 1.0}};
        axisR_ =  rotationX(M_PI_2);
    } else if (type == ZAxis) {
        axis_ << 0,0,1;
        paxis1_ << 0,1,0;
        paxis2_ << 1,0,0;
        color_ = {{0.0, 0.0, 1.0, 1.0}};
        axisR_.setIdentity();
    }

    resetAngle();
}

void Rotate1DHandle::resetAngle(double q)
{
    angle_ = q;
    preAngle_ = q;
}

// Hover event
void Rotate1DHandle::hoverEnterEvent(ptgl::GraphicsItemHoverEvent* /*e*/)
{
    drawColor_ = hoverColor_.data();
    hoverd_ = true;
}
void Rotate1DHandle::hoverLeaveEvent(ptgl::GraphicsItemHoverEvent* /*e*/)
{
    drawColor_ = color_.data();
    hoverd_ = false;
}

void Rotate1DHandle::hoverMoveEvent(ptgl::GraphicsItemHoverEvent* /*e*/)
{
    drawColor_ = hoverColor_.data();
}

void Rotate1DHandle::prevProcess()
{
    // set auto scale
    if (autoScale_) {
        Eigen::Vector3d dist(transform_->position() - this->graphicsWindow()->camera()->position());
        double d = dist.norm();
        if (d < 0.0001) d = 0.0001;
//        double scale = scale_ * 0.1*d;
        double scale = scale_ * 0.16*d;

        double w = this->viewHeight() / 480.0;    // h : 480
        double windowScale = 1.0 / w;
        scale *= windowScale;

        renderScale_ = scale;
    }
}

void Rotate1DHandle::renderOverlayScene(ptgl::Renderer3D* r)
{
    if (!transform_) return;

    const double ringWeight = 0.75;
    if (hoverd_ || dragged_) {
//            a = 0.4;
        drawColor_ = hoverColor_.data();
//        r->disableLighting();
    } else {
        drawColor_ = color_.data();
    }

//    r->setColor(drawColor_[0], drawColor_[1], drawColor_[2], 1);
    r->setColor(drawColor_[0], drawColor_[1], drawColor_[2], drawColor_[3]);

    double outer_radius = renderScale_*radius_;
    double inner_radius = renderScale_*radius_*ringWeight;

    Eigen::Matrix3d R;
    if (globalCoordinateMode_) {
        R = axisR_;
    } else {
        R = transform_->rotation() * axisR_;
    }
    r->setEffectLight(0.0);
    r->drawRingCircle(transform_->position().data(), R.data(), outer_radius, inner_radius);
    r->setEffectLight(1.0);
}

void Rotate1DHandle::mousePressEvent(ptgl::GraphicsItemMouseEvent* e)
{
    if (!transform_) return;
    dragged_ = true;
    const int x = e->x();
    const int y = e->y();

    Eigen::Vector3d cp(this->graphicsWindow()->camera()->position());
    Eigen::Vector3d wp(this->graphicsWindow()->camera()->unProject2D(x, y, 1.0));
    Eigen::Vector3d p1;
    Eigen::Vector3d p2;
    if (globalCoordinateMode_) {
        p1 = transform_->position() + paxis1_;
        p2 = transform_->position() + paxis2_;
    } else {
        p1 = transform_->position() + transform_->rotation() * paxis1_;
        p2 = transform_->position() + transform_->rotation() * paxis2_;
    }

    Eigen::Vector3d p = calcIntersectionLineAndPlane(cp, wp, transform_->position(), p1, p2);

    preP_ = p;
    preR_ = transform_->rotation();

    preAngle_ = angle_;

    // emit
    if (rotationChangedStartFunc_) {
        rotationChangedStartFunc_(transform_->rotation());
    }

    if (stateChangedStartFunc_) {
        stateChangedStartFunc_();
    }
}

void Rotate1DHandle::mouseMoveEvent(ptgl::GraphicsItemMouseEvent* e)
{
    if (!transform_) return;
    dragged_ = true;
    const int x = e->x();
    const int y = e->y();

    Eigen::Vector3d cp(this->graphicsWindow()->camera()->position());
    Eigen::Vector3d wp(this->graphicsWindow()->camera()->unProject2D(x, y, 1.0));

    Eigen::Vector3d p1;
    Eigen::Vector3d p2;
    if (globalCoordinateMode_) {
        p1 = transform_->position() + paxis1_;
        p2 = transform_->position() + paxis2_;
    } else {
        p1 = transform_->position() + preR_ * paxis1_;
        p2 = transform_->position() + preR_ * paxis2_;
    }
    Eigen::Vector3d p = calcIntersectionLineAndPlane(cp, wp, transform_->position(), p1, p2);
    Eigen::Vector3d from = preP_ - transform_->position();
    Eigen::Vector3d to = p - transform_->position();

    Eigen::Matrix3d dR = ptgl::rotFromTo(from, to);

    Eigen::Vector3d axis = transform_->rotation() * axis_;
    double dq = ptgl::angleFromAxisFromTo(from, to, axis);
    angle_ = dq + preAngle_;

    Eigen::Matrix3d R = dR * preR_;
    transform_->setRotation(R);

    // emit
    if (rotationChangedFunc_) {
        rotationChangedFunc_(R);
    }

    if (angleChangedFunc_) {
        angleChangedFunc_(angle_);
    }

    if (stateChangedFunc_) {
        stateChangedFunc_();
    }
}

void Rotate1DHandle::mouseReleaseEvent(ptgl::GraphicsItemMouseEvent* /*e*/) {
    dragged_ = false;

    // emit
    if (rotationChangedEndFunc_) {
        rotationChangedEndFunc_(transform_->rotation());
    }

    if (stateChangedEndFunc_) {
        stateChangedEndFunc_();
    }
}


}
} /* namespace ptgl */
