#ifndef PTGL_HANDLE_TRANSFORMHANDLE_H_
#define PTGL_HANDLE_TRANSFORMHANDLE_H_

#include "ptgl/Core/GraphicsItem.h"
#include "ptgl/Util/Transform.h"
#include "TranslateHandle.h"
#include "RotateHandle.h"

namespace ptgl {
namespace handle {

class TransformHandle;
typedef std::shared_ptr<TransformHandle> TransformHandlePtr;

class TransformHandle : public GraphicsItem {
public:
    TransformHandle();
    virtual ~TransformHandle();

    void setTransform(TransformPtr transform);
    void setHandleScale(double scale);
    void setAutoHandleScale(bool on);

    void setGlobalCoordinateMode(bool on);
    void setEnableTranslate(bool on);
    void setEnableRotate(bool on);

    bool isEnabledTranslate() const;
    bool isEnabledRotate() const;

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
    TransformPtr transform_;

    TranslateHandlePtr translateHandle_;
    RotateHandlePtr rotateHandle_;

    std::function<void ()> stateChangedStartFunc_;
    std::function<void ()> stateChangedFunc_;
    std::function<void ()> stateChangedEndFunc_;

    void slotStateChangedStart() {
        // emit
        if (stateChangedStartFunc_) {
            stateChangedStartFunc_();
        }
    }

    void slotStateChanged() {
        // emit
        if (stateChangedFunc_) {
            stateChangedFunc_();
        }
    }

    void slotStateChangedEnd() {
        // emit
        if (stateChangedEndFunc_) {
            stateChangedEndFunc_();
        }
    }
};

}
} /* namespace ptgl */

#endif /* PTGL_HANDLE_TRANSFORMHANDLE_H_ */
