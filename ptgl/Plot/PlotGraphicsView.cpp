#include "PlotGraphicsView.h"
#include "ptgl/Driver/GLFWGraphicsDriver.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ptgl::plot {
namespace {
InputEvent pointerEvent(InputType type, MouseEvent* source) {
    InputEvent event{};
    event.type = type;
    event.x = float(source->x());
    event.y = float(source->y());
    event.left = source->button() == MouseEvent::MouseButton::LeftButton;
    event.shift = (source->modifierKey() & ModifierKey_Shift) != 0;
    return event;
}
}
PlotGraphicsView::PlotGraphicsView(std::size_t channels, std::size_t capacity, double seconds)
    : PlotGraphicsView(std::make_unique<GLFWGraphicsDriver>(GLFWGraphicsDriver::ExecutionMode::CallingThread),
                       channels, capacity, seconds) {}
PlotGraphicsView::PlotGraphicsView(GraphicsDriverPtr driver, std::size_t channels,
                                   std::size_t capacity, double seconds)
    : PlotGraphicsView(std::move(driver), Figure(channels, capacity, seconds)) {}
PlotGraphicsView::PlotGraphicsView(GraphicsDriverPtr driver, Figure figure)
    : Graphics2DView(std::move(driver)), figure_(std::move(figure)) {
    if (!driver_) throw std::invalid_argument("PlotGraphicsView needs a graphics driver");
    setEnableCameraManipulate(false);
    setWindowTitle("ptgl Plot");
    setWindowSize(1280, 960);
    setSwapInterval(1);
    setFrameRate(0); // Display refresh rate natively, requestAnimationFrame on Web.
    const auto color = figure_.theme().background;
    setBackgroundColor(color.r, color.g, color.b, color.a);
}
PlotGraphicsView::~PlotGraphicsView() {
    // Stop before destroying callbacks and GPU resources owned by this subclass.
    terminate();
    waitUntilStopped();
}
void PlotGraphicsView::setPlotMargins(float left, float top, float right, float bottom) {
    for (float value : {left, top, right, bottom})
        if (!std::isfinite(value) || value < 0) throw std::invalid_argument("Invalid plot margin");
    margins_ = {left, top, right, bottom};
}
Rect PlotGraphicsView::plotBounds() const {
    const float scale = std::max(.01f, pixelRatio());
    return {margins_[0] * scale, margins_[1] * scale,
            std::max(0.f, float(width()) - (margins_[0] + margins_[2]) * scale),
            std::max(0.f, float(height()) - (margins_[1] + margins_[3]) * scale)};
}
void PlotGraphicsView::addWidget(gui::WidgetPtr widget) {
    if (!widget) return;
    auto theme = guiTheme_;
    theme.scale *= pixelRatio();
    widget->setTheme(theme);
    addGraphicsItem(std::move(widget));
}
void PlotGraphicsView::setGuiTheme(const gui::Theme& theme) {
    theme.validate();
    guiTheme_ = theme;
    guiPixelRatio_ = -1;
    updateGuiScale();
}
void PlotGraphicsView::updateGuiScale() {
    const float scale = pixelRatio();
    if (scale == guiPixelRatio_) return;
    auto theme = guiTheme_;
    theme.scale *= scale;
    for (auto& item : graphicsItems())
        if (auto widget = dynamic_cast<gui::Widget*>(item.get())) widget->setTheme(theme);
    guiPixelRatio_ = scale;
    updateGui();
}
void PlotGraphicsView::setMaxVertices(int count) {
    if (count < 6) throw std::invalid_argument("Plot vertex capacity must be at least six");
    if (initialized_) throw std::logic_error("Set plot vertex capacity before execute()");
    maxVertices_ = count;
}
void PlotGraphicsView::executeInitializeEvent() {
    renderer_.initialize(maxVertices_);
    Graphics2DView::executeInitializeEvent();
}
void PlotGraphicsView::executeFinalizeEvent() {
    renderer_.shutdown();
    Graphics2DView::executeFinalizeEvent();
}
void PlotGraphicsView::executePrevProcess() {
    updateGuiScale();
    Graphics2DView::executePrevProcess();
    if (update_ && !figure_.isSourcePaused()) update_(figure_, deltaTime());
}
void PlotGraphicsView::renderContents() {
    const float scale = std::max(.01f, pixelRatio());
    renderer_.begin(float(width()), float(height()), scale);
    figure_.render(renderer_, plotBounds(), scale);
    renderer_.draw();
}
void PlotGraphicsView::executeMousePressEvent(MouseEvent* event) {
    Graphics2DView::executeMousePressEvent(event);
    if (guiPointerCaptured_) figure_.handle({InputType::cancel});
}
void PlotGraphicsView::executeMouseMoveEvent(MouseEvent* event) {
    Graphics2DView::executeMouseMoveEvent(event);
    if (guiPointerCaptured_ || (event->button() == MouseEvent::MouseButton::NoButton && guiAt(event->x(), event->y())))
        figure_.handle({InputType::leave});
}
void PlotGraphicsView::mousePressEvent(MouseEvent* event) {
    event->setAccepted(figure_.handle(pointerEvent(InputType::press, event)));
}
void PlotGraphicsView::mouseMoveEvent(MouseEvent* event) {
    event->setAccepted(figure_.handle(pointerEvent(InputType::move, event)));
}
void PlotGraphicsView::mouseReleaseEvent(MouseEvent* event) {
    event->setAccepted(figure_.handle(pointerEvent(InputType::release, event)));
}
void PlotGraphicsView::wheelEvent(WheelEvent* source) {
    if (source->orientation() != WheelEvent::Vertical) return;
    InputEvent event{};
    event.type = InputType::scroll;
    event.x = float(source->x()); event.y = float(source->y());
    event.scroll_y = float(source->scrollSteps());
    event.shift = (source->modifierKey() & ModifierKey_Shift) != 0;
    source->setAccepted(figure_.handle(event));
}
void PlotGraphicsView::keyPressEvent(KeyEvent* source) {
    // GUI focus owns the keyboard, even when a widget ignores a particular key.
    if (keyboardFocus() || source->keyAction() == KeyEvent::KeyAction::KeyRelease) return;
    if (source->modifierKey() & (ModifierKey_Control | ModifierKey_Alt | ModifierKey_Super)) return;
    InputEvent event{};
    event.type = InputType::key;
    event.repeat = source->keyAction() == KeyEvent::KeyAction::KeyRepeat;
    switch (source->key()) {
    case ptgl::Key::Key_L: event.key = Key::live; break;
    case ptgl::Key::Key_R: event.key = Key::reset; break;
    case ptgl::Key::Key_F: event.key = Key::live_zoom; break;
    case ptgl::Key::Key_A: event.key = Key::fit_y; break;
    case ptgl::Key::Key_Space: event.key = Key::pause; break;
    case ptgl::Key::Key_Equal: event.key = Key::thicker; break;
    case ptgl::Key::Key_Minus: event.key = Key::thinner; break;
    default: return;
    }
    source->setAccepted(figure_.handle(event));
}
void PlotGraphicsView::cancelInput() {
    Graphics2DView::cancelInput();
    figure_.handle({InputType::cancel});
}
void PlotGraphicsView::mouseLeave() {
    Graphics2DView::mouseLeave();
    figure_.handle({InputType::leave});
}
} // namespace ptgl::plot
