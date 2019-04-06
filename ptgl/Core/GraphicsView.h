#ifndef PTGL_CORE_GRAPHICSVIEW_H_
#define PTGL_CORE_GRAPHICSVIEW_H_

#include <atomic>
#include <memory>
#include <string>
#include <chrono>
#include "Camera.h"
#include "Event.h"
#include "Font.h"
#include "GraphicsItem.h"
#include "GraphicsItemEvent.h"
#include "Light.h"
#include "Renderer3D.h"
#include "Renderer2D.h"
#include "TextRenderer.h"

namespace ptgl {

class GraphicsDriver;
typedef std::unique_ptr<GraphicsDriver> GraphicsDriverPtr;

class GraphicsView {
    friend class GraphicsDriver;
public:
    enum ViewMode {
        NormalViewMode,
        Ortho2DViewMode,
    };

    GraphicsView(GraphicsDriverPtr driver);
    virtual ~GraphicsView();

    virtual void initialize();
    virtual void execute();
    virtual void terminate();

    virtual bool terminated();

    void setWindowTitle(const std::string& title);
    const std::string& windowTitle() const;

    void setWindowSize(int width, int height);
    int width() const;
    int height() const;

    void setFrameRate(int fps);
    int frameRate() const;

    uint64_t currentFrame() const { return currentFrame_; }

    double previousTime() const { return previousTime_; }
    double currentTime() const { return currentTime_; }
    double deltaTime() const { return deltaTime_; }

    // Camera
    void setEnableCameraManipulate(bool on) { enableCameraManipulate_ = on; }
    void setCamera(CameraPtr camera);
    CameraPtr camera() const { return camera_; }

    // Light
    void addLight(LightPtr light);
    void removeLight(LightPtr light);
    LightPtr light(size_t index = 0) const;
    size_t numLights() const { return lightList_.size(); }

    //  settings
    void setBackgroundColor(double r, double g, double b, double a = 1.0);

    void setViewMode(ViewMode mode) { viewMode_ = mode; }
    ViewMode viewMode() const { return viewMode_; }

    // parameters
    float perspectiveFovy() const { return perspectiveFovy_; }      // perspective fov (deg)
    float perspectiveZnear() const { return perspectiveZnear_; }    // perspective z near
    float perspectiveZfar() const { return perspectiveZfar_; }      // perspective z far

    // GraphicsItem
    void addGraphicsItem(GraphicsItemPtr item);
    void removeGraphicsItem(GraphicsItemPtr item);
    int numGraphicsItems() const { return graphicsItems_.size(); }
    GraphicsItemPtr graphicsItem(int i) { return graphicsItems_[i]; }
    const GraphicsItemList& graphicsItems() const { return graphicsItems_; }

    GraphicsItemList getTraversedGraphicsItems() const;

    // shader program
    void setDefaultShaderProgram(ShaderProgramPtr shaderProgram);
    void setPickingUpShaderProgram(ShaderProgramPtr shaderProgram);
    void setDepthRenderShaderProgram(ShaderProgramPtr shaderProgram);

    // 2D scene
    void setDefault2DShaderProgram(ShaderProgramPtr shaderProgram);

    // Event (reference only)
    const MouseEvent& getMouseEvent() const { return *mouseEvent_; }
    const WheelEvent& getWheelEvent() const { return *wheelEvent_; }
    const KeyEvent& getKeyEvent() const { return *keyEvent_; }
    const PickingEvent& getPickingEvent() const { return *pickingEvent_; }

#if 0
    // image buffer
    bool enableImageBufferData() const { return enableImageBufferData_; }
    void setEnableImageBufferData(bool enable);
    const std::vector<std::array<uint8_t, 4>>& imageBufferData() const { return imageBufferData_; }

    // depth buffer
    bool enableDepthBufferData() const { return enableDepthBufferData_; }
    void setEnableDepthBufferData(bool enable);
    const std::vector<float>& depthBufferData() const { return depthBufferData_; }

    // depth data
    bool enableDepthData() const { return enableDepthData_; }
    void setEnableDepthData(bool enable);
    const std::vector<double>& depthData() const { return depthData_; }

