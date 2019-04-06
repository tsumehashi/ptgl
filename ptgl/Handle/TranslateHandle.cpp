#include "TranslateHandle.h"
#include "ptgl/Util/MathUtil.h"
#include "ptgl/Util/Intersection.h"
#include "ptgl/Core/GraphicsView.h"

namespace ptgl {
namespace handle {

TranslateHandle::TranslateHandle() {

    setName("TranslateHandle");

    transform_ = std::make_shared<Transform>();

    scale_ = 1.0;
    autoScale_ = true;

    xHandle_ = std::make_shared<Translate1DHandle>();
    yHandle_ = std::make_shared<Translate1DHandle>();
    zHandle_ = std::make_shared<Translate1DHandle>();

    xHandle_->setAxis(1,0,0);
    yHandle_->setAxis(0,1,0);
    zHandle_->setAxis(0,0,1);

    xHandle_->setColor(1,0,0);
    yHandle_->setColor(0,1,0);
    zHandle_->setColor(0,0,1);

    // set hover color
    xHandle_->setHoverColor(0.0,0.8,1);
    yHandle_->setHoverColor(0.0,0.8,1);
    zHandle_->setHoverColor(0.0,0.8,1);

    addChild(xHandle_);
    addChild(yHandle_);
    addChild(zHandle_);

    // set 2D Handle
    xyHandle_ = std::make_shared<Translate2DHandle>();
    yzHandle_ = std::make_shared<Translate2DHandle>();
    zxHandle_ = std::make_shared<Translate2DHandle>();

    xyHandle_->setPlaneType(Translate2DHandle::XYPlane);
    yzHandle_->setPlaneType(Translate2DHandle::YZPlane);
    zxHandle_->setPlaneType(Translate2DHandle::ZXPlane);

    xyHandle_->setColor(1,1,0);
    yzHandle_->setColor(1,1,0);
    zxHandle_->setColor(1,1,0);

    // set hover color
    xyHandle_->setHoverColor(0.0,0.8,1);
    yzHandle_->setHoverColor(0.0,0.8,1);
    zxHandle_->setHoverColor(0.0,0.8,1);

    addChild(xyHandle_);
    addChild(yzHandle_);
    addChild(zxHandle_);

    // set 3D handle
    xyzHandle_ = std::make_shared<ptgl::handle::Translate3DHandle>();
    xyzHandle_->setColor(1,1,0);

    xyzHandle_->setHoverColor(0.0,0.8,1);

    addChild(xyzHandle_);

    // set signal slot
    xHandle_->setPositionChangedStartFunction([&](Eigen::Vector3d p){ this->slotPositionChangedStart(p); });
    xHandle_->setPositionChangedFunction([&](Eigen::Vector3d p){ this->slotPositionChanged(p); });
    xHandle_->setPositionChangedEndFunction([&](Eigen::Vector3d p){ this->slotPositionChangedEnd(p); });
    yHandle_->setPositionChangedStartFunction([&](Eigen::Vector3d p){ this->slotPositionChangedStart(p); });
    yHandle_->setPositionChangedFunction([&](Eigen::Vector3d p){ this->slotPositionChanged(p); });
    yHandle_->setPositionChangedEndFunction([&](Eigen::Vector3d p){ this->slotPositionChangedEnd(p); });
    zHandle_->setPositionChangedStartFunction([&](Eigen::Vector3d p){ this->slotPositionChangedStart(p); });
    zHandle_->setPositionChangedFunction([&](Eigen::Vector3d p){ this->slotPositionChanged(p); });
    zHandle_->setPositionChangedEndFunction([&](Eigen::Vector3d p){ this->slotPositionChangedEnd(p); });

    xyHandle_->setPositionChangedStartFunction([&](Eigen::Vector3d p){ this->slotPositionChangedStart(p); });
    xyHandle_->setPositionChangedFunction([&](Eigen::Vector3d p){ this->slotPositionChanged(p); });
    xyHandle_->setPositionChangedEndFunction([&](Eigen::Vector3d p){ this->slotPositionChangedEnd(p); });
    yzHandle_->setPositionChangedStartFunction([&](Eigen::Vector3d p){ this->slotPositionChangedStart(p); });
    yzHandle_->setPositionChangedFunction([&](Eigen::Vector3d p){ this->slotPositionChanged(p); });
    yzHandle_->setPositionChangedEndFunction([&](Eigen::Vector3d p){ this->slotPositionChangedEnd(p); });
    zxHandle_->setPositionChangedStartFunction([&](Eigen::Vector3d p){ this->slotPositionChangedStart(p); });
    zxHandle_->setPositionChangedFunction([&](Eigen::Vector3d p){ this->slotPositionChanged(p); });
    zxHandle_->setPositionChangedEndFunction([&](Eigen::Vector3d p){ this->slotPositionChangedEnd(p); });

    xyzHandle_->setPositionChangedStartFunction([&](Eigen::Vector3d p){ this->slotPositionChangedStart(p); });
    xyzHandle_->setPositionChangedFunction([&](Eigen::Vector3d p){ this->slotPositionChanged(p); });
    xyzHandle_->setPositionChangedEndFunction([&](Eigen::Vector3d p){ this->slotPositionChangedEnd(p); });

    // set transform
    xHandle_->setTransform(transform_);
    yHandle_->setTransform(transform_);
    zHandle_->setTransform(transform_);

    xyHandle_->setTransform(transform_);
    yzHandle_->setTransform(transform_);
    zxHandle_->setTransform(transform_);

    xyzHandle_->setTransform(transform_);
}

TranslateHandle::~TranslateHandle()
{


}

void TranslateHandle::setTransform(TransformPtr transform) {
    transform_ = transform;
    xHandle_->setTransform(transform);
    yHandle_->setTransform(transform);
    zHandle_->setTransform(transform);

    xyHandle_->setTransform(transform);
    yzHandle_->setTransform(transform);
    zxHandle_->setTransform(transform);

    xyzHandle_->setTransform(transform);
}

void TranslateHandle::setHandleScale(double scale) {
    scale_ = scale;
    xHandle_->setHandleScale(scale);
    yHandle_->setHandleScale(scale);
    zHandle_->setHandleScale(scale);

    xyHandle_->setHandleScale(scale);
    yzHandle_->setHandleScale(scale);
    zxHandle_->setHandleScale(scale);

    xyzHandle_->setHandleScale(scale);
}

void TranslateHandle::setAutoHandleScale(bool on) {
    autoScale_ = on;
}

void TranslateHandle::setGlobalCoordinateMode(bool on)
{
    xHandle_->setGlobalCoordinateMode(on);
    yHandle_->setGlobalCoordinateMode(on);
    zHandle_->setGlobalCoordinateMode(on);

    xyHandle_->setGlobalCoordinateMode(on);
    yzHandle_->setGlobalCoordinateMode(on);
    zxHandle_->setGlobalCoordinateMode(on);

    xyzHandle_->setGlobalCoordinateMode(on);
}

void TranslateHandle::prevProcess()
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

