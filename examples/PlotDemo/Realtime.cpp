#include "ptgl/Plot/Viewer.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <utility>

namespace {
constexpr std::size_t channels = 32, capacity = 60000;
struct Source {
    std::uint64_t tick = 0;
    double remainder = 0;
    void sample(ptgl::plot::Figure& figure) {
        std::array<float, channels> row{};
        const double t = double(tick)*.001;
        for (std::size_t c = 0; c < channels; ++c) {
            row[c] = float(95*std::sin(t*(1.2+c*.035)+c*.3)+18*std::sin(t*11+c));
            if ((tick+c*79)%5000 == 0) row[c] = 180;
            if (c%4 == 3 && tick%13000 >= 9000 && tick%13000 < 9200)
                row[c] = std::numeric_limits<float>::quiet_NaN();
        }
        figure.append(t, row); ++tick;
    }
    void operator()(ptgl::plot::Figure& figure, double elapsed) {
        remainder += elapsed*1000;
        auto samples = std::uint64_t(remainder); remainder -= double(samples);
        // Bound catch-up work after tab suspension and mark the skipped interval.
        if (samples > 250) {
            tick += samples-250;
            std::array<float, channels> missing{};
            missing.fill(std::numeric_limits<float>::quiet_NaN());
            figure.append(double(tick-1)*.001, missing); samples = 250;
        }
        while (samples--) sample(figure);
    }
};
}
int main(int argc, char** argv) {
    ptgl::plot::Figure figure(channels, capacity, 60);
    figure.setTitle("PTGL / PLOT JOINT ANGLES / 1 kHz").setColumns(2);
    for (std::size_t p = 0; p < 8; ++p) {
        auto& axes = figure.addAxes("Joints "+std::to_string(p*4+1)+" - "+std::to_string(p*4+4), "deg", {-200,200});
        for (std::size_t c = p*4; c < p*4+4; ++c) {
            char label[16]; std::snprintf(label,sizeof(label),"J%02u",unsigned(c+1));
            axes.addSeries(c,label);
        }
    }
    Source source;
    for (std::size_t i = 0; i < capacity; ++i) source.sample(figure);
    ptgl::plot::ViewerOptions options; options.title = "PTGL Plot - Joint angles";
    for (int i = 1; i+1 < argc; ++i)
        if (std::strcmp(argv[i],"--smoke-frames") == 0) options.smoke_frames = unsigned(std::strtoul(argv[i+1],nullptr,10));
    return ptgl::plot::show(std::move(figure),source,options);
}
