#ifndef PTGL_UTIL_POSE_H_
#define PTGL_UTIL_POSE_H_

#include <memory>
#include <Eigen/Core>

namespace ptgl {

class Transform;
typedef std::shared_ptr<Transform> TransformPtr;

class Transform {
public:
    Transform() {}

    Transform(TransformPtr parent) :
        parent_(parent)
    {}

    Transform(const Eigen::Vector3d& p, const Eigen::Matrix3d& R) :
        mp_(p),
        mR_(R)
    {}

    virtual ~Transform() {}

    virtual void setParent(TransformPtr parent) {
        parent_ = parent;
    }

    TransformPtr parent() const { return parent_.lock(); }

    // global coordinate
    virtual void setPosition(const Eigen::Vector3d& p) {
        if (auto ptr = parent_.lock()) {
            Eigen::Vector3d tp(ptr->rotation().transpose() * (p - ptr->position()));
            localp_ = tp;
        }
        mp_ = p;
    }

    // global coordinate
    virtual void setRotation(const Eigen::Matrix3d& R) {
        if (auto ptr = parent_.lock()) {
            Eigen::Matrix3d tR(ptr->rotation().transpose().transpose() * R);
            localR_ = R;
        }
        mR_ = R;
    }

    // local coordinate
    virtual void setLocalPosition(const Eigen::Vector3d& tp) {
        localp_ = tp;
    }

    // local coordinate
    virtual void setLocalRotation(const Eigen::Matrix3d& tR) {
        localR_ = tR;
    }

    virtual const Eigen::Vector3d& position() const {
        if (auto ptr = parent_.lock()) {
            mp_ = ptr->position() + ptr->rotation() * localp_;
        }
        return mp_;
    }

    virtual const Eigen::Matrix3d& rotation() const {
        if (auto ptr = parent_.lock()) {
            mR_ =  ptr->rotation() * localR_;
        }
        return mR_;
    }

protected:
    std::weak_ptr<Transform> parent_;

    Eigen::Vector3d localp_ = Eigen::Vector3d::Zero();            // local coordinate
    Eigen::Matrix3d localR_ = Eigen::Matrix3d::Identity();        // local coordinate
    mutable Eigen::Vector3d mp_ = Eigen::Vector3d::Zero();        // global coordinate
    mutable Eigen::Matrix3d mR_ = Eigen::Matrix3d::Identity();    // global coordinate
};

} /* namespace ptgl */

#endif /* PTGL_UTIL_POSE_H_ */