        xHandle_->setHandleScale(scale);
        yHandle_->setHandleScale(scale);
        zHandle_->setHandleScale(scale);

        xyHandle_->setHandleScale(scale);
        yzHandle_->setHandleScale(scale);
        zxHandle_->setHandleScale(scale);

        xyzHandle_->setHandleScale(scale);
    }
}

// Translate1DHandle
Translate1DHandle::Translate1DHandle()
{
    setName("Translate1DHandle");
    transform_ = std::make_shared<Transform>();
    drawColor_ = color_.data();

    scale_ = 1.0;

    hoverd_ = false;
    dragged_ = false;

    aixsR_.setIdentity();
}

Translate1DHandle::~Translate1DHandle()
{

}

void Translate1DHandle::setTransform(TransformPtr transform)
{
    transform_ = transform;
}

void Translate1DHandle::setAxis(double x, double y, double z)
{
    axis_ << x, y, z;
    Eigen::Vector3d n(axis_.normalized());
    aixsR_ = rotFromRpy(double(-M_PI_2*n(1)), double(M_PI_2*n(0)), double(M_PI_2*n(2)));
    if (x >= 0.5) {
        setName("Translate1DHandleX");
    } else if (y >= 0.5) {
        setName("Translate1DHandleY");
    } else {
        setName("Translate1DHandleZ");
    }
}

