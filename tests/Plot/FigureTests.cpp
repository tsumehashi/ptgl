#include "ptgl/Plot/Figure.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {
void check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
bool close(double a, double b) { return std::abs(a-b) < 1e-7; }
class RecordingRenderer final : public ptgl::plot::Renderer {
public:
    void initialize(int n = 1024*1024) override { configure(n); }
    void shutdown() override {}
    void draw() override {}
};
void render(ptgl::plot::Figure& f, RecordingRenderer& r) { r.begin(1000,700,1); f.render(r,{0,0,1000,700}); }
ptgl::plot::InputEvent key(ptgl::plot::Key k) { ptgl::plot::InputEvent e{}; e.type = ptgl::plot::InputType::key; e.key = k; return e; }
void interaction() {
    ptgl::plot::Figure f(2,1000,10);
    auto& a = f.addAxes("Angles","deg",{-5,5}); a.addSeries(0,"A"); a.addSeries(1,"B");
    auto* stable = &a;
    f.addAxes("Second").addSeries(0);
    check(stable == &f.axes(0),"subplot references must survive insertion");
    for (int i=0;i<1000;++i) {
        const std::array<float,2> row{float(i%10),float(i%20)};
        check(f.append(i*.01,row),"array append");
    }
    RecordingRenderer r; r.initialize(); render(f,r);
    check(close(f.xRange().span(),10) && close(f.xRange().max,9.99),"follow preserves requested time width");
    auto p = a.plotRect();
    check(p.w > 0 && p.h > 0 && !r.overflowed(),"panel layout");
    ptgl::plot::InputEvent scroll{ptgl::plot::InputType::scroll}; scroll.x=p.x+p.w*.5f; scroll.y=p.y+p.h*.5f; scroll.scroll_y=2;
    check(f.handle(scroll) && !f.isFollowingLatest() && f.xRange().span()<10,"time zoom stops follow");
    const double before=f.xRange().span(); const auto ybefore=a.yRange(); scroll.shift=true;
    f.handle(scroll);
    check(close(before,f.xRange().span()) && a.yRange().span()<ybefore.span(),"shift wheel only changes Y");
    ptgl::plot::InputEvent press{ptgl::plot::InputType::press}; press.x=scroll.x; press.y=scroll.y; press.left=true;
    f.handle(press); auto move=press; move.type=ptgl::plot::InputType::move; move.y+=20;
    const double old_y=a.yRange().min; f.handle(move);
    check(a.yRange().min>old_y,"drag pans Y");
    f.handle({ptgl::plot::InputType::cancel}); const auto cancelled=a.yRange(); move.y+=20; f.handle(move);
    check(close(a.yRange().min,cancelled.min),"focus loss cancels drag");
    auto hit=a.legendRect(0); press.x=hit.x+2; press.y=hit.y+2;
    f.handle(press); check(!a.series(0).isVisible(),"legend hides curve");
    render(f,r); const auto hidden=r.selectedPoints();
    f.handle(press); render(f,r);
    check(a.series(0).isVisible() && r.selectedPoints()>hidden,"legend restores curve and drawing");
    f.handle(key(ptgl::plot::Key::thicker)); check(a.series(0).lineWidth()==2.5f,"line width shortcut");
    f.handle(key(ptgl::plot::Key::pause)); check(f.isSourcePaused(),"pause signal");
    f.handle(key(ptgl::plot::Key::reset));
    check(f.isFollowingLatest() && close(a.yRange().min,-5) && close(f.xRange().span(),10),"reset restores configured limits");
    f.handle(key(ptgl::plot::Key::live)); check(f.isFollowingLatest(),"live shortcut");
    // Input outside plots does not steal host events.
    press.x=0; press.y=0; check(!f.handle(press),"host input passthrough");
    r.begin(40,40); f.render(r,{0,0,40,40});
    check(a.plotRect().w==0,"small window invalidates stale hit regions");
}
void autoscaleAndLifetime() {
    ptgl::plot::Figure f(2,16,10); f.setLineWidth(3);
    auto& a=f.addAxes(); a.addSeries(0,"signal"); a.addSeries(1,"hidden");
    check(a.series(0).lineWidth()==3,"new plots inherit default width");
    a.series(1).setVisible(false);
    for (int i=0;i<10;++i) {
        const float row[]{i==5 ? 200.f : 1.f,10000.f}; f.append(i,row);
    }
    f.setFollowLatest(false); f.setXRange({2,8});
    check(f.fitY(0) && a.yRange().max>200 && a.yRange().max<220,"autoscale retains one-sample peak and ignores hidden series");
    f.setXRange({6,8}); check(f.fitY(0) && a.yRange().valid() && a.yRange().max<5,"constant data, excludes out-of-view peaks");
    const auto before=a.yRange(); a.series(0).setVisible(false);
    check(!f.fitY(0) && close(a.yRange().max,before.max),"no visible values keeps Y unchanged");
    auto moved=std::move(f);
    RecordingRenderer r; r.initialize(); render(moved,r);
    moved.clear(); check(moved.data().size()==0 && moved.isFollowingLatest(),"clear resets data and view");
    moved.axes(0).series(0).setVisible(true);
    const float missing[]{NAN,NAN}; moved.append(1,missing);
    check(!moved.fitY(0),"missing-only data has no range");
    const float row[]{3,4}; moved.append(2,row); render(moved,r);
    check(close(moved.xRange().span(),10),"startup with few samples keeps window width");
    bool rejected=false;
    try { moved.axes(0).addSeries(2); } catch (const std::out_of_range&) { rejected=true; }
    check(rejected,"invalid channel rejected at registration");
    rejected=false; try { moved.setXRange({1,1}); } catch (const std::invalid_argument&) { rejected=true; }
    check(rejected,"invalid axis range rejected");
    rejected=false; try { moved.setLineWidth(NAN); } catch (const std::invalid_argument&) { rejected=true; }
    check(rejected,"invalid line width rejected");
    check(!moved.append(2,row),"duplicate sample rejected");
}
void visibility() {
    ptgl::plot::Figure f(3,4,3);
    auto& a=f.addAxes(); a.addSeries(2,"C"); a.addSeries(0,"A");
    auto& b=f.addAxes(); b.addSeries(2,"C again");
    RecordingRenderer r; r.initialize();
    for (int i=0;i<4;++i) { const float row[]{float(i),0,float(100+i)}; f.append(i,row); }
    render(f,r); const auto all=r.selectedPoints();
    a.setSeriesVisible(0,false); render(f,r);
    check(!a.isSeriesVisible(0) && a.isSeriesVisible(1) && b.isSeriesVisible(0),"visibility is per registration, independent of channel and other panels");
    check(r.selectedPoints()<all,"hidden series is excluded from rendering");
    check(f.fitY(0) && a.yRange().max<4,"hidden series is excluded from fit");
    a.setSeriesVisible(1,false); b.setSeriesVisible(0,false); render(f,r);
    check(r.selectedPoints()==0,"all series can be hidden");
    bool empty_label=false;
    for (const auto& text:r.drawList().texts) empty_label |= text.value=="All series hidden";
    check(empty_label,"all-hidden state remains understandable");
    for (int i=4;i<8;++i) { const float row[]{float(i),0,float(100+i)}; f.append(i,row); }
    check(f.data().size()==4 && f.data().sample(2,3).value==107,"hidden channels continue receiving bounded data");
    a.setSeriesVisible(0); render(f,r);
    check(a.isSeriesVisible(0) && r.selectedPoints()>0 && f.fitY(0) && a.yRange().max>107,"restore uses latest samples after overwrite");
    a.setSeriesVisible(0,false); f.reset();
    check(!a.isSeriesVisible(0),"axis reset preserves explicit visibility");
    render(f,r); const auto hit=a.legendRect(0);
    ptgl::plot::InputEvent click{ptgl::plot::InputType::press}; click.left=true; click.x=hit.x+2; click.y=hit.y+2;
    f.handle(click); check(a.isSeriesVisible(0),"checkbox restores a programmatically hidden series");
    click.x=hit.x+30; f.handle(click); check(!a.isSeriesVisible(0),"label toggles the same visibility state");
    bool rejected=false;
    try { a.setSeriesVisible(2,false); } catch (const std::out_of_range&) { rejected=true; }
    check(rejected && !a.isSeriesVisible(0),"invalid series index does not change visibility");
}
void liveZoom() {
    ptgl::plot::Figure f(1,100,10); auto& a=f.addAxes(); a.addSeries(0);
    for (int i=0;i<100;++i) { const float row[]{float(i)}; f.append(i*.1,row); }
    RecordingRenderer r; r.initialize(); render(f,r);
    const auto p=a.plotRect();
    ptgl::plot::InputEvent wheel{ptgl::plot::InputType::scroll};
    wheel.x=p.x+p.w*.25f; wheel.y=p.y+p.h*.5f; wheel.scroll_y=3;
    f.handle(wheel);
    check(!f.isFollowingLatest() && !f.isLiveZoomEnabled(),"normal zoom still enters inspection");
    const double zoomed=f.xRange().span();
    const auto toggle=f.liveZoomRect();
    ptgl::plot::InputEvent click{ptgl::plot::InputType::press}; click.left=true; click.x=toggle.x+2; click.y=toggle.y+2;
    check(f.handle(click),"live zoom checkbox consumes click"); render(f,r);
    check(f.isLiveZoomEnabled() && f.isFollowingLatest() && close(f.xRange().span(),zoomed) && close(f.xRange().max,9.9),
          "enabling live zoom resumes at latest sample without resetting zoom");
    f.handle(wheel); render(f,r);
    const double width=f.xRange().span();
    check(width<zoomed && f.isFollowingLatest() && close(f.xRange().max,9.9),"live wheel zoom is anchored at latest sample");
    for (int i=100;i<400;++i) {
        const float row[]{float(i)}; f.append(i*.1,row); render(f,r);
        check(close(f.xRange().max,i*.1) && close(f.xRange().span(),width),"streaming and ring overwrite preserve zoom width");
        check(r.selectedPoints()>0 && !r.overflowed(),"zoomed streaming keeps drawing data");
    }
    check(f.data().overwritten()==300,"follow test exercises ring wraparound");
    wheel.scroll_y=-2; f.handle(wheel);
    check(f.isFollowingLatest() && f.xRange().span()>width && close(f.xRange().max,39.9),"zooming out also preserves follow");
    const auto before_y_zoom=f.xRange(); wheel.shift=true; f.handle(wheel);
    check(f.isFollowingLatest() && close(f.xRange().min,before_y_zoom.min) && close(f.xRange().max,before_y_zoom.max),"Y zoom does not change time follow");
    f.setSourcePaused(true); const auto paused=f.xRange(); render(f,r);
    f.setLiveZoomEnabled(true); render(f,r);
    check(f.isSourcePaused() && close(f.xRange().min,paused.min) && close(f.xRange().max,paused.max),"live zoom does not resume paused source");
    f.setSourcePaused(false);
    click.x=p.x+p.w*.5f; click.y=p.y+p.h*.5f; f.handle(click);
    check(!f.isFollowingLatest() && f.isLiveZoomEnabled(),"drag temporarily leaves follow but retains live zoom preference");
    auto move=click; move.type=ptgl::plot::InputType::move; move.x+=10; f.handle(move);
    f.handle({ptgl::plot::InputType::cancel});
    const auto inspection=f.xRange(); const float row[]{400}; f.append(40,row); render(f,r);
    check(close(f.xRange().min,inspection.min) && close(f.xRange().max,inspection.max),"inspection stays put while new samples arrive");
    wheel.shift=false; wheel.scroll_y=1; f.handle(wheel);
    check(!f.isFollowingLatest(),"wheel during inspection does not jump to latest sample");
    const double inspected_width=f.xRange().span();
    f.handle(click); // Leave an active drag for the resume operation to cancel.
    f.handle(key(ptgl::plot::Key::live)); render(f,r);
    const auto y=a.yRange(); move.y+=20; f.handle(move);
    check(f.isFollowingLatest() && close(f.xRange().span(),inspected_width) && close(f.xRange().max,40) && close(a.yRange().min,y.min),
          "L resumes zoomed follow and cancels an active drag");
    f.handle(key(ptgl::plot::Key::live_zoom)); check(!f.isLiveZoomEnabled(),"F disables live zoom");
    f.handle(wheel); check(!f.isFollowingLatest(),"disabled live zoom restores inspection on wheel");
    f.handle(key(ptgl::plot::Key::live_zoom)); f.reset(); render(f,r);
    check(f.isLiveZoomEnabled() && f.isFollowingLatest() && close(f.xRange().span(),10),"reset restores initial width and retains zoom preference");
    wheel.scroll_y=0; check(!f.handle(wheel) && f.isFollowingLatest(),"zero wheel delta does not change mode");
}
void seriesHandles() {
    ptgl::plot::Figure f(2,16,10);
    f.setTitle("Robot").setColumns(2).setLineWidth(3);
    auto& axes=f.addAxes("Arm","deg",{-20,20});
    auto& joint=axes.addSeries(1,"Joint");
    joint.setColor(.2f,.4f,.6f).setLineWidth(4).setLabel("Elbow");
    axes.addSeries(0,"Other").setVisible(false);
    for (int i=0;i<4;++i) f.addAxes().addSeries(0);
    check(&joint==&f.axes(0).series(0) && joint.channel()==1,"addSeries returns a stable handle to the registered channel");
    const float row[]{5,10}; f.append(1,row);
    RecordingRenderer r; r.initialize(); render(f,r);
    bool label=false, color=false;
    for (const auto& text:r.drawList().texts) label |= text.value=="Elbow";
    for (const auto& command:r.drawList().commands) color |= close(command.color.r,.2f) && close(command.color.g,.4f) && close(command.color.b,.6f);
    check(label && color && joint.lineWidth()==4,"series setters change actual rendered label, color and width");
    joint.setVisible(false); render(f,r); const auto hidden=r.selectedPoints();
    joint.setVisible(); render(f,r);
    check(r.selectedPoints()>hidden,"series handle visibility controls rendering");
    bool rejected=false;
    try { joint.setStrokeStyle({NAN,false}); } catch (const std::invalid_argument&) { rejected=true; }
    check(rejected && joint.lineWidth()==4 && joint.strokeStyle().antialias,"invalid style leaves the previous valid style intact");
    auto& xy=f.addXyAxes("XY").addTrajectory(4,"Tool");
    xy.setMarkerSize(7);
    auto style=xy.xyStyle(); style.simplify_pixels=-1;
    rejected=false; try { xy.setXyStyle(style); } catch (const std::invalid_argument&) { rejected=true; }
    check(rejected && xy.xyStyle().marker_size==7 && xy.xyStyle().simplify_pixels>=0,"invalid XY settings preserve the drawable style");
    xy.appendXy(1,2); xy.appendXy(2,3);
    auto moved=std::move(f);
    check(&joint==&moved.axes(0).series(0) && xy.xyData().size()==2,"moving the owning figure preserves series handles");
}
}
int main() {
    try { interaction(); autoscaleAndLifetime(); visibility(); liveZoom(); seriesHandles(); std::cout << "figure tests passed\n"; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
