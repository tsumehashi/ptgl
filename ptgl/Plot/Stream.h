#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace ptgl::plot {

struct Point {
    double time = 0;
    float value = 0;
    // Start a new polyline here. Never connect across missing samples.
    bool break_before = false;
};

struct Range {
    double min = 0;
    double max = 1;
    double span() const noexcept { return max - min; }
    bool valid() const noexcept;
    void pan(double delta) noexcept;
    void zoom(double anchor, double factor, double min_span, double max_span) noexcept;
    void clampTo(Range bounds) noexcept;
};

enum class BucketAlignment { viewport, time };

// A fixed-capacity, synchronized, multichannel time series. One owner thread;
// callers must synchronize concurrent reads/writes externally. Timestamps are
// finite seconds and strictly increasing. NaN/Inf values are missing samples.
class StreamBuffer {
public:
    StreamBuffer(std::size_t channels, std::size_t capacity);
    bool append(double time, const float* values, std::size_t count) noexcept;
    void clear() noexcept;
    // Mark a discontinuity without losing either neighboring sample.
    void breakBeforeNextSample() noexcept { pendingBreak_ = true; }

    std::size_t channels() const noexcept { return channels_; }
    std::size_t capacity() const noexcept { return capacity_; }
    std::size_t size() const noexcept { return size_; }
    std::uint64_t overwritten() const noexcept { return overwritten_; }
    // Allocated payload; excludes object/allocator overhead and query outputs.
    std::size_t storageBytes() const noexcept;
    double oldestTime() const noexcept;
    double newestTime() const noexcept;
    Point sample(std::size_t channel, std::size_t chronological_index) const;
    bool nearest(std::size_t channel, double time, Point& result) const;

    // Pixel-column first/min/max/last aggregation, ordered by original time.
    // At <= 1 visible sample/pixel on average, return the raw samples instead.
    // Includes at most one bracketing sample on either side for clipping.
    // Viewport alignment: <= 4*pixels+2 finite points. Time alignment anchors
    // slightly quantized bucket widths to the first timestamp for stable scrolling;
    // partial edge buckets can add up to 12 points. Missing runs can add more.
    // A reused output vector avoids allocations after capacity is sufficient.
    void select(std::size_t channel, Range view, std::size_t pixels,
                std::vector<Point>& output,
                BucketAlignment alignment = BucketAlignment::viewport) const;

private:
    static constexpr std::size_t block_size = 16;
    static constexpr std::uint32_t none = std::numeric_limits<std::uint32_t>::max();
    struct Summary {
        std::uint32_t min = none;
        std::uint32_t max = none;
        std::uint32_t missing = 0;
    };
    std::size_t channels_, capacity_, size_ = 0, next_ = 0, leaves_ = 1;
    std::uint64_t overwritten_ = 0;
    std::vector<double> times_;
    std::vector<std::uint8_t> breaks_;
    bool pendingBreak_ = false;
    double time_origin_ = 0;
    std::vector<float> values_;
    std::vector<Summary> tree_;

    std::size_t slot(std::size_t logical) const noexcept;
    float value(std::size_t channel, std::size_t physical) const noexcept;
    std::size_t lowerBound(double time) const noexcept;
    std::size_t upperBound(double time) const noexcept;
    Summary one(std::size_t channel, std::size_t physical) const noexcept;
    Summary merge(std::size_t channel, Summary a, Summary b) const noexcept;
    void rebuild(std::size_t channel, std::size_t block) noexcept;
    Summary physicalQuery(std::size_t channel, std::size_t begin, std::size_t end) const noexcept;
    Summary query(std::size_t channel, std::size_t begin, std::size_t end) const noexcept;
    void emitFinite(std::size_t channel, std::size_t begin, std::size_t end,
                     Summary summary, bool& pending_break, std::vector<Point>& out) const;
};

} // namespace ptgl::plot
