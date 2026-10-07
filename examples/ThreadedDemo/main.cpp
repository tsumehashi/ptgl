#include "ptgl/Core/QuickGraphicsView.h"
#include "ptgl/Core/SphericalCamera.h"
#include "ptgl/Driver/GLFWGraphicsDriver.h"
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>
#include <vector>

namespace {
struct State {
    std::atomic<bool> stop{false}, paused{false};
    std::array<std::atomic<bool>, 3> visible{{true, true, true}};
    std::array<std::atomic<unsigned>, 3> sent{};
    // Accessed only by posted operations and the view's text callback.
    std::array<unsigned, 3> received{};
};

class Workers {
public:
    std::shared_ptr<State> state = std::make_shared<State>();
    ~Workers()
    {
        state->stop = true;
        for (auto& thread : threads_) if (thread.joinable()) thread.join();
    }
    void start(ptgl::RenderCommandSender sender)
    {
        threads_.reserve(3);
        for (int i = 0; i < 3; ++i) {
            std::thread thread([state = state, sender, i] {
                const std::string id = "worker-" + std::to_string(i + 1);
                const std::array<std::array<double, 3>, 3> colors{{{{.86, .24, .13}}, {{.16, .52, .83}}, {{.95, .64, .12}}}};
                unsigned step = 0;
                while (!state->stop && sender.isOpen()) {
                    if (!state->visible[i]) {
                        sender.clearFrame(id);
                    } else if (!state->paused) {
                        const double angle = ++step * .025;
                        const double p[] = {(i - 1) * 2.4, 0, .85 + .35 * std::sin(angle)};
                        const Eigen::Matrix3d rotation = Eigen::AngleAxisd(angle, Eigen::Vector3d::UnitZ()).toRotationMatrix();
                        const double size[] = {1.4, 1.4, 1.4};
                        ptgl::Render3DItem commands;
                        commands.setColor(colors[i][0], colors[i][1], colors[i][2]);
                        commands.setMaterial({.3, .04});
                        if (i == 0) commands.drawSphere(p, rotation.data(), .7);
                        if (i == 1) commands.drawRoundedBox(p, rotation.data(), size, .15);
                        if (i == 2) commands.drawRoundedCylinder(p, rotation.data(), 1.4, .65, .12);
                        const auto result = sender.submitFrame(id, std::move(commands));
                        if (result == ptgl::SubmitResult::Closed) break;
                        if (result == ptgl::SubmitResult::Accepted) state->sent[i] = step;
                        if (step % 20 == 1) {
                            sender.post([state, i, step](ptgl::GraphicsView&) { state->received[i] = step; });
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10 + i * 9));
                }
            });
#ifdef __EMSCRIPTEN__
            // Workers own their data and stop when the sender closes. Never join
            // them from the browser's main thread, which must keep servicing events.
            thread.detach();
#else
            threads_.push_back(std::move(thread));
#endif
        }
    }
private:
    std::vector<std::thread> threads_;
};
}

int main()
{
    using namespace ptgl;
    QuickStyledGraphicsView view(std::make_unique<GLFWGraphicsDriver>(GLFWGraphicsDriver::ExecutionMode::CallingThread));
    view.setWindowTitle("ThreadedDemo - three independent drawing producers");
    view.setWindowSize(1280, 800);
    view.setBackgroundColor(.15, .18, .23);
    view.setDrawWorldGrid(false);
    auto camera = std::make_shared<SphericalCamera>();
    view.setCamera(camera);
    view.setInitProcessFunction([camera] {
        camera->setCenter(Eigen::Vector3d(0, 0, .6));
        camera->setDistance(12);
        camera->setHeading(25);
        camera->setElevation(25);
    });
    auto shadows = view.shadowSettings(); shadows.autoFit = true; view.setShadowSettings(shadows);
    auto quality = view.renderQualitySettings(); quality.cacheStaticEdges = true; view.setRenderQualitySettings(quality);
    view.setRenderSceneFunction([](Renderer3D* r) {
        r->setColor(.68, .71, .76);
        r->drawBox({0, 0, -.15}, r->R0(), {9, 5, .3});
    });
    Workers workers;
    auto state = workers.state;
    view.setKeyPressEventFunction([&view, state](KeyEvent* e) {
        if (e->keyAction() != KeyEvent::KeyAction::KeyPress) return;
        if (e->key() == Key::Key_Space) {
            using Style = StyledGraphicsView::RenderStyle;
            view.setRenderStyle(view.renderStyle() == Style::Plastic ? Style::CAD :
                                view.renderStyle() == Style::CAD ? Style::Legacy : Style::Plastic);
        }
        if (e->key() == Key::Key_P) state->paused = !state->paused;
        const std::array<Key, 3> keys{{Key::Key_1, Key::Key_2, Key::Key_3}};
        for (int i = 0; i < 3; ++i) if (e->key() == keys[i]) state->visible[i] = !state->visible[i];
        if (e->key() == Key::Key_Escape) view.terminate();
    });
    view.setRenderTextSceneFunction([state, &view](TextRenderer* r) {
        r->setTextColor(1, 1, 1);
        using Style = StyledGraphicsView::RenderStyle;
        const char* style = view.renderStyle() == Style::Plastic ? "Plastic" : view.renderStyle() == Style::CAD ? "CAD" : "Legacy";
        r->drawText(20, 25, std::string(style) + " | Space: style | P: pause | 1/2/3: show/hide | Esc: stop");
        for (int i = 0; i < 3; ++i)
            r->drawText(20, 55 + i * 25, "Worker " + std::to_string(i + 1) + " | submitted: " +
                std::to_string(state->sent[i].load()) + " | posted: " + std::to_string(state->received[i]) +
                (state->visible[i] ? " | visible" : " | hidden"));
    });
    view.initialize();
    workers.start(view.commandSender());
    view.execute(); // Native: returns after window closes. Web: yields to the browser.
    view.waitUntilStopped();
    return 0;
}
