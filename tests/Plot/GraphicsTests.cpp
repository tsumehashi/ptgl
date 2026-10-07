#include "ptgl/Plot/PlotGraphicsView.h"
#include "ptgl/Plot/Viewer.h"
#include "ptgl/Driver/GLFWGraphicsDriver.h"
#include "ptgl/GUI/PushButton.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
class View final : public ptgl::plot::PlotGraphicsView {
public:
    explicit View(ptgl::GraphicsDriverPtr driver) : PlotGraphicsView(std::move(driver), 2,10000,10) {
        figure().addAxes("Joints", "deg", {-100,100}).addSeries(0,"Shoulder");
        figure().axes(0).addSeries(1,"Elbow");
        for (int i=0;i<10000;++i)
            figure().append(i*.001,std::array<float,2>{float(80*std::sin(i*.001)),float(60*std::cos(i*.002))});
        auto button = std::make_shared<ptgl::gui::PushButton>("Pause");
        button->setPos(40,10); button->setSize(130,30);
        addWidget(button);
        setPlotMargins(0,50,0,0);
        figure().setLineWidth(4);
        figure().setLiveZoomEnabled(true);
    }
    ~View() override { terminate(); waitUntilStopped(); }
    int sceneCalls = 0;
protected:
    void executeRenderScene(ptgl::Renderer3D*) override { ++sceneCalls; }
    void executeRenderPickingScene(ptgl::Renderer3D*) override { ++sceneCalls; }
    void postProcess() override {
        check(!pickingUpShaderProgram_ && !depthRenderShaderProgram_, "plot must not initialize picking shaders");
        check(sceneCalls == 0, "plot must not execute 3D/picking passes");
        check(!renderer().overflowed() && renderer().selectedPoints()>0, "plot geometry must be rendered");
        check(glGetError() == GL_NO_ERROR, "plot + GUI generated GL errors");
        if (currentFrame() == 0) {
            // Exercise reconfiguration between frames, alongside GUI rendering.
            figure().axes(0).series(1).setVisible(false);
            setWindowSize(900,600);
        }
        if (currentFrame() >= 3) terminate();
    }
};
class SceneView final : public ptgl::GraphicsView {
public:
    using GraphicsView::GraphicsView;
    int frames = 0;
protected:
    void postProcess() override {
        check(pickingUpShaderProgram_ != nullptr, "3D view still initializes picking");
        check(glGetError() == GL_NO_ERROR, "existing 3D view generated GL errors");
        ++frames;
        if (frames == 2) terminate();
    }
};
template<class T> void runView() {
    auto driver = std::make_unique<ptgl::GLFWGraphicsDriver>(ptgl::GLFWGraphicsDriver::ExecutionMode::CallingThread);
    auto* execution = driver.get();
    T view(std::move(driver));
    view.initialize();
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    view.execute();
    if (execution->executionError()) std::rethrow_exception(execution->executionError());
}
}
int main() {
    try {
        runView<View>();
        runView<SceneView>(); // Sequential windows exercise GLFW shutdown/reinitialization.
        // Keep a caller-created driver alive while show() creates one in the DLL.
        // GLFW ownership must be shared across library/application boundaries.
        auto driver = std::make_unique<ptgl::GLFWGraphicsDriver>(ptgl::GLFWGraphicsDriver::ExecutionMode::CallingThread);
        auto* execution = driver.get();
        SceneView pending(std::move(driver));
        pending.initialize();
        ptgl::plot::ViewerOptions options;
        options.smoke_frames = 2;
        ptgl::plot::Figure figure;
        figure.addXyAxes("XY").addXySeries({{0,0},{1,1},{2,0}});
        check(ptgl::plot::show(std::move(figure),{},options) == 0, "show convenience path");
        pending.execute();
        if (execution->executionError()) std::rethrow_exception(execution->executionError());
        std::cout << "Plot GPU, GUI, resize, lifecycle and 3D regression passed\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
