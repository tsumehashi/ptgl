#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ptgl::plot {
struct XYPoint {
    double x = 0, y = 0;
    bool break_before = false;
};
// Ordered XY points. X need not increase: loops and reversals keep input order.
// Fixed-capacity ring, one owner thread. Non-finite coordinates mark gaps.
class XYBuffer {
public:
    explicit XYBuffer(std::size_t capacity);
    explicit XYBuffer(std::vector<XYPoint> points);
    void append(XYPoint point) noexcept;
    void clear() noexcept { size_ = next_ = 0; overwritten_ = 0; }
    std::size_t size() const noexcept { return size_; }
    std::size_t capacity() const noexcept { return points_.size(); }
    std::uint64_t overwritten() const noexcept { return overwritten_; }
    XYPoint sample(std::size_t chronological_index) const;
private:
    std::vector<XYPoint> points_;
    std::size_t size_ = 0, next_ = 0;
    std::uint64_t overwritten_ = 0;
};
enum class XYMode { line, scatter, line_scatter };
struct XYStyle {
    XYMode mode = XYMode::line;
    float marker_size = 5;
    // Skip consecutive line vertices closer than this many screen pixels.
    // Set to zero for every point. Scatter modes always retain all points.
    float simplify_pixels = .25f;
    bool show_latest = false;
};
} // namespace ptgl::plot
