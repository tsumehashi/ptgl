#include "ptgl/Plot/Stream.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace ptgl::plot {

bool Range::valid() const noexcept {
    return std::isfinite(min) && std::isfinite(max) && max > min && std::isfinite(span());
}
void Range::pan(double delta) noexcept {
    const Range next{min + delta, max + delta};
    if (next.valid()) *this = next;
}
void Range::zoom(double anchor, double factor, double min_span, double max_span) noexcept {
    if (!valid() || !std::isfinite(anchor) || !std::isfinite(factor) || factor <= 0 ||
        !std::isfinite(min_span) || !std::isfinite(max_span) || min_span <= 0 || max_span < min_span) return;
    const double fraction = std::clamp((anchor - min) / span(), 0.0, 1.0);
    const double width = std::clamp(span() * factor, min_span, max_span);
    const Range next{anchor - fraction * width, anchor + (1 - fraction) * width};
    if (next.valid()) *this = next;
}
void Range::clampTo(Range bounds) noexcept {
    if (!valid() || !bounds.valid()) return;
    if (span() >= bounds.span()) { *this = bounds; return; }
    if (min < bounds.min) pan(bounds.min - min);
    if (max > bounds.max) pan(bounds.max - max);
}

StreamBuffer::StreamBuffer(std::size_t channels, std::size_t capacity)
    : channels_(channels), capacity_(capacity) {
    if (!channels || !capacity || capacity >= none)
        throw std::invalid_argument("ptgl::plot: channels and capacity must be positive; capacity < UINT32_MAX");
    while (leaves_ < (capacity + block_size - 1) / block_size) leaves_ *= 2;
    if (channels > values_.max_size() / capacity || channels > tree_.max_size() / (2 * leaves_))
        throw std::length_error("ptgl::plot: buffer size overflow");
    times_.resize(capacity);
    breaks_.resize(capacity);
    values_.resize(channels * capacity);
    tree_.resize(channels * 2 * leaves_);
}

void StreamBuffer::clear() noexcept {
    size_ = next_ = 0;
    overwritten_ = 0;
    pendingBreak_ = false;
    std::fill(tree_.begin(), tree_.end(), Summary{});
}

std::size_t StreamBuffer::storageBytes() const noexcept {
    return times_.capacity() * sizeof(double) + values_.capacity() * sizeof(float) +
           tree_.capacity() * sizeof(Summary) + breaks_.capacity();
}

std::size_t StreamBuffer::slot(std::size_t logical) const noexcept {
    return ((size_ == capacity_ ? next_ : 0) + logical) % capacity_;
}
float StreamBuffer::value(std::size_t channel, std::size_t physical) const noexcept {
    return values_[channel * capacity_ + physical];
}
double StreamBuffer::oldestTime() const noexcept {
    return size_ ? times_[slot(0)] : std::numeric_limits<double>::quiet_NaN();
}
double StreamBuffer::newestTime() const noexcept {
    return size_ ? times_[(next_ + capacity_ - 1) % capacity_] : std::numeric_limits<double>::quiet_NaN();
}

bool StreamBuffer::append(double time, const float* values, std::size_t count) noexcept {
    if (!values || count != channels_ || !std::isfinite(time) || (size_ && time <= newestTime())) return false;
    if (!size_) time_origin_ = time;
    const auto physical = next_;
    times_[physical] = time;
    breaks_[physical] = pendingBreak_ ? 1 : 0;
    pendingBreak_ = false;
    for (std::size_t c = 0; c < channels_; ++c)
        values_[c * capacity_ + physical] = std::isfinite(values[c]) ? values[c] : std::numeric_limits<float>::quiet_NaN();
    if (size_ < capacity_) ++size_; else ++overwritten_;
    next_ = (next_ + 1) % capacity_;
    for (std::size_t c = 0; c < channels_; ++c) rebuild(c, physical / block_size);
    return true;
}

Point StreamBuffer::sample(std::size_t channel, std::size_t index) const {
    if (channel >= channels_ || index >= size_) throw std::out_of_range("ptgl::plot: sample index");
    const auto p = slot(index);
    return {times_[p], value(channel, p), breaks_[p] != 0};
}

