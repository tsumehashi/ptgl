#include "ptgl/Plot/Figure.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {
void check(bool ok,const char* message) { if (!ok) throw std::runtime_error(message); }
bool close(double a,double b,double tolerance=1e-5) { return std::abs(a-b)<tolerance; }
class RecordingRenderer final:public ptgl::plot::Renderer {
public:
    void initialize(int n=1024*1024) override { configure(n); }
    void shutdown() override {}
    void draw() override {}
};
void render(ptgl::plot::Figure& f,RecordingRenderer& r) { r.begin(1000,700,1); f.render(r,{0,0,1000,700}); }
ptgl::plot::Vertex2D center(const ptgl::plot::DrawList& f,std::size_t i) {
    return {(f.vertices[2*i].x+f.vertices[2*i+1].x)*.5f,(f.vertices[2*i].y+f.vertices[2*i+1].y)*.5f};
}
void buffers() {
    ptgl::plot::XYBuffer data(3);
    for (int i=0;i<5;++i) data.append({double(5-i),double(i),i==3});
    check(data.size()==3 && data.capacity()==3 && data.overwritten()==2,"bounded XY storage");
    check(data.sample(0).x==3 && data.sample(2).x==1 && data.sample(1).break_before,"ring retains insertion order and breaks");
    data.clear(); data.append({NAN,2});
    check(data.size()==1 && data.overwritten()==0 && std::isnan(data.sample(0).x),"clear and missing samples");
    ptgl::plot::XYBuffer owned(std::vector<ptgl::plot::XYPoint>{{2,3},{1,4}});
    check(owned.size()==2 && owned.sample(1).x==1,"static data keeps non-monotone X");
    owned.append({0,5}); check(owned.sample(0).x==1 && owned.sample(1).x==0,"owned XY buffer is a valid ring");
    ptgl::plot::XYBuffer empty(std::vector<ptgl::plot::XYPoint>{}); empty.append({1,2});
    check(empty.size()==1,"empty static data remains a valid buffer");
    bool rejected=false; try { ptgl::plot::XYBuffer invalid(0); } catch (const std::invalid_argument&) { rejected=true; }
    check(rejected,"zero capacity rejected");
}
void geometry() {
    RecordingRenderer r; r.initialize(); r.begin(800,800);
    const ptgl::plot::Rect plot{100,200,400,400};
    ptgl::plot::XYBuffer loop(std::vector<ptgl::plot::XYPoint>{{1,0},{0,1},{-1,0},{0,-1},{1,0}});
    ptgl::plot::XYStyle exact; exact.simplify_pixels=0;
    r.xy(loop,{-2,2},{-2,2},plot,{1,0,0},{2,false},exact);
    const auto& list=r.drawList();
    check(list.commands.size()==1 && list.vertices.size()==10,"closed XY path has one ordered strip");
    const ptgl::plot::Vertex2D expected[]{{400,400},{300,300},{200,400},{300,500},{400,400}};
    for (std::size_t i=0;i<5;++i) {
        const auto c=center(list,i); check(close(c.x,expected[i].x) && close(c.y,expected[i].y),"XY loops are never sorted by X");
    }
    check(list.commands[0].clip.x==100 && list.commands[0].clip.h==400,"XY uses plot clipping");
    ptgl::plot::XYBuffer gaps(std::vector<ptgl::plot::XYPoint>{{0,0},{1,1},{NAN,2},{1,0},{-1,0,true},{0,0}});
    r.begin(800,800); r.xy(gaps,{-2,2},{-2,2},plot,{1,0,0},{2,false},exact);
    check(list.commands.size()==3 && list.commands[0].count==4 && list.commands[1].count==6 && list.commands[2].count==4,
          "NaN and explicit breaks split runs and preserve isolated points");
    ptgl::plot::XYBuffer epoch(std::vector<ptgl::plot::XYPoint>{{1e12,1e12},{1e12+.01,1e12+.02}});
    const ptgl::plot::Range range{1e12,1e12+.04};
    r.begin(800,800); r.xy(epoch,range,range,plot,{1,0,0},{2,false},exact);
    const auto c=center(list,1);
    check(close(c.x,100+(.01+1e12-1e12)/range.span()*400,.02) && c.y<450,"double XY precision survives a large coordinate origin");
    ptgl::plot::XYBuffer scatter(std::vector<ptgl::plot::XYPoint>{{.5,0},{-.5,1},{NAN,0},{100,100}});
    ptgl::plot::XYStyle style; style.mode=ptgl::plot::XYMode::scatter; style.marker_size=7;
    r.begin(800,800); r.xy(scatter,{-2,2},{-2,2},plot,{1,0,0},{2,true},style);
    check(list.commands.size()==1 && list.commands[0].primitive==ptgl::plot::Primitive::triangles && list.vertices.size()==144,
          "visible scatter markers use a single batch and no connecting line");
    style.mode=ptgl::plot::XYMode::line_scatter;
    r.begin(800,800); r.xy(loop,{-2,2},{-2,2},plot,{1,0,0},{2,false},style);
    check(list.commands.size()==2 && list.commands[1].count==5*72,"line plus markers");
    style=exact; style.show_latest=true;
    r.begin(800,800); r.xy(loop,{-2,2},{-2,2},plot,{1,0,0},{2,false},style);
    check(list.commands.size()==2 && list.commands.back().count==72,"latest trajectory point is marked");
    loop.append({NAN,0});
    r.begin(800,800); r.xy(loop,{-2,2},{-2,2},plot,{1,0,0},{2,false},style);
    check(list.commands.size()==1,"a missing latest point does not leave a stale head marker");
    ptgl::plot::XYBuffer ring(3);
    for (int i=0;i<5;++i) ring.append({double(i),double(i)});
    r.begin(800,800); r.xy(ring,{0,5},{0,5},plot,{1,0,0},{2,false},exact);
    check(list.vertices.size()==6 && close(center(list,0).x,260) && close(center(list,2).x,420),"ring wrap never connects newest point back to oldest");
}
void densityAndBudget() {
    RecordingRenderer r; r.initialize(); r.begin(800,800);
    ptgl::plot::XYBuffer dense(10001);
    for (int i=0;i<=10000;++i) dense.append({i*.0001,0});
    r.xy(dense,{0,1},{-1,1},{0,0,500,400},{1,0,0},{2,false});
    const auto& f=r.drawList();
    check(r.selectedPoints()<2500 && r.selectedPoints()>1000,"screen distance simplifies a dense ordered curve");
    check(close(center(f,0).x,0) && close(center(f,r.selectedPoints()-1).x,500),"simplification preserves both endpoints");
    ptgl::plot::XYStyle exact; exact.simplify_pixels=0;
    r.begin(800,800); r.xy(dense,{0,1},{-1,1},{0,0,500,400},{1,0,0},{2,false},exact);
    check(r.selectedPoints()==10001,"simplification can be disabled");
    RecordingRenderer small; small.initialize(80); small.begin(800,800);
    ptgl::plot::XYBuffer line(std::vector<ptgl::plot::XYPoint>{{0,0},{1,1},{2,0},{3,1},{4,0}});
    exact.show_latest=true;
    small.xy(line,{0,5},{0,5},{0,0,500,400},{1,0,0},{2,false},exact);
    check(small.overflowed() && small.drawList().vertices.empty(),"line and latest marker overflow atomically");
    exact.mode=ptgl::plot::XYMode::scatter;
    small.begin(800,800); small.xy(line,{0,5},{0,5},{0,0,500,400},{1,0,0},{2,false},exact);
    check(small.overflowed() && small.drawList().commands.empty(),"scatter respects mesh budget");
}
void interaction() {
    ptgl::plot::Figure f(1,100,10); f.setColumns(2);
    auto& a=f.addXyAxes("Path","x","y");
    a.setXRange({-2,2}).setYRange({-1,1}).setEqualAspectEnabled().addXySeries({{-1,-.5},{1,.5}},"Static");
    auto& stable=a.series(0);
    a.addTrajectory(4,"Trail"); check(&stable==&a.series(0),"series handles survive additions");
    auto& trail=a.series(1);
    for (int i=0;i<8;++i) trail.appendXy(i*.1,i*.05);
    check(trail.xyData().size()==4 && trail.xyData().overwritten()==4,"trajectory owns bounded storage");
    f.addAxes("Time").addSeries(0); f.setLiveZoomEnabled(true);
    for (int i=0;i<100;++i) { const float row[]{float(i)}; f.append(i*.1,row); }
    RecordingRenderer r; r.initialize(); render(f,r);
    const auto p=a.plotRect();
    check(close(p.w/a.xRange().span(),p.h/a.yRange().span()),"equal aspect uses identical X and Y pixel scales");
    const auto time=f.xRange(), initial_x=a.xRange(), initial_y=a.yRange();
    ptgl::plot::InputEvent e{ptgl::plot::InputType::scroll}; e.x=p.x+p.w*.5f; e.y=p.y+p.h*.5f; e.scroll_y=2;
    f.handle(e);
    check(a.xRange().span()<initial_x.span() && a.yRange().span()<initial_y.span(),"XY wheel zooms both axes");
    check(f.isFollowingLatest() && close(f.xRange().span(),time.span()),"XY zoom leaves time follow unchanged");
    const auto x=a.xRange(); e.shift=true; f.handle(e);
    check(close(a.xRange().span(),x.span()),"shift wheel changes only Y");
    e.type=ptgl::plot::InputType::press; e.left=true; f.handle(e);
    const auto y=a.yRange(); e.type=ptgl::plot::InputType::move; e.x+=10; e.y+=10; f.handle(e);
    check(a.xRange().min<x.min && a.yRange().min>y.min && f.isFollowingLatest(),"XY pan is independent of time follow");
    f.handle({ptgl::plot::InputType::cancel});
    const auto zoom=a.xRange(); trail.appendXy(-.8,-.6); render(f,r);
    check(close(a.xRange().min,zoom.min) && close(a.xRange().span(),zoom.span()),"trajectory updates preserve manual zoom");
    a.setSeriesVisible(0,false); check(f.fitXy(0) && a.xRange().max<1,"XY fit ignores hidden series");
    a.setSeriesVisible(1,false); check(!f.fitXy(0),"hidden-only XY fit leaves limits unchanged");
    a.setSeriesVisible(0); stable.setXyData({{10,20},{12,22}});
    check(f.fitXy(0) && a.xRange().min<10 && a.xRange().max>12 && a.yRange().max>22,"static replacement and XY fit");
    f.reset(); check(close(a.xRange().min,-2) && close(a.yRange().max,1),"reset restores configured XY limits");
    bool rejected=false; try { stable.appendXy(0,0); } catch (const std::logic_error&) { rejected=true; }
    check(rejected,"static series cannot accidentally overwrite its data");
    rejected=false; try { trail.setXyData({{0,0}}); } catch (const std::logic_error&) { rejected=true; }
    check(rejected,"trajectory capacity cannot accidentally change via static replacement");
    rejected=false; try { a.addSeries(0); } catch (const std::logic_error&) { rejected=true; }
    check(rejected,"time series rejected on XY axes");
    auto moved=std::move(f); render(moved,r); moved.clear();
    check(moved.axes(0).series(0).xyData().size()==0 && moved.axes(0).series(1).xyData().size()==0,"figure move and clear include XY data");
    ptgl::plot::Figure only_xy; only_xy.addXyAxes().addXySeries({{0,0},{1,1}}); render(only_xy,r);
    check(only_xy.liveZoomRect().w==0 && r.selectedPoints()>0,"XY-only figures render without time samples or time controls");
    ptgl::plot::Figure automatic;
    auto& axes=automatic.addXyAxes();
    axes.addXySeries({{100,200},{110,220}});
    check(axes.xRange().min<100 && axes.xRange().max>110 && axes.yRange().max>220,"static XY initially fits its data");
    axes.setXRange({0,1}); axes.addScatterSeries({{300,400}});
    check(close(axes.xRange().min,0) && close(axes.xRange().max,1) && axes.yRange().max>400,"static additions respect explicit ranges and fit unset axes");
    axes.series(0).setXyData({{-100,-100}});
    check(close(axes.xRange().max,1),"static replacement keeps the user's viewport");
}
}
int main() {
    try { buffers(); geometry(); densityAndBudget(); interaction(); std::cout<<"XY tests passed\n"; }
    catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
