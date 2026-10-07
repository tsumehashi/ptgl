#include "ptgl/Plot/Renderer.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
class RecordingRenderer final : public ptgl::plot::Renderer {
public:
    void initialize(int n = 1024) override { configure(n); }
    void shutdown() override {}
    void draw() override {}
};
// Sample the generated mesh, independently of its tessellation strategy.
static float coverageAt(const ptgl::plot::DrawList& f, float x, float y) {
    float coverage = 0;
    for (const auto& cmd : f.commands) {
        for (std::size_t i = 0; i + 2 < cmd.count; i += cmd.primitive == ptgl::plot::Primitive::triangles ? 3 : 1) {
            const auto a = f.vertices[cmd.first+i], b = f.vertices[cmd.first+i+1], c = f.vertices[cmd.first+i+2];
            const double denominator = (b.y-c.y)*(a.x-c.x)+(c.x-b.x)*(a.y-c.y);
            if (std::abs(denominator) < 1e-9) continue;
            const double u = ((b.y-c.y)*(x-c.x)+(c.x-b.x)*(y-c.y))/denominator;
            const double v = ((c.y-a.y)*(x-c.x)+(a.x-c.x)*(y-c.y))/denominator;
            const double w = 1-u-v;
            if (u >= 0 && v >= 0 && w >= 0)
                coverage = std::max(coverage,float(u*a.coverage+v*b.coverage+w*c.coverage));
        }
    }
    return coverage;
}
void geometry() {
    ptgl::plot::StreamBuffer data(1, 8);
    constexpr double epoch = 1700000000.0;
    const float values[] = {0, 1, NAN, 2, 3, NAN, 4};
    for (int i = 0; i < 7; ++i) check(data.append(epoch + i * .001, values + i, 1), "append");
    RecordingRenderer backend; backend.initialize();
    ptgl::plot::Renderer& r = backend; // Caller is independent of the concrete backend.
    r.begin(800, 600);
    std::vector<ptgl::plot::Point> scratch;
    r.series(data, 0, {epoch, epoch + .006}, {0,4}, {10,20,600,400}, {1,0,0}, scratch, {2,false});
    const auto& f = r.drawList();
    check(f.commands.size() == 3 && f.vertices.size() == 14, "gaps must split three runs");
    check(f.commands[0].primitive == ptgl::plot::Primitive::triangle_strip && f.commands[0].count == 4, "first run");
    check(f.commands[1].first == 4 && f.commands[1].count == 4, "second run offset");
    check(f.commands[2].primitive == ptgl::plot::Primitive::triangles, "isolated sample remains visible");
    check(std::abs((f.vertices[2].x + f.vertices[3].x) / 2 - 110.f) < .05f, "millisecond precision at epoch timestamps");
    check(std::abs((f.vertices[0].y + f.vertices[1].y) / 2 - 420.f) < .01f && std::abs(f.vertices.back().y - 21.f) < .01f, "vertical mapping");
    check(f.commands[1].clip.x == 10 && f.commands[1].clip.w == 600, "plot clipping survives backend handoff");
    char label[] = "angle";
    r.text(0,0,label,{1,1,1}); label[0] = 'X';
    check(f.texts[0].value == "angle", "deferred text owns its bytes");
    r.begin(400,300);
    check(f.commands.empty() && f.vertices.empty() && f.texts.empty() && r.selectedPoints() == 0, "new frame clears state");
}
void strokes() {
    RecordingRenderer r; r.initialize(); r.begin(200,200);
    r.line(10,50,100,50,{1,0,0},{4,false});
    const auto& f = r.drawList();
    check(f.commands.size() == 1 && f.vertices.size() == 4, "solid ribbon");
    check(f.vertices[0].y == 52 && f.vertices[1].y == 48, "requested physical line width");
    r.begin(200,200); r.line(10,50,100,50,{1,0,0},{4,true});
    float lo = 200, hi = 0; bool feather = false;
    for (auto p : f.vertices) {
        lo = std::min(lo,p.y); hi = std::max(hi,p.y);
        check(std::isfinite(p.x) && std::isfinite(p.y), "finite stroke geometry");
        check(p.coverage >= 0 && p.coverage <= 1, "coverage range");
        feather |= p.coverage == 0;
    }
    check(feather && hi-lo == 5, "one-pixel AA fringe");
    r.begin(200,200); r.line(50,50,50,50,{1,0,0},{4,true});
    check(f.commands.size() == 1 && f.vertices.size() == 72, "zero-length stroke becomes a dot");
    bool rejected = false;
    try { r.line(0,0,1,1,{1,1,1},{NAN,true}); } catch (const std::invalid_argument&) { rejected = true; }
    check(rejected,"reject invalid widths");
    ptgl::plot::StreamBuffer data(1,4); const float values[]{0,100,-100,0};
    for (int i=0;i<4;++i) data.append(i,values+i,1);
    std::vector<ptgl::plot::Point> scratch;
    r.begin(200,200); r.series(data,0,{0,3},{-100,100},{0,0,1,100},{1,0,0},scratch,{8,true});
    for (auto p : f.vertices) check(std::isfinite(p.x) && std::isfinite(p.y) && std::abs(p.x) < 20, "bounded joins at sharp reversals");
    for (float width : {.5f,1.f,2.f,4.f}) {
        float previous = -1;
        for (int step = 0; step <= 100; ++step) {
            r.begin(200,200); r.line(10,50+step*.01f,100,50+step*.01f,{1,0,0},{width,true});
            const float coverage = coverageAt(f,50.5f,50.5f);
            if (previous >= 0) check(std::abs(coverage-previous) < .015f,"subpixel motion must not toggle a pixel on/off");
            previous = coverage;
            float integrated = 0;
            for (int y = 44; y < 57; ++y) integrated += coverageAt(f,50.5f,y+.5f);
            check(std::abs(integrated-width) < .2f,"stroke brightness remains stable during motion");
        }
    }
}
void budgets() {
    RecordingRenderer r; r.initialize(6); r.begin(100,100);
    r.rectangle({0,0,10,10},{1,1,1});
    r.line(0,0,1,1,{1,1,1});
    check(r.overflowed() && r.drawList().vertices.size() == 6, "reject geometry exceeding budget");
    r.begin(100,100);
    ptgl::plot::StreamBuffer data(1,8); float value = 1;
    for (int i = 0; i < 8; ++i) data.append(i,&value,1);
    std::vector<ptgl::plot::Point> scratch;
    r.series(data,0,{0,7},{0,2},{0,0,100,100},{1,1,1},scratch);
    check(r.overflowed() && r.drawList().vertices.empty() && r.drawList().commands.empty(), "series overflow is atomic");
    r.begin(100,100);
    for (int i = 0; i < 4097; ++i) r.text(0,0,"",{1,1,1});
    check(r.overflowed() && r.drawList().texts.size() == 4096, "empty text cannot bypass command limit");
}
}
int main() {
    try { geometry(); strokes(); budgets(); std::cout << "render tests passed\n"; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
