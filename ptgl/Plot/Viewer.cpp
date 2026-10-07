#include "Viewer.h"
#include "PlotGraphicsView.h"
#include "ptgl/Driver/GLFWGraphicsDriver.h"
#include <cstdio>
#include <stdexcept>

namespace ptgl::plot {
namespace {
class Viewer final : public PlotGraphicsView {
public:
    Viewer(GraphicsDriverPtr driver, Figure figure, unsigned frames)
        : PlotGraphicsView(std::move(driver), std::move(figure)), smokeFrames_(frames) {}
protected:
    void postProcess() override {
        if (!smokeFrames_) return;
        const auto error = glGetError();
        if (error != GL_NO_ERROR || renderer().overflowed())
            throw std::runtime_error("Plot smoke: GL error or vertex overflow");
        if (++frames_ >= smokeFrames_) {
            std::printf("ptgl plot smoke: %u frames, %zu selected points\n", frames_, renderer().selectedPoints());
            terminate();
        }
    }
private:
    unsigned smokeFrames_ = 0, frames_ = 0;
};
// Web callbacks outlive show(); finalization releases GPU resources on close.
// The last, stopped viewer is retained until replacement or runtime shutdown.
std::unique_ptr<Viewer> active;
}
int show(Figure figure, Update update, ViewerOptions options) {
    if (active && !active->terminated()) throw std::logic_error("A plot viewer is already running");
    if (options.width <= 0 || options.height <= 0 || options.max_vertices < 6)
        throw std::invalid_argument("Invalid plot viewer options");
    active.reset();
    try {
        auto driver = std::make_unique<GLFWGraphicsDriver>(GLFWGraphicsDriver::ExecutionMode::CallingThread);
        auto* execution = driver.get();
        active = std::make_unique<Viewer>(std::move(driver), std::move(figure), options.smoke_frames);
        active->setWindowTitle(options.title);
        active->setWindowSize(options.width, options.height);
        active->setMaxVertices(options.max_vertices);
        active->setUpdateFunction(std::move(update));
        active->initialize();
        active->execute();
        const int result = execution->executionError() ? 1 : 0;
        active.reset();
        return result;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "ptgl plot: %s\n", error.what());
        active.reset();
        return 1;
    }
}
} // namespace ptgl::plot
