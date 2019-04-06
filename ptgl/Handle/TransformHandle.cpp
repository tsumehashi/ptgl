#include "TransformHandle.h"

namespace ptgl {
namespace handle {

TransformHandle::TransformHandle()
{
    transform_ = std::make_shared<Transform>();
    translateHandle_ = std::make_shared<TranslateHandle>();
    rotateHandle_ = std::make_shared<RotateHandle>();

    translateHandle_->setStateChangedStartFunction([&](){ this->slotStateChangedStart(); });
    translateHandle_->setStateChangedFunction([&](){ this->slotStateChanged(); });
    translateHandle_->setStateChangedEndFunction([&](){ this->slotStateChangedEnd(); });

    rotateHandle_->setStateChangedStartFunction([&](){ this->slotStateChangedStart(); });
    rotateHandle_->setStateChangedFunction([&](){ this->slotStateChanged(); });
    rotateHandle_->setStateChangedEndFunction([&](){ this->slotStateChangedEnd(); });

    translateHandle_->setTransform(transform_);
    rotateHandle_->setTransform(transform_);

    addChild(translateHandle_);
    addChild(rotateHandle_);
}

TransformHandle::~TransformHandle()
{

}

void TransformHandle::setTransform(TransformPtr transform)
{
    transform_ = transform;
    translateHandle_->setTransform(transform);
    rotateHandle_->setTransform(transform);
}

void TransformHandle::setHandleScale(double scale)
{
    translateHandle_->setHandleScale(scale);
    rotateHandle_->setHandleScale(scale);
}

void TransformHandle::setAutoHandleScale(bool on)
{
    translateHandle_->setAutoHandleScale(on);
    rotateHandle_->setAutoHandleScale(on);
}

void TransformHandle::setGlobalCoordinateMode(bool on)
{
    translateHandle_->setGlobalCoordinateMode(on);
    rotateHandle_->setGlobalCoordinateMode(on);
}

void TransformHandle::setEnableTranslate(bool on)
{
    translateHandle_->setEnabled(on);
}

void TransformHandle::setEnableRotate(bool on)
{
    rotateHandle_->setEnabled(on);
}

bool TransformHandle::isEnabledTranslate() const
{
    return translateHandle_->isEnabled();
}

bool TransformHandle::isEnabledRotate() const
{
    return rotateHandle_->isEnabled();
}

}
} /* namespace ptgl */
