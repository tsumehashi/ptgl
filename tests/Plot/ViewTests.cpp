#include "ptgl/Plot/PlotGraphicsView.h"
#include "ptgl/Core/GraphicsDriver.h"
#include "ptgl/GUI/PushButton.h"
#include "ptgl/GUI/Slider.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
bool close(double a, double b) { return std::abs(a - b) < 1e-7; }
class Driver final : public ptgl::GraphicsDriver {
    void execute() override {}
    void terminate() override {}
    bool terminated() override { return true; }
    void setWindowSize(int w, int h) override { w_ = w; h_ = h; }
    void setWindowTitle(const std::string& title) override { title_ = title; }
    void setFrameRate(int fps) override { fps_ = fps; }
    const std::string& windowTitle() const override { return title_; }
    int width() const override { return w_; }
    int height() const override { return h_; }
    int frameRate() const override { return fps_; }
    std::string title_;
    int w_ = 1000, h_ = 700, fps_ = 60;
};
class View final : public ptgl::plot::PlotGraphicsView {
public:
    View() : PlotGraphicsView(std::make_unique<Driver>(), 1, 1000, 10) {}
    using PlotGraphicsView::executeMousePressEvent;
    using PlotGraphicsView::executeMouseMoveEvent;
    using GraphicsView::executeMouseReleaseEvent;
    using GraphicsView::executeWheelEvent;
    using GraphicsView::executeKeyPressEvent;
    using GraphicsView::executeGraphicsViewPrevProcessEvent;
};
class Recorder final : public ptgl::plot::Renderer {
public:
    void initialize(int count = 1024*1024) override { configure(count); }
    void shutdown() override {}
    void draw() override {}
};
void run() {
    View view;
    view.setWindowSize(1000,700);
    auto& f = view.figure();
    auto& axes = f.addAxes("Test");
    axes.addSeries(0);
    for (int i = 0; i < 1000; ++i) f.append(i*.01, std::array<float,1>{float(std::sin(i*.01))});
    Recorder renderer;
    renderer.initialize(); renderer.begin(1000,700);
    f.render(renderer, {0,0,1000,700});
    const auto plot = axes.plotRect();
    const int x = int(plot.x + plot.w*.5f), y = int(plot.y + plot.h*.5f);

    // Place a real ptgl GUI control over the plot, to test input precedence.
    auto button = std::make_shared<ptgl::gui::PushButton>("Toggle series");
    button->setPos(x, y); button->setSize(130, 30);
    int clicks = 0;
    button->setOnClickedFunction([&](bool) { ++clicks; axes.series(0).setVisible(!axes.series(0).isVisible()); });
    view.addWidget(button);
    ptgl::MouseEvent mouse(&view);
    mouse.setPressEvent(x+4, y+4, ptgl::MouseEvent::MouseButton::LeftButton);
    view.executeMousePressEvent(&mouse);
    check(f.isFollowingLatest(), "GUI press must not start plot pan");
    mouse.setMoveEvent(x+240,y+60,ptgl::MouseEvent::MouseButton::LeftButton);
    view.executeMouseMoveEvent(&mouse);
    check(f.isFollowingLatest(), "GUI drag capture must survive leaving the widget");
    mouse.setReleaseEvent(x+4,y+4,ptgl::MouseEvent::MouseButton::LeftButton);
    view.executeMouseReleaseEvent(&mouse);
    check(clicks == 1 && !axes.series(0).isVisible(), "GUI click toggles plot series");

    ptgl::WheelEvent wheel(&view);
    const auto span = f.xRange().span();
    wheel.setWheelEvent(x+4,y+4,80);
    view.executeWheelEvent(&wheel);
    check(close(f.xRange().span(),span), "GUI consumes wheel over plot");
    wheel.setWheelEvent(x-40,y,0);
    wheel.setScrollSteps(.125); // Trackpad precision must survive integer legacy delta.
    view.executeWheelEvent(&wheel);
    check(f.xRange().span() < span, "precise wheel zooms plot");
    const auto xSpan = f.xRange().span(), ySpan = axes.yRange().span();
    wheel.setModifierKey(ptgl::ModifierKey_Shift);
    view.executeWheelEvent(&wheel);
    check(close(f.xRange().span(), xSpan) && axes.yRange().span() < ySpan, "shift wheel zooms only Y");

    ptgl::KeyEvent key(&view);
    key.setKeyPressEvent(ptgl::Key::Key_L);
    view.executeKeyPressEvent(&key);
    check(!f.isFollowingLatest(), "GUI focus suppresses plot shortcut even if unhandled");
    mouse.setPressEvent(x-40,y,ptgl::MouseEvent::MouseButton::LeftButton);
    view.executeMousePressEvent(&mouse);
    check(!view.keyboardFocus(), "clicking plot clears GUI focus");
    key.setKeyPressEvent(ptgl::Key::Key_L);
    view.executeKeyPressEvent(&key);
    check(f.isFollowingLatest(), "plot shortcut works after focus transfer");

    mouse.setPressEvent(x-40,y,ptgl::MouseEvent::MouseButton::LeftButton);
    view.executeMousePressEvent(&mouse);
    view.cancelInput();
    auto before = axes.yRange();
    mouse.setMoveEvent(x-40,y+50,ptgl::MouseEvent::MouseButton::LeftButton);
    view.executeMouseMoveEvent(&mouse);
    check(close(before.min, axes.yRange().min), "focus loss cancels plot drag");
    mouse.setPressEvent(x+4,y+4,ptgl::MouseEvent::MouseButton::LeftButton);
    view.executeMousePressEvent(&mouse);
    view.cancelInput();
    mouse.setReleaseEvent(x+4,y+4,ptgl::MouseEvent::MouseButton::LeftButton);
    view.executeMouseReleaseEvent(&mouse);
    check(clicks == 1 && !button->isDown(), "focus loss cancels GUI click");

    double elapsed = -1;
    int updates = 0;
    view.setUpdateFunction([&](ptgl::plot::Figure&, double dt) { elapsed = dt; ++updates; });
    view.executeGraphicsViewPrevProcessEvent();
    check(elapsed > 0 && close(elapsed, view.currentTime()-view.previousTime()), "update sees a positive current frame interval");
    f.setSourcePaused(true);
    view.executeGraphicsViewPrevProcessEvent();
    check(updates == 1, "pause suppresses data callback");
    view.setPlotMargins(180,10,20,30);
    check(close(view.plotBounds().w,800) && close(view.plotBounds().h,660), "reserved GUI margins");
    check(button->theme().preset == ptgl::gui::Theme::Preset::Light, "white GUI theme by default");
}
}
int main() {
    try { run(); std::cout << "Plot view interaction tests passed\n"; }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
