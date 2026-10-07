#pragma once
#include "ptgl/Core/Graphics2DView.h"
#include "ptgl/GUI/Widget.h"
#include "Figure.h"
#include "PtglRenderer.h"
#include <functional>

namespace ptgl::plot {
using Update = std::function<void(Figure&, double elapsed)>;

// A plot window with ptgl GUI, without 3D scene or GPU picking passes.
// Mutate the figure and widgets on the view/event thread, or before execute().
// Acquisition threads should send batches through an application-owned queue.
class PlotGraphicsView : public Graphics2DView {
public:
    // The convenience constructor runs GLFW on the calling (main) thread.
    explicit PlotGraphicsView(std::size_t channels = 1, std::size_t capacity = 1,
                              double windowSeconds = 60);
    PlotGraphicsView(GraphicsDriverPtr driver, std::size_t channels = 1,
                     std::size_t capacity = 1, double windowSeconds = 60);
    PlotGraphicsView(GraphicsDriverPtr driver, Figure figure);
    ~PlotGraphicsView() override;

    Figure& figure() noexcept { return figure_; }
    const Figure& figure() const noexcept { return figure_; }
    // Called before drawing, except while Figure::isSourcePaused().
    void setUpdateFunction(Update function) { update_ = std::move(function); }
    // Reserve logical pixels for GUI panels; the default plot fills the window.
    void setPlotMargins(float left, float top, float right, float bottom);
    Rect plotBounds() const;
    void addWidget(gui::WidgetPtr widget);
    void setGuiTheme(const gui::Theme& theme);
    const gui::Theme& guiTheme() const noexcept { return guiTheme_; }
    // Set before execute(). A too-small budget is reported by renderer().overflowed().
    void setMaxVertices(int count);
    const PtglRenderer& renderer() const noexcept { return renderer_; }
    void cancelInput() override;
    void mouseLeave() override;

protected:
    bool usesSceneRendering() const override { return false; }
    void executeInitializeEvent() override;
    void executeFinalizeEvent() override;
    void executePrevProcess() override;
    void renderContents() override;
    void executeMousePressEvent(MouseEvent* event) override;
    void executeMouseMoveEvent(MouseEvent* event) override;
    void mousePressEvent(MouseEvent* event) override;
    void mouseMoveEvent(MouseEvent* event) override;
    void mouseReleaseEvent(MouseEvent* event) override;
    void wheelEvent(WheelEvent* event) override;
    void keyPressEvent(KeyEvent* event) override;

private:
    void updateGuiScale();
    Figure figure_;
    PtglRenderer renderer_;
    Update update_;
    std::array<float, 4> margins_{};
    gui::Theme guiTheme_ = gui::Theme::light();
    float guiPixelRatio_ = -1;
    int maxVertices_ = 1024 * 1024;
};
} // namespace ptgl::plot