// Hover event
void Translate1DHandle::hoverEnterEvent(ptgl::GraphicsItemHoverEvent* /*e*/)
{
    drawColor_ = hoverColor_.data();
    hoverd_ = true;
}

void Translate1DHandle::hoverLeaveEvent(ptgl::GraphicsItemHoverEvent* /*e*/)
{
    drawColor_ = color_.data();
    hoverd_ = false;
}

void Translate1DHandle::hoverMoveEvent(ptgl::GraphicsItemHoverEvent* /*e*/)
{
    drawColor_ = hoverColor_.data();
}

void Translate1DHandle::renderPickingOverlayScene(ptgl::Renderer3D* r)
{
    if (!transform_) return;

    double length = scale_ * coneLength_;
    double rasius = scale_ * coneRadius_;
    Eigen::Vector3d p;
    Eigen::Matrix3d R;
    if (globalCoordinateMode_) {
        p = transform_->position() + (scale_ * (1.0 - coneLength_) * axis_);
        R = aixsR_;
    } else {
        p = transform_->position() + (scale_ * (1.0 - coneLength_) * transform_->rotation() * axis_);
        R = transform_->rotation() * aixsR_;
    }
    r->drawCone(p.data(), R.data(), length, rasius);
}

void Translate1DHandle::renderOverlayScene(ptgl::Renderer3D* r)
{
    if (!transform_) return;

    double length = scale_ * coneLength_;
    double rasius = scale_ * coneRadius_;
    Eigen::Vector3d p;
    Eigen::Matrix3d R;
    if (globalCoordinateMode_) {
        p = transform_->position() + (scale_ * (1.0 - coneLength_) * axis_);
        R = aixsR_;
    } else {
        p = transform_->position() + (scale_ * (1.0 - coneLength_) * transform_->rotation() * axis_);
        R = transform_->rotation() * aixsR_;
    }
    cp_ = p;

    r->setColor(1,1,1);
    r->setLineWidth(2);
    r->drawLine(transform_->position().data(), p.data());

    double a = 1.0;
    if (hoverd_ || dragged_) {
        drawColor_ = hoverColor_.data();
        r->setEffectLight(0);    // disable lighting
    } else {
        drawColor_ = color_.data();
    }

    r->setColor(drawColor_[0], drawColor_[1], drawColor_[2], a);

    r->drawCone(p.data(), R.data(), length, rasius);
    r->setEffectLight(1);    // enable lighting

}

void Translate1DHandle::mousePressEvent(ptgl::GraphicsItemMouseEvent* e)
{
    dragged_ = true;
    if (!transform_) return;

    const int x = e->x();
    const int y = e->y();

    const Eigen::Vector3d& line1_p1 = transform_->position();
    const Eigen::Vector3d& line1_p2 = cp_;
    Eigen::Vector3d line2_p1 = graphicsWindow()->camera()->position();
    Eigen::Vector3d line2_p2 = graphicsWindow()->camera()->unProject2D(x, y, 1.0);    // far
    Eigen::Vector3d np1, np2;
    if (calcIntersectionLineAndLine(np1, np2, line1_p1, line1_p2, line2_p1, line2_p2)) {
        dcp_ = cp_ - np1;

        if (positionChangedStartFunc_) {
            positionChangedStartFunc_(transform_->position());
        }
    }
}

