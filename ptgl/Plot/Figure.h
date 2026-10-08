#pragma once
#include "ptgl/Plot/Renderer.h"
#include <array>
#include <deque>
#include <optional>
#include <utility>

namespace ptgl::plot {
struct Theme {
    Color background{.965f, .973f, .980f}, panel{1, 1, 1};
    Color ink{.125f, .157f, .200f}, muted{.349f, .396f, .459f};
    Color grid{.855f, .878f, .906f}, cursor{.388f, .439f, .510f};
    std::array<Color, 4> palette{{{.137f,.404f,.690f}, {.733f,.353f,.035f},
                                 {.031f,.486f,.400f}, {.604f,.271f,.565f}}};
};
class Series {
public:
    std::size_t channel() const noexcept { return channel_; }
    Series& setLabel(std::string value) { label_ = std::move(value); return *this; }
    const std::string& label() const noexcept { return label_; }
    Series& setColor(Color value) noexcept { color_ = value; return *this; }
    Series& setColor(float r, float g, float b, float a = 1) noexcept { return setColor({r,g,b,a}); }
    Color color() const noexcept { return color_; }
    Series& setVisible(bool visible = true) noexcept { visible_ = visible; return *this; }
    bool isVisible() const noexcept { return visible_; }
    Series& setLineWidth(float logicalPixels);
    float lineWidth() const noexcept { return stroke_.width; }
    Series& setStrokeStyle(StrokeStyle value);
    StrokeStyle strokeStyle() const noexcept { return stroke_; }
    Series& setXyStyle(XYStyle value);
    XYStyle xyStyle() const noexcept { return xy_style_; }
    Series& setMarkerSize(float logicalPixels);
    // Append is available only for a trajectory; setXyData only for static XY.
    void appendXy(double x, double y, bool break_before = false);
    void setXyData(std::vector<XYPoint> points);
    const XYBuffer& xyData() const;
    bool isXy() const noexcept { return xy_.has_value(); }
    bool isTrajectory() const noexcept { return trajectory_; }
private:
    friend class Axes;
    friend class Figure;
    std::size_t channel_ = 0;
    std::string label_;
    Color color_{.137f,.404f,.690f};
    StrokeStyle stroke_{2, true}; // Logical pixels; scaled by render()'s DPI.
    bool visible_ = true;
    XYStyle xy_style_;
    std::optional<XYBuffer> xy_;
    bool trajectory_ = false;
};

class Axes {
public:
    // References remain valid across subsequent additions to the Figure/Axes.
    Series& addSeries(std::size_t channel, std::string label = {});
    Series& addSeries(std::size_t channel, std::string label, Color color, StrokeStyle stroke = {2, true});
    Series& addXySeries(std::vector<XYPoint> points, std::string label = {});
    Series& addScatterSeries(std::vector<XYPoint> points, std::string label = {});
    Series& addTrajectory(std::size_t capacity, std::string label = {});
    Axes& setXRange(Range limits); // Independent X range, only for XY panels.
    Range xRange() const noexcept { return x_; }
    Axes& setXLabel(std::string value);
    const std::string& xLabel() const noexcept { return xlabel_; }
    Axes& setEqualAspectEnabled(bool enabled = true); // Equal screen scale for X and Y.
    bool isEqualAspectEnabled() const noexcept { return equal_aspect_; }
    bool isXy() const noexcept { return xy_; }
    Axes& setYRange(Range limits); // Also updates the reset range.
    Axes& setTitle(std::string value);
    const std::string& title() const noexcept { return title_; }
    Axes& setYLabel(std::string value);
    const std::string& yLabel() const noexcept { return ylabel_; }
    Range yRange() const noexcept { return y_; }
    Series& series(std::size_t index) { return series_.at(index); }
    const Series& series(std::size_t index) const { return series_.at(index); }
    // Index is the plot registration order within this Axes, not a channel ID.
    // Hidden series keep receiving data and are excluded from drawing and fitY().
    Axes& setSeriesVisible(std::size_t index, bool visible = true);
    bool isSeriesVisible(std::size_t index) const { return series_.at(index).isVisible(); }
    std::size_t numSeries() const noexcept { return series_.size(); }
    Rect plotRect() const noexcept { return plot_; } // Last rendered layout.
    Rect legendRect(std::size_t index) const { return legends_.at(index); }
private:
    friend class Figure;
    std::size_t channels_ = 0;
    float default_width_ = 2;
    Theme theme_;
    std::string title_, ylabel_, xlabel_;
    Range x_{-1, 1}, initial_x_{-1, 1};
    Range y_{-1, 1}, initial_y_{-1, 1};
    bool xy_ = false, equal_aspect_ = false;
    bool explicit_x_ = false, explicit_y_ = false;
    void fitInitialXy();
    std::deque<Series> series_;
    Rect plot_{};
    std::vector<Rect> legends_;
};

enum class InputType { move, press, release, scroll, leave, cancel, key };
enum class Key { other, live, reset, fit_y, pause, thicker, thinner, live_zoom };
struct InputEvent {
    InputType type;
    // Physical framebuffer pixels, matching Renderer coordinates.
    float x = 0, y = 0, scroll_y = 0;
    bool left = false, shift = false, repeat = false;
    Key key = Key::other;
};

// Owns bounded data and interactive plot state. Use on one display thread;
// transfer acquisition-thread data with an application-owned queue.
class Figure {
public:
    Figure() : Figure(1, 1) {} // Minimal time-buffer storage for XY-only figures.
    explicit Figure(std::size_t channels, std::size_t capacity, double window_seconds = 60);
    Figure(const Figure&) = delete;
    Figure& operator=(const Figure&) = delete;
    Figure(Figure&&) = default;
    Figure& operator=(Figure&&) = default;
    Figure& setTitle(std::string value);
    const std::string& title() const noexcept { return title_; }
    Figure& setColumns(std::size_t count);
    std::size_t columns() const noexcept { return columns_; }
    Figure& setLineWidth(float logical_pixels);
    Axes& addAxes(std::string title = {}, std::string unit = {}, Range y = {-1, 1});
    Axes& addXyAxes(std::string title = {}, std::string x_label = "X", std::string y_label = "Y");
    Axes& axes(std::size_t index) { return axes_.at(index); }
    const Axes& axes(std::size_t index) const { return axes_.at(index); }
    std::size_t numAxes() const noexcept { return axes_.size(); }
    bool append(double time, const float* values, std::size_t count) noexcept;
    template<std::size_t N> bool append(double time, const std::array<float, N>& values) noexcept {
        return append(time, values.data(), N);
    }
    template<std::size_t N> bool append(double time, const float (&values)[N]) noexcept {
        return append(time, values, N);
    }
    bool append(double time, const std::vector<float>& values) noexcept {
        return append(time, values.data(), values.size());
    }
    const StreamBuffer& data() const noexcept { return data_; }
    void clear();
    void breakBeforeNextSample() noexcept { data_.breakBeforeNextSample(); }
    void setFollowLatest(bool enabled = true) noexcept { live_ = enabled; dragging_ = std::size_t(-1); }
    bool isFollowingLatest() const noexcept { return live_; }
    // Keep following the newest sample while wheel-zooming the time axis.
    // Enabling also resumes following at the current width. Disabling changes
    // future zoom behavior; use setFollowLatest(false) to stop following now.
    void setLiveZoomEnabled(bool enabled) noexcept { live_zoom_ = enabled; if (enabled) setFollowLatest(); }
    bool isLiveZoomEnabled() const noexcept { return live_zoom_; }
    Rect liveZoomRect() const noexcept { return live_zoom_rect_; } // Last rendered toggle.
    void setXRange(Range time); // Switches to inspection mode.
    Range xRange() const noexcept { return time_; }
    void reset();
    bool fitY(std::size_t panel); // Visible series, current time range, 5% margin.
    void fitY();
    bool fitXy(std::size_t panel); // Visible XY series, both axes, 5% margin.
    void fitXy();
    void setSourcePaused(bool paused) noexcept { paused_ = paused; }
    bool isSourcePaused() const noexcept { return paused_; }
    float lineWidth() const noexcept { return line_width_; }
    const Theme& theme() const noexcept { return theme_; }
    // Adds commands to an already begun frame. Does not clear, draw or swap.
    void render(Renderer& renderer, Rect bounds, float dpi = 1);
    bool handle(const InputEvent& event);
private:
    void updateTime();
    StreamBuffer data_;
    Theme theme_;
    std::deque<Axes> axes_; // Axes references survive subsequent insertions.
    std::vector<Point> scratch_;
    std::string title_ = "PTGL / PLOT";
    std::size_t columns_ = 1;
    Range time_;
    double initial_span_;
    float line_width_ = 2, mouse_x_ = -1, mouse_y_ = -1;
    bool live_ = true, paused_ = false, live_zoom_ = false;
    Rect live_zoom_rect_{};
    std::size_t dragging_ = std::size_t(-1);
};
} // namespace ptgl::plot