    int getBufferIndex(int x, int y);    // left upper origin

#endif

protected:
    // for derived class

    // execute initialize process
    virtual void initProcess() {}

    // execute prev/post process (render)
    virtual void prevProcess() {}
    virtual void postProcess() {}

    // execute prev/post process (render)
    virtual void prevEventProcess() {}
    virtual void postEventProcess() {}

    // execute render scene post process
    virtual void renderScenePostProcess(ptgl::Renderer3D*) {}

    // render scene
    virtual void renderBackground2DScene(ptgl::Renderer2D*) {}
    virtual void renderScene(ptgl::Renderer3D*) {}
    virtual void renderOverlayScene(ptgl::Renderer3D*) {}
    virtual void render2DScene(ptgl::Renderer2D*) {}
    virtual void renderTextScene(ptgl::TextRenderer*) {}

    // event
    virtual void mousePressEvent(MouseEvent*) {}
    virtual void mouseMoveEvent(MouseEvent*) {}
    virtual void mouseReleaseEvent(MouseEvent*) {}
    virtual void wheelEvent(WheelEvent*) {}
    virtual void keyPressEvent(KeyEvent*) {}
    virtual void dropEvent(DropEvent*) {}
    virtual void pickingUpEvent(PickingEvent*) {}
    virtual void resizeEvent(int /* width */, int /* height */) {}

protected:

    // event
    virtual void executeInitializeEvent();
    virtual void executeResizeEvent(int width, int height);
    virtual void executeRenderEvent();

    // execute prev/post process (render)
    virtual void executePrevProcess();
    virtual void executePostProcess();

    // execute prev/post process (event)
    virtual void executePrevEventProcess();
    virtual void executePostEventProcess();

    // render scene
    virtual void executeRenderBackground2DScene(ptgl::Renderer2D* r);
    virtual void executeRenderScene(ptgl::Renderer3D* r);
    virtual void executeRenderOverlayScene(ptgl::Renderer3D* );
    virtual void executeRender2DScene(ptgl::Renderer2D* r);
    virtual void executeRenderTextScene(ptgl::TextRenderer* r);

    // render picking scene
    virtual void executeRenderPickingScene(ptgl::Renderer3D* r);
    virtual void executeRenderPickingOverlayScene(ptgl::Renderer3D* );
    virtual void executeRenderPicking2DScene(ptgl::Renderer2D* r);

    // execute render scene post process
    virtual void executeRenderScenePostProcess(ptgl::Renderer3D* r);

    // input event
    virtual void executeMousePressEvent(MouseEvent* e);
    virtual void executeMouseMoveEvent(MouseEvent* e);
    virtual void executeMouseReleaseEvent(MouseEvent* e);
    virtual void executeWheelEvent(WheelEvent* e);
    virtual void executeKeyPressEvent(KeyEvent* e);
    virtual void executePickingUpEvent(PickingEvent* e);
    virtual void executeDropEvent(DropEvent* e);

    // picking event
    std::array<double, 4> pickIdToColor(uint32_t id);    // return rgba (0x00 - 0xFFFFFF)
    uint32_t colorToPickId(uint8_t r, uint8_t g, uint8_t b, uint8_t a);    // 0-255
    GraphicsItemPtr pickingUpSceneGrahicsItem(int x, int y);
    GraphicsItemPtr pickingUpOverlaySceneGrahicsItem(int x, int y);
    GraphicsItemPtr pickingUp2DSceneGrahicsItem(int x, int y);
    GraphicsItemPtr pickingUpGraphicsItem(int x, int y);
    void calcPickedDepth();
    void handlePickingUpEvent();

    // handle GraphicsItem event
    void executeGraphicsItemMousePressEvent(MouseEvent* e);
    void executeGraphicsItemMouseMoveEvent(MouseEvent* e);
    void executeGraphicsItemMouseReleaseEvent(MouseEvent* e);

    // shader
    void loadDefaultShader();

    // get time
    double getCurrentTime();

    // frame
    uint64_t currentFrame_ = 0;

    // time
    decltype(std::chrono::system_clock::now()) startTimePoint_;
    double previousTime_ = 0.0;
    double currentTime_ = 0.0;
    double deltaTime_ = 0.0;