void Translate1DHandle::mouseMoveEvent(ptgl::GraphicsItemMouseEvent* e)
{
    if (!transform_) return;
    const int x = e->x();
    const int y = e->y();

    const Eigen::Vector3d& line1_p1 = transform_->position();
    const Eigen::Vector3d& line1_p2 = cp_;
    Eigen::Vector3d line2_p1 = graphicsWindow()->camera()->position();
    Eigen::Vector3d line2_p2 = graphicsWindow()->camera()->unProject2D(x, y, 1.0);    // far
    Eigen::Vector3d np1, np2;
    if (calcIntersectionLineAndLine(np1, np2, line1_p1, line1_p2, line2_p1, line2_p2)) {
        Eigen::Vector3d dp;
        dp = np1 - cp_;

        transform_->setPosition(Eigen::Vector3d(transform_->position() + dp + dcp_));

        // emit
//        sigPositionChanged_(transform_->position());
        if (positionChangedFunc_) {
            positionChangedFunc_(transform_->position());
        }
    }

}

void Translate1DHandle::mouseReleaseEvent(ptgl::GraphicsItemMouseEvent* /*e*/)
{
    drawColor_ = color_.data();
    dragged_ = false;

    if (positionChangedEndFunc_) {
        positionChangedEndFunc_(transform_->position());
    }
}

//
Translate2DHandle::Translate2DHandle()
{
    setName("Translate2DHandle");
    transform_ = std::make_shared<Transform>();
    drawColor_ = color_.data();
    scale_ = 1.0;

    axis1_ << 1,0,0;
    axis2_ << 0,1,0;

    hoverd_ = false;
    dragged_ = false;

    length_ = 0.2;
}

Translate2DHandle::~Translate2DHandle()
{

}

void Translate2DHandle::setTransform(TransformPtr transform)
{
    transform_ = transform;
}

void Translate2DHandle::setPlaneType(PlaneType type)
{
    planeType_ = type;
    if (planeType_ == XYPlane) {
        axis1_ << 1,0,0;
        axis2_ << 0,1,0;
        setName("Translate2DHandleXY");
    } else if (planeType_ == YZPlane) {
        axis1_ << 0,1,0;
        axis2_ << 0,0,1;
        setName("Translate2DHandleYZ");
    } else if (planeType_ == ZXPlane) {
        axis1_ << 0,0,1;
        axis2_ << 1,0,0;
        setName("Translate2DHandleZX");
    }
}

// Hover event
void Translate2DHandle::hoverEnterEvent(ptgl::GraphicsItemHoverEvent* /*e*/)
{
    hoverd_ = true;
}

void Translate2DHandle::hoverLeaveEvent(ptgl::GraphicsItemHoverEvent* /*e*/)
{
    hoverd_ = false;
}

void Translate2DHandle::hoverMoveEvent(ptgl::GraphicsItemHoverEvent* /*e*/)
{

}

void Translate2DHandle::renderOverlayScene(ptgl::Renderer3D* r)
{
    if (!transform_) return;

    Eigen::Vector3d p1, p2, p3;

    double length = scale_ * length_;

    if (globalCoordinateMode_) {
        p1 = transform_->position() + length * axis1_;
        p2 = transform_->position() + length * axis2_;
        p3 = transform_->position() + length * (axis1_ + axis2_);
    } else {
        p1 = transform_->position() + length * transform_->rotation() * axis1_;
        p2 = transform_->position() + length * transform_->rotation() * axis2_;
        p3 = transform_->position() + length * transform_->rotation() * (axis1_ + axis2_);
    }

    double a = 0.1;
    drawColor_ = color_.data();
    if (hoverd_ || dragged_) {
        a = 0.4;
        drawColor_ = hoverColor_.data();
    }

    // draw line
    r->setLineWidth(2);
    r->setColor(drawColor_[0], drawColor_[1], drawColor_[2], drawColor_[3]);
    r->drawLine(p1.data(), p3.data());
    r->drawLine(p2.data(), p3.data());

    // draw plane
    r->setColor(drawColor_[0], drawColor_[1], drawColor_[2], a);

    // todo optimize
    {
        r->pushMatrix();
        if (globalCoordinateMode_) {
            r->translate(transform_->position());
        } else {
            r->transform(transform_->position(), transform_->rotation());
        }
        if (planeType_ == XYPlane) {
            ;    // do nothing
        } else if (planeType_ == YZPlane) {
            r->rotate(-M_PI/2, Eigen::Vector3d::UnitY());
        } else if (planeType_ == ZXPlane) {
            r->rotate(M_PI/2, Eigen::Vector3d::UnitX());
        }
        r->drawRect(r->p(length/2,length/2,0), r->R0(), length, length);
        r->popMatrix();
    }
}

