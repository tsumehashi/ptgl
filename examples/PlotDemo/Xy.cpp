#include <ptgl/Plot/Viewer.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <utility>

int main(int argc, char** argv) {
    ptgl::plot::Figure figure;
    figure.setTitle("PTGL Plot / STATIC XY / CURVES AND SCATTER").setColumns(2);
    std::vector<ptgl::plot::XYPoint> curve, samples, loop;
    for (int i=0;i<=1000;++i) {
        const double x=-4+i*.008, t=i*(6.283185307179586/1000);
        curve.push_back({x,std::sin(x)});
        if (i%50==0) samples.push_back({x,std::sin(x)+.08*std::cos(x*7)});
        loop.push_back({std::sin(t),std::sin(2*t)});
    }
    auto& graph=figure.addXyAxes("Function and measured points","X","Y");
    graph.setXRange({-4.5,4.5}).setYRange({-1.4,1.4});
    graph.addXySeries(std::move(curve),"sin(x)");
    graph.addScatterSeries(std::move(samples),"Samples").setMarkerSize(7);
    figure.addXyAxes("Parametric curve / input order preserved","X","Y")
          .setXRange({-1.3,1.3}).setYRange({-1.3,1.3}).setEqualAspectEnabled().addXySeries(std::move(loop),"Figure eight");
    ptgl::plot::ViewerOptions options; options.title="PTGL Plot - Static XY"; options.height=720;
    for (int i=1;i+1<argc;++i)
        if (std::strcmp(argv[i],"--smoke-frames")==0) options.smoke_frames=unsigned(std::strtoul(argv[i+1],nullptr,10));
    return ptgl::plot::show(std::move(figure),{},options);
}
