#include <ptgl/Plot/Viewer.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <utility>

struct Source {
    std::uint64_t tick=0;
    double remainder=0;
    void sample(ptgl::plot::Figure& f) {
        const double t=double(tick++)*.001;
        f.axes(0).series(0).appendXy((1+.12*std::cos(.13*t))*std::cos(1.2*t),
                                     (.8+.1*std::sin(.15*t))*std::sin(1.2*t));
        f.axes(1).series(0).appendXy(std::sin(.35*t),std::sin(.7*t+.2*std::sin(.025*t)));
    }
    void operator()(ptgl::plot::Figure& f,double elapsed) {
        remainder+=elapsed*1000;
        auto count=std::uint64_t(remainder); remainder-=double(count);
        if (count>250) {
            tick+=count-250; count=250;
            for (std::size_t i=0;i<2;++i) f.axes(i).series(0).appendXy(std::numeric_limits<double>::quiet_NaN(),0);
        }
        while (count--) sample(f);
    }
};
int main(int argc,char** argv) {
    ptgl::plot::Figure figure;
    figure.setTitle("PTGL / PLOT XY TRAJECTORIES / 1 kHz").setColumns(2);
    figure.addXyAxes("Tool path / last 4,000 points","X [m]","Y [m]")
          .setXRange({-1.4,1.4}).setYRange({-1.4,1.4}).setEqualAspectEnabled().addTrajectory(4000,"Tool");
    figure.addXyAxes("Phase trajectory / last 60,000 points","Joint 1 [rad]","Joint 2 [rad]")
          .setXRange({-1.3,1.3}).setYRange({-1.3,1.3}).setEqualAspectEnabled().addTrajectory(60000,"Phase");
    Source source;
    for (int i=0;i<60000;++i) source.sample(figure);
    ptgl::plot::ViewerOptions options; options.title="PTGL Plot - Realtime XY"; options.height=720;
    for (int i=1;i+1<argc;++i)
        if (std::strcmp(argv[i],"--smoke-frames")==0) options.smoke_frames=unsigned(std::strtoul(argv[i+1],nullptr,10));
    return ptgl::plot::show(std::move(figure),source,options);
}
