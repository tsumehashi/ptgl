#include "ptgl/Plot/Figure.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <utility>

namespace ptgl::plot {
namespace {
constexpr auto no_panel = std::size_t(-1);
void checkWidth(float width) {
    if (!std::isfinite(width) || width < .5f || width > 32)
        throw std::invalid_argument("ptgl::plot: line width must be between 0.5 and 32");
}
void checkRange(Range value) {
    if (!value.valid()) throw std::invalid_argument("ptgl::plot: invalid axis range");
}
Range paddedRange(double lo,double hi) {
    const double pad=hi>lo ? (hi-lo)*.05 : std::max(1.,std::abs(lo)*.05);
    return {lo-pad,hi+pad};
}
// ASCII labels use the bundled monospace font. Bound text to its layout cell.
std::string shorten(const std::string& text, float available, float font) {
    const auto count = std::size_t(std::max(0.f, available / (font * .62f)));
    if (text.size() <= count) return text;
    return count > 3 ? text.substr(0, count - 3) + "..." : std::string{};
}
}
Series& Series::setLineWidth(float width) {
    checkWidth(width); stroke_.width = width; return *this;
}
Series& Series::setStrokeStyle(StrokeStyle value) {
    checkWidth(value.width); stroke_ = value; return *this;
}
Series& Series::setXyStyle(XYStyle value) {
    if (!std::isfinite(value.marker_size) || value.marker_size < .5f || value.marker_size > 32 ||
        !std::isfinite(value.simplify_pixels) || value.simplify_pixels < 0 ||
        (value.mode != XYMode::line && value.mode != XYMode::scatter && value.mode != XYMode::line_scatter))
        throw std::invalid_argument("ptgl::plot: invalid XY style");
    xy_style_ = value; return *this;
}
Series& Series::setMarkerSize(float size) {
    auto style = xy_style_; style.marker_size = size; return setXyStyle(style);
}
void Series::appendXy(double x, double y, bool break_before) {
    if (!trajectory_ || !xy_) throw std::logic_error("ptgl::plot: appendXy requires a trajectory");
    xy_->append({x,y,break_before});
}
void Series::setXyData(std::vector<XYPoint> points) {
    if (!xy_ || trajectory_) throw std::logic_error("ptgl::plot: setXyData requires a static XY series");
    *xy_ = XYBuffer(std::move(points));
}
const XYBuffer& Series::xyData() const {
    if (!xy_) throw std::logic_error("ptgl::plot: this is not an XY series");
    return *xy_;
}
Series& Axes::addSeries(std::size_t channel, std::string label) {
    return addSeries(channel, std::move(label), theme_.palette[series_.size() % theme_.palette.size()], {default_width_, true});
}
Series& Axes::addSeries(std::size_t channel, std::string label, Color color, StrokeStyle stroke) {
    if (xy_) throw std::logic_error("ptgl::plot: use addXySeries or addTrajectory on XY axes");
    if (channel >= channels_) throw std::out_of_range("ptgl::plot: channel index");
    checkWidth(stroke.width);
    if (label.empty()) label = "CH" + std::to_string(channel + 1);
    Series s; s.channel_=channel; s.label_=std::move(label); s.color_=color; s.stroke_=stroke;
    series_.push_back(std::move(s));
    return series_.back();
}
Series& Axes::addXySeries(std::vector<XYPoint> points, std::string label) {
    if (!xy_) throw std::logic_error("ptgl::plot: addXySeries requires addXyAxes");
    Series s;
    s.label_ = label.empty() ? "XY"+std::to_string(series_.size()+1) : std::move(label);
    s.color_ = theme_.palette[series_.size()%theme_.palette.size()]; s.stroke_.width=default_width_;
    s.xy_.emplace(std::move(points));
    series_.push_back(std::move(s)); fitInitialXy(); return series_.back();
}
Series& Axes::addScatterSeries(std::vector<XYPoint> points, std::string label) {
    auto& series = addXySeries(std::move(points),std::move(label));
    series.xy_style_.mode=XYMode::scatter; return series;
}
Series& Axes::addTrajectory(std::size_t capacity, std::string label) {
    if (!xy_) throw std::logic_error("ptgl::plot: addTrajectory requires addXyAxes");
    Series s;
    s.label_ = label.empty() ? "Trail"+std::to_string(series_.size()+1) : std::move(label);
    s.color_ = theme_.palette[series_.size()%theme_.palette.size()]; s.stroke_.width=default_width_;
    s.xy_.emplace(capacity); s.trajectory_=true; s.xy_style_.show_latest=true; s.xy_style_.marker_size=8;
    series_.push_back(std::move(s)); return series_.back();
}
Axes& Axes::setXRange(Range limits) {
    if (!xy_) throw std::logic_error("ptgl::plot: use Figure::setXRange for a time axis");
    checkRange(limits); x_=initial_x_=limits; explicit_x_=true; return *this;
}
Axes& Axes::setXLabel(std::string value) { xlabel_=std::move(value); return *this; }
Axes& Axes::setEqualAspectEnabled(bool enabled) { equal_aspect_=enabled; return *this; }
Axes& Axes::setYRange(Range limits) { checkRange(limits); y_ = initial_y_ = limits; explicit_y_=true; return *this; }
Axes& Axes::setTitle(std::string value) { title_ = std::move(value); return *this; }
Axes& Axes::setYLabel(std::string value) { ylabel_ = std::move(value); return *this; }
Axes& Axes::setSeriesVisible(std::size_t index, bool visible) {
    series_.at(index).visible_ = visible;
    return *this;
}
void Axes::fitInitialXy() {
    double xlo=std::numeric_limits<double>::infinity(), xhi=-xlo, ylo=xlo, yhi=-xlo;
    for (const auto& s:series_) if (s.visible_) for (std::size_t i=0;i<s.xy_->size();++i) {
        const auto p=s.xy_->sample(i);
        if (!std::isfinite(p.x) || !std::isfinite(p.y)) continue;
        xlo=std::min(xlo,p.x); xhi=std::max(xhi,p.x); ylo=std::min(ylo,p.y); yhi=std::max(yhi,p.y);
    }
    if (xlo>xhi) return;
    const auto x=paddedRange(xlo,xhi), y=paddedRange(ylo,yhi);
    if (!explicit_x_ && x.valid()) x_=initial_x_=x;
    if (!explicit_y_ && y.valid()) y_=initial_y_=y;
}
Figure::Figure(std::size_t channels, std::size_t capacity, double window_seconds)
    : data_(channels, capacity), time_{0, window_seconds}, initial_span_(window_seconds) {
    checkRange(time_);
    scratch_.reserve(std::min(capacity, std::size_t(8192)));
}
Figure& Figure::setTitle(std::string value) { title_ = std::move(value); return *this; }
Figure& Figure::setColumns(std::size_t count) {
    if (!count) throw std::invalid_argument("ptgl::plot: columns must be positive");
    columns_ = count; return *this;
}
Figure& Figure::setLineWidth(float width) {
    checkWidth(width); line_width_ = width;
    for (auto& a : axes_) {
        a.default_width_ = width;
        for (auto& s : a.series_) s.stroke_.width = width;
    }
    return *this;
}
Axes& Figure::addAxes(std::string title, std::string unit, Range y) {
    checkRange(y);
    axes_.emplace_back();
    auto& a = axes_.back();
    a.channels_ = data_.channels(); a.theme_ = theme_; a.default_width_ = line_width_;
    a.setTitle(std::move(title)).setYLabel(std::move(unit)).setYRange(y);
    return a;
}
Axes& Figure::addXyAxes(std::string title, std::string x_label, std::string y_label) {
    auto& a=addAxes(std::move(title),std::move(y_label));
    a.xy_=true; a.explicit_x_=a.explicit_y_=false; a.xlabel_=std::move(x_label); return a;
}
bool Figure::append(double time, const float* values, std::size_t count) noexcept {
    return data_.append(time, values, count);
}
void Figure::clear() {
    data_.clear();
    for (auto& a:axes_) for (auto& s:a.series_) if (s.xy_) s.xy_->clear();
    reset();
}
void Figure::setXRange(Range time) { checkRange(time); time_ = time; live_ = false; }
void Figure::reset() {
    live_ = true; time_ = {0, initial_span_}; dragging_ = no_panel;
    for (auto& a : axes_) { a.y_ = a.initial_y_; a.x_ = a.initial_x_; }
    updateTime();
}
void Figure::updateTime() {
    if (!data_.size()) return;
    const auto span = time_.span();
    if (live_ || span >= data_.newestTime() - data_.oldestTime()) {
        const Range next{data_.newestTime() - span, data_.newestTime()};
        if (next.valid()) time_ = next;
    } else time_.clampTo({data_.oldestTime(), data_.newestTime()});
}
bool Figure::fitY(std::size_t panel) {
    auto& a = axes_.at(panel);
    updateTime();
    double lo = std::numeric_limits<double>::infinity(), hi = -lo;
    for (const auto& s : a.series_) if (s.visible_) {
        if (a.xy_) {
            for (std::size_t i=0;i<s.xy_->size();++i) {
                const auto p=s.xy_->sample(i);
                if (std::isfinite(p.x) && std::isfinite(p.y) && p.x>=a.x_.min && p.x<=a.x_.max) {
                    lo=std::min(lo,p.y); hi=std::max(hi,p.y);
                }
            }
            continue;
        }
        data_.select(s.channel_, time_, 1, scratch_);
        for (const auto& p : scratch_) if (p.time >= time_.min && p.time <= time_.max && std::isfinite(p.value)) {
            lo = std::min(lo, double(p.value)); hi = std::max(hi, double(p.value));
        }
    }
    if (lo > hi) return false;
    const double pad = hi > lo ? (hi - lo) * .05 : std::max(1.0, std::abs(lo) * .05);
    const Range next{lo-pad,hi+pad}; if (!next.valid()) return false;
    a.y_ = next;
    return true;
}
void Figure::fitY() { for (std::size_t i = 0; i < axes_.size(); ++i) fitY(i); }
bool Figure::fitXy(std::size_t panel) {
    auto& a=axes_.at(panel); if (!a.xy_) return false;
    double xlo=std::numeric_limits<double>::infinity(), xhi=-xlo, ylo=xlo, yhi=-xlo;
    for (const auto& s:a.series_) if (s.visible_) for (std::size_t i=0;i<s.xy_->size();++i) {
        const auto p=s.xy_->sample(i);
        if (!std::isfinite(p.x) || !std::isfinite(p.y)) continue;
        xlo=std::min(xlo,p.x); xhi=std::max(xhi,p.x); ylo=std::min(ylo,p.y); yhi=std::max(yhi,p.y);
    }
    if (xlo>xhi) return false;
    const auto x=paddedRange(xlo,xhi), y=paddedRange(ylo,yhi);
    if (!x.valid() || !y.valid()) return false;
    a.x_=x; a.y_=y; return true;
}
void Figure::fitXy() { for (std::size_t i=0;i<axes_.size();++i) fitXy(i); }

void Figure::render(Renderer& r, Rect bounds, float dpi) {
    if (!std::isfinite(dpi) || dpi <= 0 || !std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
        !std::isfinite(bounds.w) || !std::isfinite(bounds.h) || bounds.w <= 0 || bounds.h <= 0)
        throw std::invalid_argument("ptgl::plot: invalid figure bounds or DPI");
    updateTime();
    const float scale = dpi, font = 12 * r.drawList().text_scale;
    const float line = std::max(18 * scale, font * 1.3f), margin = 12 * scale;
    r.rectangle(bounds, theme_.background);
    const auto write = [&](float x, float y, const std::string& value, Color color, float width) {
        const auto label = shorten(value, width, font);
        if (!label.empty()) r.text(x, y, label.c_str(), color);
    };
    const auto checkbox = [&](Rect hit, bool checked, Color color, const std::string& label) {
        const float size = 12*scale;
        if (hit.w < size) return;
        const Rect box{hit.x, hit.y+(font-size)*.5f, size, size};
        r.rectangle(box, checked ? color : theme_.muted);
        if (checked) {
            const StrokeStyle stroke{std::clamp(1.5f*scale,.5f,32.f),true};
            r.line(box.x+size*.2f,box.y+size*.5f,box.x+size*.43f,box.y+size*.74f,theme_.panel,stroke);
            r.line(box.x+size*.43f,box.y+size*.74f,box.x+size*.82f,box.y+size*.25f,theme_.panel,stroke);
        } else r.rectangle({box.x+scale,box.y+scale,size-2*scale,size-2*scale},theme_.panel);
        write(hit.x+size+5*scale,hit.y,label,checked ? color : theme_.muted,hit.w-size-9*scale);
    };
    write(bounds.x+margin, bounds.y+margin, title_, theme_.ink, bounds.w-2*margin);
    const bool has_time=std::any_of(axes_.begin(),axes_.end(),[](const Axes& a){return !a.xy_;});
    live_zoom_rect_={};
    float status_x=bounds.x+margin;
    if (has_time) {
        live_zoom_rect_ = {bounds.x+margin, bounds.y+margin+line,
                          std::min(std::max(0.f,bounds.w-2*margin),std::max(140*scale,font*.62f*18)),line};
        checkbox(live_zoom_rect_,live_zoom_,theme_.palette[0],"Live zoom (F)");
        status_x=live_zoom_rect_.x+live_zoom_rect_.w+margin;
    }
    char label[192];
    if (has_time) std::snprintf(label, sizeof(label), "%s | %.3g s | %s | %zu ch | line %.1f px",
                  live_ ? (live_zoom_ ? "LIVE ZOOM" : "LIVE") : "INSPECT", time_.span(),
                  paused_ ? "SOURCE PAUSED" : "SOURCE RUNNING", data_.channels(), line_width_);
    else {
        std::size_t count=0; bool streaming=false;
        for (const auto& a:axes_) for (const auto& s:a.series_) if (s.xy_) { count+=s.xy_->size(); streaming |= s.trajectory_; }
        std::snprintf(label,sizeof(label),"XY | %s | %zu points | line %.1f px",
                      streaming ? (paused_ ? "SOURCE PAUSED" : "SOURCE RUNNING") : "STATIC",count,line_width_);
    }
    write(status_x, bounds.y+margin+line, label, theme_.palette[0], bounds.x+bounds.w-margin-status_x);
    write(bounds.x+margin, bounds.y+margin+2*line, "Wheel: zoom | Shift+wheel: Y | Drag: pan | Checkbox/label: show/hide", theme_.muted, bounds.w-2*margin);
    write(bounds.x+margin, bounds.y+margin+3*line, has_time ? "L: live | R: reset | A: fit axes | Space: pause source | -/+: width" :
          "R: reset | A: fit XY | Space: pause source | -/+: width", theme_.muted, bounds.w-2*margin);
    for (auto& a : axes_) { a.plot_ = {}; a.legends_.assign(a.series_.size(), Rect{}); }
    if (axes_.empty()) {
        write(bounds.x+margin, bounds.y+margin+5*line, "Add a subplot and register a channel with addSeries().", theme_.muted, bounds.w-2*margin);
        return;
    }
    const auto cols = std::min(columns_, axes_.size()), rows = (axes_.size() + cols - 1) / cols;
    const float top = margin + 4*line + margin;
    const float cw = (bounds.w - (float(cols)+1)*margin) / float(cols);
    const float ch = (bounds.h - top - (float(rows)+1)*margin) / float(rows);
    if (cw < 200*scale || ch < 110*scale) {
        dragging_ = no_panel;
        write(bounds.x+margin, bounds.y+top, "Enlarge the window to view all panels.", theme_.ink, bounds.w-2*margin);
        return;
    }
    for (std::size_t i = 0; i < axes_.size(); ++i) {
        auto& a = axes_[i];
        const Rect card{bounds.x+margin+float(i%cols)*(cw+margin), bounds.y+top+float(i/cols)*(ch+margin), cw, ch};
        r.rectangle(card, theme_.panel);
        write(card.x+8*scale, card.y+4*scale, a.title_, theme_.ink, card.w-16*scale);
        const float legend_top = card.y+line+4*scale;
        const auto legend_cols = std::max(std::size_t(1), std::min(a.series_.size(), std::size_t(std::max(1.f, (cw-16*scale)/(90*scale)))));
        const auto legend_rows = std::max(std::size_t(1), (a.series_.size()+legend_cols-1)/legend_cols);
        const float cell = (cw-16*scale) / float(legend_cols);
        Rect plot{card.x+68*scale, legend_top+float(legend_rows)*line+8*scale,
                  cw-84*scale, ch-(legend_top-card.y)-float(legend_rows)*line-8*scale-line-6*scale-(a.xy_ ? line : 0)};
        if (plot.h < 24*scale) {
            write(card.x+8*scale, legend_top, "Panel too small", theme_.muted, cw-16*scale);
            continue;
        }
        if (a.xy_ && a.equal_aspect_) {
            const double unit=std::min(plot.w/a.x_.span(),plot.h/a.y_.span());
            const float w=float(unit*a.x_.span()), h=float(unit*a.y_.span());
            plot.x+=(plot.w-w)*.5f; plot.y+=(plot.h-h)*.5f; plot.w=w; plot.h=h;
        }
        a.plot_ = plot;
        for (std::size_t k = 0; k < a.series_.size(); ++k) {
            const auto& s = a.series_[k];
            const Rect hit{card.x+8*scale+float(k%legend_cols)*cell, legend_top+float(k/legend_cols)*line, cell, line};
            a.legends_[k] = hit;
            checkbox(hit,s.visible_,s.color_,s.label_);
        }
        if (a.xy_) write(plot.x,plot.y-font-2*scale,a.ylabel_,theme_.muted,plot.w);
        else write(card.x+4*scale, plot.y-line, a.ylabel_, theme_.muted, 60*scale);
        const int divisions = plot.h >= 140*scale ? 4 : 2;
        for (int tick = 0; tick <= divisions; ++tick) {
            const double fraction = double(tick) / divisions;
            const float yy = plot.y + float(fraction)*plot.h;
            r.line(plot.x, yy, plot.x+plot.w, yy, theme_.grid);
            std::snprintf(label, sizeof(label), "%.4g", a.y_.max - fraction*a.y_.span());
            write(card.x+4*scale, yy-font*.4f, label, theme_.muted, 62*scale);
        }
        for (int tick = 0; tick <= 2; ++tick) {
            const float xx = plot.x + float(tick)*.5f*plot.w;
            r.line(xx, plot.y, xx, plot.y+plot.h, theme_.grid);
            const auto xr=a.xy_ ? a.x_ : time_;
            std::snprintf(label, sizeof(label), a.xy_ ? "%.4g" : "%.3fs", xr.min+tick*.5*xr.span());
            const auto text = shorten(label, plot.w*.48f, font);
            const float x = tick == 2 ? xx-float(text.size())*font*.6f : xx;
            r.text(x, plot.y+plot.h+4*scale, text.c_str(), theme_.muted);
        }
        if (a.xy_) {
            const auto title=shorten(a.xlabel_,plot.w,font);
            r.text(plot.x+std::max(0.f,(plot.w-float(title.size())*font*.6f)*.5f),plot.y+plot.h+line+4*scale,title.c_str(),theme_.muted);
        }
        bool any_visible = false, have_data = false;
        for (const auto& s : a.series_) if (s.visible_) {
            any_visible = true;
            auto stroke = s.stroke_; stroke.width = std::clamp(stroke.width*scale, .5f, 32.f);
            if (a.xy_) {
                auto style=s.xy_style_; style.marker_size=std::clamp(style.marker_size*scale,.5f,32.f);
                style.simplify_pixels*=scale;
                r.xy(*s.xy_,a.x_,a.y_,plot,s.color_,stroke,style); have_data |= s.xy_->size()>0;
            } else { r.series(data_, s.channel_, time_, a.y_, plot, s.color_, scratch_, stroke); have_data |= data_.size()>0; }
        }
        if (!have_data || !any_visible)
            write(plot.x+8*scale, plot.y+8*scale, !any_visible ? "All series hidden" : "Waiting for samples...", theme_.muted, plot.w-16*scale);
        if (a.xy_ && plot.contains(mouse_x_,mouse_y_)) {
            const double x=a.x_.min+(mouse_x_-plot.x)/plot.w*a.x_.span();
            const double y=a.y_.max-(mouse_y_-plot.y)/plot.h*a.y_.span();
            r.line(mouse_x_,plot.y,mouse_x_,plot.y+plot.h,theme_.cursor);
            r.line(plot.x,mouse_y_,plot.x+plot.w,mouse_y_,theme_.cursor);
            std::snprintf(label,sizeof(label),"X %.5g  Y %.5g",x,y);
            write(plot.x+6*scale,plot.y+4*scale,label,theme_.ink,plot.w-12*scale);
        }
        if (!a.xy_ && plot.contains(mouse_x_, mouse_y_) && data_.size()) {
            const double time = time_.min+(mouse_x_-plot.x)/plot.w*time_.span();
            r.line(mouse_x_, plot.y, mouse_x_, plot.y+plot.h, theme_.cursor);
            for (std::size_t k = 0; k < a.series_.size(); ++k) if (a.series_[k].visible_) {
                const auto& s = a.series_[k];
                Point p;
                if (data_.nearest(s.channel_, time, p)) std::snprintf(label, sizeof(label), "%.4g", p.value);
                else std::snprintf(label, sizeof(label), "--");
                // Raw values in the plot preserve the clickable legend text above.
                if (float(k+1)*line < plot.h)
                    write(plot.x+6*scale, plot.y+4*scale+float(k)*line, s.label_+": "+label, s.color_, plot.w-12*scale);
            }
        }
    }
    if (r.overflowed())
        r.text(bounds.x+margin, bounds.y+bounds.h-line, "RENDER BUDGET EXCEEDED", theme_.palette[3]);
}

bool Figure::handle(const InputEvent& e) {
    if (e.type == InputType::cancel || e.type == InputType::leave) {
        dragging_ = no_panel; mouse_x_ = mouse_y_ = -1; return false;
    }
    if (e.type == InputType::key) {
        if (e.repeat) return false;
        switch (e.key) {
            case Key::live: setFollowLatest(); return true;
            case Key::live_zoom: setLiveZoomEnabled(!live_zoom_); return true;
            case Key::reset: reset(); return true;
            case Key::fit_y:
                for (std::size_t i=0;i<axes_.size();++i) { if (axes_[i].xy_) fitXy(i); else fitY(i); }
                return true;
            case Key::pause: paused_ = !paused_; return true;
            case Key::thicker: setLineWidth(std::min(32.f, line_width_+.5f)); return true;
            case Key::thinner: setLineWidth(std::max(.5f, line_width_-.5f)); return true;
            default: return false;
        }
    }
    if (!std::isfinite(e.x) || !std::isfinite(e.y)) return false;
    const float dx = e.x-mouse_x_, dy = e.y-mouse_y_;
    mouse_x_ = e.x; mouse_y_ = e.y;
    if (e.type == InputType::press && e.left && live_zoom_rect_.contains(e.x,e.y)) {
        setLiveZoomEnabled(!live_zoom_); return true;
    }
    if (e.type == InputType::release && e.left) {
        const bool consumed = dragging_ != no_panel; dragging_ = no_panel; return consumed;
    }
    if (e.type == InputType::move && dragging_ < axes_.size()) {
        auto& a = axes_[dragging_]; const auto p = a.plot_;
        if (p.w > 0 && p.h > 0) {
            auto& x=a.xy_ ? a.x_ : time_;
            x.pan(-dx/p.w*x.span()); a.y_.pan(dy/p.h*a.y_.span());
            if (!a.xy_) updateTime();
        }
        return true;
    }
    for (std::size_t i = 0; i < axes_.size(); ++i) {
        auto& a = axes_[i];
        if (e.type == InputType::press && e.left) {
            for (std::size_t k = 0; k < a.legends_.size(); ++k) if (a.legends_[k].contains(e.x,e.y)) {
                a.setSeriesVisible(k, !a.isSeriesVisible(k)); return true;
            }
            if (a.plot_.contains(e.x,e.y)) { dragging_ = i; if (!a.xy_) live_ = false; return true; }
        }
        if (e.type == InputType::scroll && a.plot_.contains(e.x,e.y) && std::isfinite(e.scroll_y) && e.scroll_y != 0) {
            const auto p = a.plot_;
            const double factor = std::exp(-std::clamp(double(e.scroll_y),-20.0,20.0)*.12);
            if (a.xy_) {
                if (!e.shift) a.x_.zoom(a.x_.min+(e.x-p.x)/p.w*a.x_.span(),factor,1e-9,1e30);
                a.y_.zoom(a.y_.max-(e.y-p.y)/p.h*a.y_.span(),factor,1e-9,1e30);
            }
            else if (e.shift) a.y_.zoom(a.y_.max-(e.y-p.y)/p.h*a.y_.span(), factor, .000001, 1e30);
            else {
                const bool keep_following = live_ && live_zoom_;
                const double anchor = keep_following ? time_.max : time_.min+(e.x-p.x)/p.w*time_.span();
                time_.zoom(anchor, factor, .000001, std::max(initial_span_,time_.span()));
                live_ = keep_following; updateTime();
            }
            return true;
        }
    }
    return false;
}
} // namespace ptgl::plot