    // driver
    std::atomic<bool> initialized_;
    std::unique_ptr<GraphicsDriver> driver_;

    // renderer
    std::unique_ptr<Renderer3D> renderer3D_;
    std::unique_ptr<Renderer2D> renderer2D_;
    std::unique_ptr<TextRenderer> textRenderer_;

    // camera
    bool enableCameraManipulate_;
    CameraPtr camera_;

    // light
    std::vector<LightPtr> lightList_;

    // view
    float perspectiveFovy_;        // perspective fov (deg)
    float perspectiveZnear_;    // perspective z near
    float perspectiveZfar_;        // perspective z far

    std::array<double, 4> backgroundColor_;
    ViewMode viewMode_;

    // event
    std::unique_ptr<MouseEvent> mouseEvent_;
    std::unique_ptr<WheelEvent> wheelEvent_;
    std::unique_ptr<KeyEvent> keyEvent_;
    std::unique_ptr<DropEvent> dropEvent_;
    std::unique_ptr<PickingEvent> pickingEvent_;

    // picking
    int mouseX_ = 0;
    int mouseY_ = 0;
    double pickedZ_ = 0.0;
    double pickedDepth_ = 0.0;
    std::vector<GraphicsItemPtr> pickIdToItemList_;    // index=0 is background(null_ptr)

    // shader
    ShaderProgramPtr pickingUpShaderProgram_;
    ShaderProgramPtr depthRenderShaderProgram_;

    // GraphicsItem
    GraphicsItemList graphicsItems_;

    GraphicsItemPtr mouseGraphicsItem_;
    GraphicsItemPtr pickedGraphicsItem_;
    GraphicsItemPtr focusedGraphicsItem_;
    GraphicsItemPtr hoveredGraphicsItem_;

    GraphicsItemPtr prevMousePressGraphicsItem_;

    std::unique_ptr<GraphicsItemMouseEvent> graphicsItemMouseEvent_;
    std::unique_ptr<GraphicsItemWheelEvent> graphicsItemWheelEvent_;
    std::unique_ptr<GraphicsItemHoverEvent> graphicsItemHoverEvent_;
    std::unique_ptr<GraphicsItemSelectEvent> graphicsItemSelectEvent_;
    std::unique_ptr<GraphicsItemKeyEvent> graphicsItemKeyEvent_;

    std::vector<GraphicsItemPtr> traversedItems_;

#if 0
    // image buffer
    bool enableImageBufferData_;
    std::vector<std::array<uint8_t, 4>> imageBufferData_;

    // depth buffer
    bool enableDepthBufferData_;
    std::vector<float> depthBufferData_;

    bool enableDepthData_;
    std::vector<double> depthData_;
#endif

protected:
    // for GraphicsDriver
    virtual void executeGraphicsViewInitializeEvent();
    virtual void executeGraphicsViewResizeEvent(int width, int height);
    virtual void executeGraphicsViewRenderEvent();

    virtual void executeGraphicsViewPrevProcessEvent();
    virtual void executeGraphicsViewPostProcessEvent();

    virtual void executeGraphicsViewPrevEventProcessEvent();
    virtual void executeGraphicsViewPostEventProcessEvent();

    virtual void executeGraphicsViewMousePressEvent(MouseEvent* e);
    virtual void executeGraphicsViewMouseMoveEvent(MouseEvent* e);
    virtual void executeGraphicsViewMouseReleaseEvent(MouseEvent* e);
    virtual void executeGraphicsViewWheelEvent(WheelEvent* e);
    virtual void executeGraphicsViewKeyPressEvent(KeyEvent* e);
    virtual void executeGraphicsViewDropEvent(DropEvent* e);

    MouseEvent* getGraphicsViewMouseEvent() { return mouseEvent_.get(); }
    WheelEvent* getGraphicsViewWheelEvent() { return wheelEvent_.get(); }
    KeyEvent* getGraphicsViewKeyEvent() { return keyEvent_.get(); }
    DropEvent* getGraphicsViewDropEvent() { return dropEvent_.get(); }
};

} /* namespace ptgl */

#endif /* PTGL_CORE_GRAPHICSVIEW_H_ */