void Translate2DHandle::mousePressEvent(ptgl::GraphicsItemMouseEvent* e)
{
    dragged_ = true;
    if (!transform_) return;

    const int x = e->x();
    const int y = e->y();

    double length = scale_ * length_;

    const Eigen::Vector3d& p0 = transform_->position();
    Eigen::Vector3d p1, p2, p3;
//        p3 = p1 + p2;

    if (globalCoordinateMode_) {
        p1 = transform_->position() + length * axis1_;
        p2 = transform_->position() + length * axis2_;
        p3 = transform_->position() + length * (axis1_ + axis2_);
    } else {
        p1 = transform_->position() + length * transform_->rotation() * axis1_;
        p2 = transform_->position() + length * transform_->rotation() * axis2_;
        p3 = transform_->position() + length * transform_->rotation() * (axis1_ + axis2_);
    }

    Eigen::Vector3d cp(graphicsWindow()->camera()->position());
    Eigen::Vector3d wp(graphicsWindow()->camera()->unProject2D(x, y, 1.0));    // far

    preHp_ = calcIntersectionLineAndPlane(cp, wp, p1, p2, p3);
    preHp_ -= p0;

    if (positionChangedStartFunc_) {
        positionChangedStartFunc_(transform_->position());
    }
}

void Translate2DHandle::mouseMoveEvent(ptgl::GraphicsItemMouseEvent* e)
{
    if (!transform_) return;

    const int x = e->x();
    const int y = e->y();

    double length = scale_ * length_;

    Eigen::Vector3d p1, p2, p3;
    if (globalCoordinateMode_) {
        p1 = transform_->position() + length * axis1_;
        p2 = transform_->position() + length * axis2_;
        p3 = transform_->position() + length * (axis1_ + axis2_);
    } else {
        p1 = transform_->position() + length * transform_->rotation() * axis1_;
        p2 = transform_->position() + length * transform_->rotation() * axis2_;
        p3 = transform_->position() + length * transform_->rotation() * (axis1_ + axis2_);
    }

    Eigen::Vector3d cp(graphicsWindow()->camera()->position());
    Eigen::Vector3d wp(graphicsWindow()->camera()->unProject2D(x, y, 1.0));    // far

    Eigen::Vector3d hp;
    hp = calcIntersectionLineAndPlane(cp, wp, p1, p2, p3);

    Eigen::Vector3d dhp;
    dhp = hp - preHp_;

    transform_->setPosition(dhp);

    // emit
    if (positionChangedFunc_) {
        positionChangedFunc_(transform_->position());
    }
}

void Translate2DHandle::mouseReleaseEvent(ptgl::GraphicsItemMouseEvent* /*e*/)
{
    dragged_ = false;

    if (positionChangedEndFunc_) {
        positionChangedEndFunc_(transform_->position());
    }
}

// Translate3DHandle
Translate3DHandle::Translate3DHandle()
{
    setName("Translate3DHandle");
    transform_ = std::make_shared<Transform>();
    drawColor_ = color_.data();
    scale_ = 1.0;

    hoverd_ = false;
    dragged_ = false;

    length_ = 0.15;

    axis1_ << 1,0,0;
    axis2_ << 0,-1,0;

    cR_.setIdentity();
}

Translate3DHandle::~Translate3DHandle()
{

}

void Translate3DHandle::setTransform(TransformPtr transform)
{
    transform_ = transform;
}