std::size_t StreamBuffer::lowerBound(double time) const noexcept {
    std::size_t lo = 0, hi = size_;
    while (lo < hi) {
        const auto mid = lo + (hi - lo) / 2;
        if (times_[slot(mid)] < time) lo = mid + 1; else hi = mid;
    }
    return lo;
}
std::size_t StreamBuffer::upperBound(double time) const noexcept {
    std::size_t lo = 0, hi = size_;
    while (lo < hi) {
        const auto mid = lo + (hi - lo) / 2;
        if (times_[slot(mid)] <= time) lo = mid + 1; else hi = mid;
    }
    return lo;
}

bool StreamBuffer::nearest(std::size_t channel, double time, Point& result) const {
    if (channel >= channels_ || !size_ || !std::isfinite(time) || time < oldestTime() || time > newestTime()) return false;
    auto i = lowerBound(time);
    if (i == size_) --i;
    else if (i && time - times_[slot(i - 1)] <= times_[slot(i)] - time) --i;
    result = sample(channel, i);
    return std::isfinite(result.value);
}

StreamBuffer::Summary StreamBuffer::one(std::size_t channel, std::size_t p) const noexcept {
    if (!std::isfinite(value(channel, p))) return {none, none, 1};
    return {static_cast<std::uint32_t>(p), static_cast<std::uint32_t>(p), breaks_[p]};
}

StreamBuffer::Summary StreamBuffer::merge(std::size_t c, Summary a, Summary b) const noexcept {
    a.missing += b.missing;
    if (b.min == none) return a;
    if (a.min == none) { a.min = b.min; a.max = b.max; return a; }
    if (value(c, b.min) < value(c, a.min) ||
        (value(c, b.min) == value(c, a.min) && times_[b.min] < times_[a.min])) a.min = b.min;
    if (value(c, b.max) > value(c, a.max) ||
        (value(c, b.max) == value(c, a.max) && times_[b.max] < times_[a.max])) a.max = b.max;
    return a;
}

void StreamBuffer::rebuild(std::size_t c, std::size_t block) noexcept {
    Summary s;
    const auto end = std::min(size_, (block + 1) * block_size);
    for (auto p = block * block_size; p < end; ++p) s = merge(c, s, one(c, p));
    auto* nodes = tree_.data() + c * 2 * leaves_;
    auto i = leaves_ + block;
    nodes[i] = s;
    while (i > 1) { i /= 2; nodes[i] = merge(c, nodes[2 * i], nodes[2 * i + 1]); }
}

StreamBuffer::Summary StreamBuffer::physicalQuery(std::size_t c, std::size_t begin, std::size_t end) const noexcept {
    Summary s;
    while (begin < end && begin % block_size) s = merge(c, s, one(c, begin++));
    const auto aligned_end = end - end % block_size;
    if (begin < aligned_end) {
        auto a = leaves_ + begin / block_size, b = leaves_ + aligned_end / block_size;
        const auto* nodes = tree_.data() + c * 2 * leaves_;
        while (a < b) {
            if (a & 1) s = merge(c, s, nodes[a++]);
            if (b & 1) s = merge(c, s, nodes[--b]);
            a /= 2; b /= 2;
        }
        begin = aligned_end;
    }
    while (begin < end) s = merge(c, s, one(c, begin++));
    return s;
}

StreamBuffer::Summary StreamBuffer::query(std::size_t c, std::size_t begin, std::size_t end) const noexcept {
    if (begin >= end) return {};
    const auto start = slot(begin);
    const auto first_count = std::min(end - begin, capacity_ - start);
    auto s = physicalQuery(c, start, start + first_count);
    if (first_count < end - begin) s = merge(c, s, physicalQuery(c, 0, end - begin - first_count));
    return s;
}

