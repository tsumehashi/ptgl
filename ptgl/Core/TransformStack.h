#ifndef RAPIDGRAPHICSLIBRARY_RGL_RGLCORE_TRANSFORMSTACK_H_
#define RAPIDGRAPHICSLIBRARY_RGL_RGLCORE_TRANSFORMSTACK_H_

#include <stack>
#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <Eigen/StdVector>

namespace ptgl {

class TransformStack {
public:
    TransformStack();
    virtual ~TransformStack();

    const Eigen::Affine3d& transformation() const;
    const Eigen::Matrix4d& matrix() const;

    Eigen::Vector3d getTranslation() const;
    Eigen::Matrix3d getRotation() const;
    Eigen::Vector3d getScale() const;

    size_t size() const;

    void clear();

    void push();
    void pop();

    void identiry();

    void scale(double s);
    void scale(double sx, double sy, double sz);
    void scale(const Eigen::Vector3d s);

    void translate(double x, double y, double z);
    void translate(const Eigen::Vector3d t);

    void rotate(const Eigen::Matrix3d& R);
    void rotate(double angle, const Eigen::Vector3d& axis);
    void rotateRpy(double r, double p, double y);
    void rotateX(double deg);
    void rotateY(double deg);
    void rotateZ(double deg);

    void transform(const Eigen::Affine3d& t);
    void transform(const Eigen::Matrix4d& m);
    void transform(const Eigen::Vector3d& p, const Eigen::Matrix3d& R);

private:
    typedef std::vector<Eigen::Affine3d, Eigen::aligned_allocator<Eigen::Affine3d>> StackContainer;
    typedef std::stack<Eigen::Affine3d, StackContainer> StackType;
    StackType stack_;

    Eigen::Affine3d& tf();
};

} /* namespace ptgl */

#endif /* RAPIDGRAPHICSLIBRARY_RGL_RGLCORE_TRANSFORMSTACK_H_ */