// Hover event
void Translate3DHandle::hoverEnterEvent(ptgl::GraphicsItemHoverEvent* /*e*/)
{
    drawColor_ = hoverColor_.data();
    hoverd_ = true;
}
void Translate3DHandle::hoverLeaveEvent(ptgl::GraphicsItemHoverEvent* /*e*/)
{
    drawColor_ = color_.data();
    hoverd_ = false;
}

void Translate3DHandle::hoverMoveEvent(ptgl::GraphicsItemHoverEvent* /*e*/)
{
    drawColor_ = hoverColor_.data();
}

void Translate3DHandle::renderOverlayScene(ptgl::Renderer3D* r)
{
    if (!transform_) return;

    double length = scale_ * length_;

    const double sides[] = {length, length, length};
    double a = 0;
    if (hoverd_ || dragged_) {
        drawColor_ = hoverColor_.data();
    } else {
        drawColor_ = color_.data();
    }

    a = drawColor_[3];
    r->setColor(drawColor_[0], drawColor_[1], drawColor_[2], a);
    if (globalCoordinateMode_) {
        r->drawBox(transform_->position().data(), r->R0(), sides);
    } else {
        r->drawBox(transform_->position().data(), transform_->rotation().data(), sides);
    }
}


void Translate3DHandle::mousePressEvent(ptgl::GraphicsItemMouseEvent* e)
{
    dragged_ = true;
    if (!transform_) return;

    const int x = e->x();
    const int y = e->y();

    Eigen::Vector3d camera_p = graphicsWindow()->camera()->position();
    Eigen::Vector3d center_up = graphicsWindow()->camera()->unProject2D(graphicsWindow()->width()/2.0, graphicsWindow()->height()/2.0, 1.0);    // far
    Eigen::Vector3d plane_norm = camera_p - center_up;
    plane_norm.normalize();
    Eigen::Vector3d plane_p = ptgl::calcIntersectionPointAndLine(transform_->position(), camera_p, center_up);
    Eigen::Vector3d up = graphicsWindow()->camera()->unProject2D(x, y, 1.0);    // far
    Eigen::Vector3d plane_up = ptgl::calcIntersectionLineAndPlane(up, camera_p, plane_p, plane_norm);

    preHp_ = plane_up;
    dhp_ = preHp_ - transform_->position();

    if (positionChangedStartFunc_) {
        positionChangedStartFunc_(transform_->position());
    }
}

void Translate3DHandle::mouseMoveEvent(ptgl::GraphicsItemMouseEvent* e)
{
    dragged_ = true;
    if (!transform_) return;

    const int x = e->x();
    const int y = e->y();

    Eigen::Vector3d camera_p = graphicsWindow()->camera()->position();
    Eigen::Vector3d center_up = graphicsWindow()->camera()->unProject2D(graphicsWindow()->width()/2.0, graphicsWindow()->height()/2.0, 1.0);    // far
    Eigen::Vector3d plane_norm = camera_p - center_up;
    plane_norm.normalize();
    Eigen::Vector3d plane_p = ptgl::calcIntersectionPointAndLine(transform_->position(), camera_p, center_up);
    Eigen::Vector3d up = graphicsWindow()->camera()->unProject2D(x, y, 1.0);    // far
    Eigen::Vector3d plane_up = ptgl::calcIntersectionLineAndPlane(up, camera_p, plane_p, plane_norm);

//    transform_->setPosition(Eigen::Vector3d(plane_up - dhp_));
    transform_->setPosition(Eigen::Vector3d(plane_up));

    // emit
    if (positionChangedFunc_) {
        positionChangedFunc_(transform_->position());
    }
}

void Translate3DHandle::mouseReleaseEvent(ptgl::GraphicsItemMouseEvent* /*e*/)
{
    dragged_ = false;

    if (positionChangedEndFunc_) {
        positionChangedEndFunc_(transform_->position());
    }
}

}
} /* namespace ptgl */
