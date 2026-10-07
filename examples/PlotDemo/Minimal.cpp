#include <ptgl/Plot/Viewer.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <utility>

int main(int argc, char** argv) {
    ptgl::plot::Figure figure(2, 10000, 10); // 2 channels, 10,000 rows, 10-second view.
    figure.setTitle("Two joint angles").setLineWidth(3);
    auto& axes = figure.addAxes("Arm", "deg", {-100, 100});
    axes.addSeries(0, "Shoulder").setColor(.137f, .404f, .690f);
    axes.addSeries(1, "Elbow");
    ptgl::plot::ViewerOptions options;
    for (int i = 1; i+1 < argc; ++i)
        if (std::strcmp(argv[i], "--smoke-frames") == 0) options.smoke_frames = unsigned(std::strtoul(argv[i+1],nullptr,10));
    return ptgl::plot::show(std::move(figure),
        [tick = std::uint64_t{0}, remainder = 0.0](ptgl::plot::Figure& f, double elapsed) mutable {
            // Demo data at 1 kHz. A device integration would drain its input queue here.
            // Keep work bounded after browser suspension; synthetic time pauses then.
            remainder += std::min(elapsed, .25)*1000;
            auto count = std::uint64_t(remainder); remainder -= double(count);
            while (count--) {
                const double t = double(tick++)*.001;
                f.append(t, std::array<float, 2>{float(80*std::sin(t)), float(50*std::cos(2*t))});
            }
        }, options);
}
