#ifndef PTGL_CORE_GRAPHICSITEM_H_
#define PTGL_CORE_GRAPHICSITEM_H_

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace ptgl {

class GraphicsView;
class Renderer3D;
class Renderer2D;
class TextRenderer;
class GraphicsItemHoverEvent;
class GraphicsItemMouseEvent;
class GraphicsItemWheelEvent;
class GraphicsItemSelectEvent;
class GraphicsItemKeyEvent;

class GraphicsItem;
typedef std::shared_ptr<GraphicsItem> GraphicsItemPtr;

typedef std::vector<GraphicsItemPtr> GraphicsItemList;

class GraphicsItem {
    friend class GraphicsView;
public:
    GraphicsItem();
    virtual ~GraphicsItem();

    void setName(const std::string& name) { name_ = name; }
    const std::string& name() const { return name_; }

    virtual void addChild(GraphicsItemPtr item);
    virtual void removeChild(GraphicsItemPtr item);

    // visible == false not picking
    bool isVisible() const { return visible_; }
    virtual void setVisible(bool visible);

    bool isEnabled() const { return enabled_; }
    virtual void setEnabled(bool enabled);

    // pickable == false not picking
    bool isPickable() const { return pickable_; }
    virtual void setPickable(bool enable);

    // enabledWheelEvent (default disabled)
    bool isEnabledWheelEvent() const { return enabledWheelEvent_; }
    virtual void setEnabledWheelEvent(bool enable);

    // enabledKeyEvent (default disabled)
    bool isEnabledKeyEvent() const { return enabledKeyEvent_; }
    virtual void setEnabledKeyEvent(bool enable);

    GraphicsItem* parentItem() const { return parent_; }

    GraphicsItemList& childItems()  { return children_; }
    const GraphicsItemList& childItems() const { return children_; }
    size_t numChildItems() const { return children_.size(); }
    GraphicsItemPtr childItem(size_t i) const { return children_[i]; }

    GraphicsView* graphicsWindow() const { return graphicsView_; }
    int viewWidth() const;
    int viewHeight() const;

    bool isHoverd() const { return hoverd_; }
    bool isPicked() const { return picked_; }
    bool isChecked() const { return checked_; }
    bool isPressed() const { return pressed_; }

    bool isPickChanged() const { return pickChanged_; }
    bool isCheckChanged() const { return checkChanged_; }

    void setHoverd(bool hoverd);    // for GraphicsView
    void setPicked(bool picked);    // for GraphicsView
    void setChecked(bool checked);    // for GraphicsView

    static void traverse(GraphicsItemPtr item, std::function<void (GraphicsItemPtr)> func);
    static void traverseReverse(GraphicsItemPtr item, std::function<void (GraphicsItemPtr)> func);

protected:

    // attach/detach graphics window
    virtual void attachedGraphicsWindow(GraphicsView* /* view */) {}
    virtual void detachedGraphicsWindow(GraphicsView* /* view */) {}

    // execute pre/post process
    virtual void prevProcess() {}
    virtual void postProcess() {}

    // render scene
    virtual void renderBackground2DScene(ptgl::Renderer2D* /* r */) {}    // not picked

    virtual void renderScene(ptgl::Renderer3D* /* r */) {}
    virtual void renderOverlayScene(ptgl::Renderer3D* /* r */) {}

    virtual void render2DScene(ptgl::Renderer2D* /* r */) {}
    virtual void renderTextScene(ptgl::TextRenderer* /* r */) {}

    // picking scene
    virtual void renderPickingScene(ptgl::Renderer3D* r) { renderScene(r); }
    virtual void renderPickingOverlayScene(ptgl::Renderer3D* r) { renderOverlayScene(r); }
    virtual void renderPicking2DScene(ptgl::Renderer2D* r) { render2DScene(r); }

    // Hover event
    virtual void hoverEnterEvent(ptgl::GraphicsItemHoverEvent* /*e*/) {}
    virtual void hoverLeaveEvent(ptgl::GraphicsItemHoverEvent* /*e*/) {}
    virtual void hoverMoveEvent(ptgl::GraphicsItemHoverEvent* /*e*/) {}

    // Mouse event
    virtual void mousePressEvent(ptgl::GraphicsItemMouseEvent* /*e*/) {}
    virtual void mouseMoveEvent(ptgl::GraphicsItemMouseEvent* /*e*/) {}
    virtual void mouseReleaseEvent(ptgl::GraphicsItemMouseEvent* /*e*/) {}

    // Wheel event
    virtual void wheelEvent(ptgl::GraphicsItemWheelEvent* /*e*/) {}

    // Select event
    virtual void selectEnterEvent(ptgl::GraphicsItemSelectEvent* /*e*/) {}
    virtual void selectLeaveEvent(ptgl::GraphicsItemSelectEvent* /*e*/) {}

    // Key event
    virtual void keyPressEvent(ptgl::GraphicsItemKeyEvent* /*e*/) {}

private:

    void setGraphicsView(GraphicsView* view);

    // Mouse event
    void executeMousePressEvent(ptgl::GraphicsItemMouseEvent* e);
    void executeMouseMoveEvent(ptgl::GraphicsItemMouseEvent* e);
    void executeMouseReleaseEvent(ptgl::GraphicsItemMouseEvent* e);

    // Prev/Post event
    void executePrevProcess();
    void executePostProcess();

    std::string name_;

    GraphicsItem* parent_ = nullptr;
    GraphicsItemList children_;

    GraphicsView* graphicsView_ = nullptr;
    bool visible_ = true;
    bool enabled_ = true;
    bool pickable_ = true;    // hover/picked/checked
    bool enabledWheelEvent_ = false;
    bool enabledKeyEvent_ = false;

    bool hoverd_ = false;
    bool picked_ = false;
    bool checked_ = false;
    bool pressed_ = false;

    bool pickChanged_ = false;
    bool checkChanged_ = false;
};

} /* namespace ptgl */

#endif /* PTGL_CORE_GRAPHICSITEM_H_ */
