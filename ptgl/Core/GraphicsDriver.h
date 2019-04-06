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

    virtual bool terminated() = 0;

    virtual void setWindowSize(int width, int height) = 0;
    virtual void setWindowTitle(const std::string& title) = 0;
    virtual void setFrameRate(int fps) = 0;

    virtual const std::string& windowTitle() const = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual int frameRate() const = 0;

    void executeGraphicsViewInitializeEvent();
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
