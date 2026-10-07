#include "ptgl/Plot/Renderer.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace ptgl::plot {
void Renderer::configure(int max_vertices) {
    if (max_vertices < 6) throw std::invalid_argument("max_vertices must be at least 6");
    max_vertices_ = max_vertices;
    list_.vertices.reserve(std::size_t(max_vertices));
    list_.commands.reserve(4096); list_.texts.reserve(256);
    begin(1, 1, 1);
}
void Renderer::begin(float width, float height, float text_scale) {
    if (!std::isfinite(width) || !std::isfinite(height) || !std::isfinite(text_scale))
        throw std::invalid_argument("non-finite frame dimensions");
    list_.width = std::max(1.0f, width); list_.height = std::max(1.0f, height);
    list_.text_scale = std::max(1.0f, text_scale);
    list_.vertices.clear(); list_.commands.clear(); list_.texts.clear();
    selected_ = text_bytes_ = 0; overflow_ = false;
}
bool Renderer::room(std::size_t vertices, std::size_t commands) {
    if (vertices > std::size_t(max_vertices_) - list_.vertices.size() ||
        commands > 4096 - list_.commands.size()) {
        overflow_ = true; return false;
    }
    return true;
}
void Renderer::rectangle(Rect r, Color c) {
    if (!room(6, 1)) return;
    list_.commands.push_back({Primitive::triangles, list_.vertices.size(), 6, screen(), c});
    list_.vertices.insert(list_.vertices.end(), {{r.x,r.y},{r.x+r.w,r.y},{r.x+r.w,r.y+r.h},
                                                {r.x,r.y},{r.x+r.w,r.y+r.h},{r.x,r.y+r.h}});
}
namespace {
void validate(StrokeStyle style) {
    if (!std::isfinite(style.width) || style.width < .5f || style.width > 32)
        throw std::invalid_argument("stroke width must be between 0.5 and 32 pixels");
}
std::size_t strokeVertices(std::size_t n, StrokeStyle s) {
    if (n == 0) return 0;
    return s.antialias ? std::max(std::size_t(72), 6 * n + 16) : std::max(std::size_t(6), 2 * n);
}
Vertex2D normal(Vertex2D a, Vertex2D b) {
    const double dx = double(b.x) - a.x, dy = double(b.y) - a.y;
    const double length = std::hypot(dx, dy);
    return length > 1e-8 ? Vertex2D{float(-dy / length), float(dx / length)} : Vertex2D{0,1};
}
}
void Renderer::line(float x0, float y0, float x1, float y1, Color color, StrokeStyle style) {
    validate(style);
    if (!std::isfinite(x0) || !std::isfinite(y0) || !std::isfinite(x1) || !std::isfinite(y1)) return;
    if (!room(strokeVertices(2, style), style.antialias ? 5 : 1)) return;
    const Vertex2D points[]{{x0,y0},{x1,y1}};
    stroke(points, 2, screen(), color, style);
}
void Renderer::stroke(const Vertex2D* points, std::size_t count, Rect clip, Color color, StrokeStyle style) {
    path_.clear();
    for (std::size_t i = 0; i < count; ++i) {
        if (path_.empty() || std::hypot(double(points[i].x) - path_.back().x,
                                       double(points[i].y) - path_.back().y) > 1e-5)
            path_.push_back(points[i]);
    }
    if (path_.empty()) return;
    const float inner = style.antialias ? std::max(0.f, (style.width - 1) * .5f) : style.width * .5f;
    const float outer = (style.width + 1) * .5f;
    const float coverage = style.antialias ? std::min(style.width / outer, 1.f) : 1.f;
    auto command = [&](std::size_t first, Primitive primitive) {
        list_.commands.push_back({primitive, first, list_.vertices.size() - first, clip, color});
    };
    if (path_.size() == 1) {
        const auto p = path_.front();
        const auto first = list_.vertices.size();
        if (!style.antialias) {
            const float r = style.width * .5f;
            list_.vertices.insert(list_.vertices.end(), {{p.x-r,p.y-r},{p.x+r,p.y-r},{p.x+r,p.y+r},
                                                        {p.x-r,p.y-r},{p.x+r,p.y+r},{p.x-r,p.y+r}});
        } else {
            constexpr float d = .70710678f;
            constexpr std::array<Vertex2D,9> circle{{{1,0},{d,d},{0,1},{-d,d},{-1,0},{-d,-d},{0,-1},{d,-d},{1,0}}};
            for (std::size_t i = 0; i < 8; ++i) {
                const Vertex2D a{p.x+circle[i].x*inner,p.y+circle[i].y*inner,coverage};
                const Vertex2D b{p.x+circle[i+1].x*inner,p.y+circle[i+1].y*inner,coverage};
                const Vertex2D aa{p.x+circle[i].x*outer,p.y+circle[i].y*outer,0};
                const Vertex2D bb{p.x+circle[i+1].x*outer,p.y+circle[i+1].y*outer,0};
                list_.vertices.insert(list_.vertices.end(), {{p.x,p.y,coverage},a,b,a,aa,bb,a,bb,b});
            }
        }
        command(first, Primitive::triangles);
        return;
    }
    offsets_.resize(path_.size());
    auto previous = normal(path_[0],path_[1]);
    offsets_[0] = previous;
    for (std::size_t i = 1; i + 1 < path_.size(); ++i) {
        const auto next = normal(path_[i],path_[i+1]);
        const float divisor = 1 + previous.x * next.x + previous.y * next.y;
        auto m = next;
        if (divisor > .001f) {
            m = {(previous.x + next.x) / divisor, (previous.y + next.y) / divisor};
            const float length = std::hypot(m.x,m.y);
            if (length > 2) { m.x *= 2 / length; m.y *= 2 / length; }
        }
        offsets_[i] = m; previous = next;
    }
    offsets_.back() = previous;
    auto ribbon = [&](float a, float ca, float b, float cb) {
        const auto first = list_.vertices.size();
        for (std::size_t i = 0; i < path_.size(); ++i) {
            const auto p = path_[i], n = offsets_[i];
            list_.vertices.push_back({p.x+n.x*a,p.y+n.y*a,ca});
            list_.vertices.push_back({p.x+n.x*b,p.y+n.y*b,cb});
        }
        command(first, Primitive::triangle_strip);
    };
    ribbon(inner, coverage, -inner, coverage);
    if (!style.antialias) return;
    ribbon(outer, 0, inner, coverage);
    ribbon(-inner, coverage, -outer, 0);
    for (int end = 0; end < 2; ++end) {
        const auto p = end ? path_.back() : path_.front();
        const auto n = end ? offsets_.back() : offsets_.front();
        const float direction = end ? 1.f : -1.f;
        const Vertex2D tangent{n.y * direction, -n.x * direction};
        const auto first = list_.vertices.size();
        const float distances[]{outer, inner, -inner, -outer};
        const float coverages[]{0,coverage,coverage,0};
        for (int i = 0; i < 4; ++i) {
            const float x = p.x + n.x * distances[i], y = p.y + n.y * distances[i];
            list_.vertices.push_back({x,y,coverages[i]});
            list_.vertices.push_back({x+tangent.x,y+tangent.y,0});
        }
        command(first, Primitive::triangle_strip);
    }
}
void Renderer::text(float x, float y, const char* value, Color c) {
    if (!value) return;
    const auto size = std::strlen(value);
    if (size > 16384 - text_bytes_ || list_.texts.size() >= 4096) { overflow_ = true; return; }
    text_bytes_ += size; list_.texts.push_back({x, y, c, value});
}
void Renderer::series(const StreamBuffer& data, std::size_t channel, Range time, Range y,
                      Rect plot, Color color, std::vector<Point>& scratch, StrokeStyle style) {
    validate(style);
    if (!time.valid() || !y.valid() || !std::isfinite(plot.x) || !std::isfinite(plot.y) ||
        !std::isfinite(plot.w) || !std::isfinite(plot.h) || plot.w < 1 || plot.h < 1) return;
    data.select(channel, time, std::size_t(std::clamp(plot.w, 1.0f, 2048.0f)), scratch, BucketAlignment::time);
    selected_ += scratch.size();
    std::size_t vertices = 0, commands = 0;
    for (std::size_t begin = 0; begin < scratch.size();) {
        auto end = begin + 1;
        while (end < scratch.size() && !scratch[end].break_before) ++end;
        vertices += strokeVertices(end - begin, style);
        commands += style.antialias ? 5 : 1;
        begin = end;
    }
    if (!room(vertices, commands)) return;
    transformed_.clear();
    for (const auto& p : scratch) {
        // Subtract the origin in double before converting to GPU coordinates.
        const double x = plot.x + (p.time - time.min) / time.span() * plot.w;
        const double yy = plot.y + (1 - (double(p.value) - y.min) / y.span()) * plot.h;
        transformed_.push_back({float(std::clamp(x, -1e12, 1e12)), float(std::clamp(yy, -1e12, 1e12))});
    }
    for (std::size_t begin = 0; begin < scratch.size();) {
        auto end = begin + 1;
        while (end < scratch.size() && !scratch[end].break_before) ++end;
        stroke(transformed_.data() + begin, end - begin, plot, color, style);
        begin = end;
    }
}
void Renderer::dots(const Vertex2D* points, std::size_t count, Rect clip, Color color, float size) {
    if (!count) return;
    const auto command=list_.commands.size(), first=list_.vertices.size();
    // Merge discs into one triangle batch, avoiding one GPU draw per marker.
    for (std::size_t i=0;i<count;++i) {
        stroke(points+i,1,clip,color,{size,true});
        list_.commands.resize(command+1);
    }
    list_.commands[command]={Primitive::triangles,first,list_.vertices.size()-first,clip,color};
}
void Renderer::xy(const XYBuffer& data, Range x, Range y, Rect plot, Color color,
                  StrokeStyle stroke_style, XYStyle style) {
    validate(stroke_style);
    if (!std::isfinite(style.marker_size) || style.marker_size<.5f || style.marker_size>32 ||
        !std::isfinite(style.simplify_pixels) || style.simplify_pixels<0)
        throw std::invalid_argument("ptgl::plot: invalid XY marker size or simplification tolerance");
    if (!x.valid() || !y.valid() || !std::isfinite(plot.x) || !std::isfinite(plot.y) ||
        !std::isfinite(plot.w) || !std::isfinite(plot.h) || plot.w<1 || plot.h<1) return;
    const bool lines=style.mode!=XYMode::scatter, markers=style.mode!=XYMode::line;
    transformed_.clear(); markers_.clear(); run_ends_.clear();
    const double tolerance=markers ? 0 : style.simplify_pixels;
    std::size_t run_start=0;
    Vertex2D pending{}, latest{}; bool have_pending=false, have_latest=false;
    const auto finish=[&]() {
        if (!have_pending) return;
        if (transformed_.back().x!=pending.x || transformed_.back().y!=pending.y) transformed_.push_back(pending);
        run_ends_.push_back(transformed_.size()); run_start=transformed_.size(); have_pending=false;
    };
    const auto inMarkerClip=[&](Vertex2D p) {
        const float pad=(style.marker_size+1)*.5f;
        return p.x>=plot.x-pad && p.x<=plot.x+plot.w+pad && p.y>=plot.y-pad && p.y<=plot.y+plot.h+pad;
    };
    for (std::size_t i=0;i<data.size();++i) {
        const auto p=data.sample(i);
        if (p.break_before) finish();
        const double px=plot.x+(p.x-x.min)/x.span()*plot.w;
        const double py=plot.y+(1-(p.y-y.min)/y.span())*plot.h;
        have_latest=false;
        if (!std::isfinite(px) || !std::isfinite(py)) { finish(); continue; }
        const Vertex2D point{float(std::clamp(px,-1e12,1e12)),float(std::clamp(py,-1e12,1e12))};
        if (markers && inMarkerClip(point)) markers_.push_back(point);
        if (lines) {
            if (transformed_.size()==run_start ||
                std::hypot(double(point.x)-transformed_.back().x,double(point.y)-transformed_.back().y)>=tolerance)
                transformed_.push_back(point);
            pending=point; have_pending=true;
        }
        latest=point; have_latest=inMarkerClip(point);
    }
    finish();
    if (style.show_latest && have_latest && !markers) markers_.push_back(latest);
    selected_ += lines ? transformed_.size() : markers_.size();
    // Preflight the complete series, including its latest-point marker.
    if (markers_.size()>std::size_t(maxVertices())/72) { markOverflow(); return; }
    std::size_t vertices=markers_.size()*72, commands=markers_.empty() ? 0 : 1, begin=0;
    for (const auto end:run_ends_) {
        vertices+=strokeVertices(end-begin,stroke_style); commands+=stroke_style.antialias ? 5 : 1; begin=end;
        if (vertices>std::size_t(maxVertices()) || commands>4096) { markOverflow(); return; }
    }
    if (!room(vertices,commands)) return;
    begin=0;
    for (const auto end:run_ends_) { stroke(transformed_.data()+begin,end-begin,plot,color,stroke_style); begin=end; }
    dots(markers_.data(),markers_.size(),plot,color,style.marker_size);
}
} // namespace ptgl::plot
