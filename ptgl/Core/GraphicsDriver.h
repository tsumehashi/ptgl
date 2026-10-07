#ifndef PTGL_CORE_GRAPHICSDRIVER_H_
#define PTGL_CORE_GRAPHICSDRIVER_H_

#include <string>
#include <memory>

namespace ptgl {

class GraphicsView;
class MouseEvent;
class WheelEvent;
class KeyEvent;
class DropEvent;

class GraphicsDriver;
typedef std::unique_ptr<GraphicsDriver> GraphicsDriverPtr;

class GraphicsDriver {
    friend class GraphicsView;
public:
    GraphicsDriver();
    virtual ~GraphicsDriver();

protected:
    virtual void initialize(GraphicsView* view);
    virtual void execute() = 0;
    virtual void terminate() = 0;
    // Background drivers must override and synchronize with their render loop.
    virtual void waitUntilStopped() {}

    virtual bool terminated() = 0;

    virtual void setWindowSize(int width, int height) = 0;
    virtual void setWindowTitle(const std::string& title) = 0;
    virtual void setFrameRate(int fps) = 0;
    virtual void setSwapInterval(int) {}

    virtual const std::string& windowTitle() const = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual int frameRate() const = 0;
    virtual float pixelRatio() const { return 1.0f; }
    virtual std::string clipboardText() const { return {}; }
    virtual void setClipboardText(const std::string &) {}
    virtual bool hasTextInputEvents() const { return false; }
    virtual void setTextInputRect(int, int, int, int) {}
    virtual void cancelTextComposition() {}

    void executeGraphicsViewInitializeEvent();
    void executeGraphicsViewFinalizeEvent();
    void executeGraphicsViewRenderEvent();
    void executeGraphicsViewResizeEvent(int width, int height);

    void executeGraphicsViewPrevProcessEvent();
    void executeGraphicsViewPostProcessEvent();

    void executeGraphicsViewPrevEventProcessEvent();
    void executeGraphicsViewPostEventProcessEvent();

    void executeGraphicsViewMousePressEvent(MouseEvent* e);
    void executeGraphicsViewMouseMoveEvent(MouseEvent* e);
    void executeGraphicsViewMouseReleaseEvent(MouseEvent* e);
    void executeGraphicsViewWheelEvent(WheelEvent* e);
    void executeGraphicsViewKeyPressEvent(KeyEvent* e);
    void executeGraphicsViewDropEvent(DropEvent* e);

    MouseEvent* getGraphicsViewMouseEvent();
    WheelEvent* getGraphicsViewWheelEvent();
    KeyEvent* getGraphicsViewKeyEvent();
    DropEvent* getGraphicsViewDropEvent();

    GraphicsView* graphicsView() const { return view_; }
private:

    GraphicsView* view_ = nullptr;
};

} /* namespace ptgl */

#endif /* PTGL_CORE_GRAPHICSDRIVER_H_ */
