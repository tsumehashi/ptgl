#include "ptgl/Plot/Xy.h"
#include <stdexcept>
#include <utility>

namespace ptgl::plot {
XYBuffer::XYBuffer(std::size_t capacity) : points_(capacity) {
    if (!capacity) throw std::invalid_argument("ptgl::plot: XY capacity must be positive");
}
XYBuffer::XYBuffer(std::vector<XYPoint> points) : points_(std::move(points)), size_(points_.size()) {
    if (points_.empty()) points_.resize(1);
}
void XYBuffer::append(XYPoint point) noexcept {
    points_[next_] = point;
    if (size_ < points_.size()) ++size_; else ++overwritten_;
    next_ = (next_ + 1) % points_.size();
}
XYPoint XYBuffer::sample(std::size_t index) const {
    if (index >= size_) throw std::out_of_range("ptgl::plot: XY sample index");
    return points_[((size_ == points_.size() ? next_ : 0) + index) % points_.size()];
}
} // namespace ptgl::plot
