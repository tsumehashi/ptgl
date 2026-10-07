#pragma once
#include "ptgl/Plot/Stream.h"
#include "ptgl/Plot/Xy.h"
#include <string>

namespace ptgl::plot {
struct Color { float r, g, b, a = 1; };
struct Rect {
    float x, y, w, h;
    bool contains(float px, float py) const noexcept { return px >= x && py >= y && px < x + w && py < y + h; }
};
struct Vertex2D { float x, y, coverage = 1; };
// Physical framebuffer pixels. AA adds a one-pixel coverage fringe.
struct StrokeStyle { float width = 1; bool antialias = true; };
enum class Primitive { points, lines, line_strip, triangles, triangle_strip };
struct DrawCommand {
    Primitive primitive;
    std::size_t first, count;
    Rect clip;
    Color color;
};
struct TextCommand { float x, y; Color color; std::string value; };
struct DrawList {
    float width = 1, height = 1, text_scale = 1;
    std::vector<Vertex2D> vertices;
    std::vector<DrawCommand> commands;
    std::vector<TextCommand> texts;
};

// Coordinates are framebuffer pixels. Text is drawn above all geometry.
// Host owns the window, input, graphics context and presentation.
// Use each instance on one rendering thread.
class Renderer {
public:
    Renderer() = default;
    virtual ~Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    virtual void initialize(int max_vertices = 1024 * 1024) = 0;
    virtual void shutdown() = 0; // Call before destroying the graphics context.
    virtual void draw() = 0;
    void begin(float width, float height, float text_scale = 2);
    void rectangle(Rect rect, Color color);
    void line(float x0, float y0, float x1, float y1, Color color, StrokeStyle stroke = {});
    void text(float x, float y, const char* text, Color color);
    void series(const StreamBuffer& data, std::size_t channel, Range time, Range y,
                Rect plot, Color color, std::vector<Point>& scratch, StrokeStyle stroke = {2, true});
    void xy(const XYBuffer& data, Range x, Range y, Rect plot, Color color,
            StrokeStyle stroke = {2, true}, XYStyle style = {});
    bool overflowed() const noexcept { return overflow_; }
    std::size_t selectedPoints() const noexcept { return selected_; }
    const DrawList& drawList() const noexcept { return list_; }
protected:
    void configure(int max_vertices);
    void markOverflow() noexcept { overflow_ = true; }
    int maxVertices() const noexcept { return max_vertices_; }
private:
    bool room(std::size_t vertices, std::size_t commands);
    void stroke(const Vertex2D* points, std::size_t count, Rect clip, Color color, StrokeStyle style);
    void dots(const Vertex2D* points, std::size_t count, Rect clip, Color color, float size);
    Rect screen() const { return {0, 0, list_.width, list_.height}; }
    DrawList list_;
    std::vector<Vertex2D> transformed_, path_, offsets_;
    std::vector<Vertex2D> markers_;
    std::vector<std::size_t> run_ends_;
    int max_vertices_ = 0;
    std::size_t selected_ = 0, text_bytes_ = 0;
    bool overflow_ = false;
};
} // namespace ptgl::plot