void StreamBuffer::emitFinite(std::size_t c, std::size_t begin, std::size_t end,
                              Summary summary, bool& pending_break, std::vector<Point>& out) const {
    if (begin == end || summary.min == none) return;
    std::array<std::size_t, 4> points{slot(begin), summary.min, summary.max, slot(end - 1)};
    std::sort(points.begin(), points.end(), [&](auto a, auto b) { return times_[a] < times_[b]; });
    std::size_t previous = capacity_;
    for (auto p : points) {
        if (p == previous) continue;
        out.push_back({times_[p], value(c, p), pending_break});
        pending_break = false;
        previous = p;
    }
}

void StreamBuffer::select(std::size_t c, Range view, std::size_t pixels, std::vector<Point>& out,
                          BucketAlignment alignment) const {
    out.clear();
    if (c >= channels_) throw std::out_of_range("ptgl::plot: channel index");
    if (!view.valid() || !pixels || !size_ || view.max < oldestTime() || view.min > newestTime()) return;
    pixels = std::min(pixels, size_);
    out.reserve(pixels > (size_ > 2 ? (size_ - 2) / 4 : 0) ? size_ : pixels * 4 + 2);
    const auto inside_begin = lowerBound(view.min);
    const auto inside_end = upperBound(view.max);
    if (inside_end - inside_begin <= pixels) {
        // At sample-level zoom, emitting raw data is cheaper than traversing
        // empty pixel columns. Keep missing-data boundaries intact.
        bool gap = true;
        const auto a = inside_begin ? inside_begin - 1 : inside_begin;
        const auto b = inside_end < size_ ? inside_end + 1 : inside_end;
        for (auto i = a; i < b; ++i) {
            const auto p = slot(i);
            const auto v = value(c, p);
            if (!std::isfinite(v)) { gap = true; continue; }
            out.push_back({times_[p], v, gap || breaks_[p] != 0});
            gap = false;
        }
        return;
    }
    bool pending_break = true;
    auto emit = [&](std::size_t a, std::size_t b) {
        auto summary = query(c, a, b);
        if (!summary.missing) { emitFinite(c, a, b, summary, pending_break, out); return; }
        // Only mixed-validity buckets need a raw scan. Preserve every gap;
        // pathological alternating NaNs intentionally exceed the 4W bound.
        if (summary.min == none) { pending_break = true; return; }
        auto run = a;
        Summary finite;
        for (auto i = a; i < b; ++i) {
            const auto physical = slot(i);
            if (breaks_[physical]) {
                emitFinite(c, run, i, finite, pending_break, out);
                pending_break = true; finite = {}; run = i;
            }
            auto item = one(c, physical);
            item.missing = std::isfinite(value(c, physical)) ? 0 : 1;
            if (item.missing) {
                emitFinite(c, run, i, finite, pending_break, out);
                pending_break = true;
                finite = {};
                run = i + 1;
            } else finite = merge(c, finite, item);
        }
        emitFinite(c, run, b, finite, pending_break, out);
    };
    auto begin = inside_begin;
    if (begin) emit(begin - 1, begin);
    // Quantize to 16 significant binary digits so subtraction roundoff in a
    // moving view cannot perturb every bucket. Keep the original origin on eviction.
    int exponent = 0;
    const auto mantissa = std::frexp(view.span() / pixels, &exponent);
    const auto step = std::ldexp(std::round(mantissa * 65536.0), exponent - 16);
    const auto bucket = step > 0 ? std::floor((view.min - time_origin_) / step) : 0;
    if (alignment == BucketAlignment::time && step > 0 && std::isfinite(bucket) && bucket + 1 > bucket) {
        for (std::size_t x = 0; x < pixels + 2 && begin < inside_end; ++x) {
            const auto edge = time_origin_ + (bucket + double(x + 1)) * step;
            if (edge >= view.max) break;
            const auto end = std::clamp(lowerBound(edge), begin, inside_end);
            emit(begin, end);
            begin = end;
        }
        emit(begin, inside_end);
        begin = inside_end;
    } else {
        for (std::size_t x = 0; x < pixels; ++x) {
            const auto end = x + 1 == pixels ? inside_end :
                lowerBound(view.min + view.span() * (static_cast<double>(x + 1) / pixels));
            emit(begin, end);
            begin = end;
        }
    }
    if (begin < size_) emit(begin, begin + 1);
}

} // namespace ptgl::plot
