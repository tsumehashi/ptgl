#ifndef PTGL_CORE_QUICKGRAPHICSITEM_H_
#define PTGL_CORE_QUICKGRAPHICSITEM_H_

#include <functional>
#include "GraphicsItem.h"
#include "Graphics2DItem.h"
#include "GraphicsItemEvent.h"
#include "Renderer3D.h"
#include "Renderer2D.h"
#include "TextRenderer.h"

namespace ptgl {

template <typename QuickGraphicsItemType>
class TemplateQuickGraphicsItem;

typedef TemplateQuickGraphicsItem<GraphicsItem> QuickGraphicsItem;
typedef std::shared_ptr<QuickGraphicsItem> QuickGraphicsItemPtr;

typedef TemplateQuickGraphicsItem<Graphics2DItem> QuickGraphics2DItem;
typedef std::shared_ptr<QuickGraphics2DItem> QuickGraphics2DItemPtr;

template <typename GraphicsItemType>
class TemplateQuickGraphicsItem : public GraphicsItemType {
public:

    TemplateQuickGraphicsItem() {}
    virtual ~TemplateQuickGraphicsItem() {}

    void setAttachedGraphicsWindowFunction(std::function<void(GraphicsView*)> func) {
        attachedGraphicsWindowFunc_ = func;
    }

    void setDetachedGraphicsWindowFunction(std::function<void(GraphicsView*)> func) {
        detachedGraphicsWindowFunc_ = func;
    }

    void setPrevProcessFunction(std::function<void(void)> func) {
        prevProcessFunc_ = func;
    }

    void setPostProcessFunction(std::function<void(void)> func) {
        postProcessFunc_ = func;
    }

    void setRenderBackground2DSceneFunction(std::function<void(ptgl::Renderer2D*)> func) {
        renderBackground2DSceneFunc_ = func;
    }

    void setRenderSceneFunction(std::function<void(ptgl::Renderer3D*)> func) {
        renderSceneFunc_ = func;
    }

    void setRenderOverlaySceneFunction(std::function<void(ptgl::Renderer3D*)> func) {
        renderOverlaySceneFunc_ = func;
    }

    void setRender2DSceneFunction(std::function<void(ptgl::Renderer2D*)> func) {
        render2DSceneFunc_ = func;
    }

    void setRenderTextSceneFunction(std::function<void(ptgl::TextRenderer*)> func) {
        renderTextSceneFunc_ = func;
    }

    // Mouse event
    void setMousePressEventFunction(std::function<void(ptgl::GraphicsItemMouseEvent*)> func) {
        mousePressEventFunc_ = func;
    }

    void setMouseMoveEventFunction(std::function<void(ptgl::GraphicsItemMouseEvent*)> func) {
        mouseMoveEventFunc_ = func;
    }

    void setMouseReleaseEventFunction(std::function<void(ptgl::GraphicsItemMouseEvent*)> func) {
        mouseReleaseEventFunc_ = func;
    }

protected:

    // attach/detach graphics window
    virtual void attachedGraphicsWindow(GraphicsView* view) {
        GraphicsItemType::attachedGraphicsWindow(view);
        if (attachedGraphicsWindowFunc_) {
            attachedGraphicsWindowFunc_(view);
        }
    }

    virtual void detachedGraphicsWindow(GraphicsView* view) {
        GraphicsItemType::detachedGraphicsWindow(view);
        if (detachedGraphicsWindowFunc_) {
            detachedGraphicsWindowFunc_(view);
        }
    }

    // execute pre/post process
    virtual void prevProcess() override {
        GraphicsItemType::prevProcess();
        if (prevProcessFunc_) {
            prevProcessFunc_();
        }
    };
    virtual void postProcess() override {
        GraphicsItemType::postProcess();
        if (postProcessFunc_) {
            postProcessFunc_();
        }
    };

    // render scene
    virtual void renderBackground2DScene(ptgl::Renderer2D* r) override {
        GraphicsItemType::renderBackground2DScene(r);
        if (renderBackground2DSceneFunc_) {
            renderBackground2DSceneFunc_(r);
        }
    };

    virtual void renderScene(ptgl::Renderer3D* r) override {
        GraphicsItemType::renderScene(r);
        if (renderSceneFunc_) {
            renderSceneFunc_(r);
        }
    };

    virtual void renderOverlayScene(ptgl::Renderer3D* r) override {
        GraphicsItemType::renderOverlayScene(r);
        if (renderOverlaySceneFunc_) {
            renderOverlaySceneFunc_(r);
        }
    };

    virtual void render2DScene(ptgl::Renderer2D* r) override {
        GraphicsItemType::render2DScene(r);
        if (render2DSceneFunc_) {
            render2DSceneFunc_(r);
        }
    };

    virtual void renderTextScene(ptgl::TextRenderer* r) override {
        GraphicsItemType::renderTextScene(r);
        if (renderTextSceneFunc_) {
            renderTextSceneFunc_(r);
        }
    };

    // Hover event
    virtual void hoverEnterEvent(ptgl::GraphicsItemHoverEvent* e) override {
        GraphicsItemType::hoverEnterEvent(e);
        if (hoverEnterEventFunc_) {
            hoverEnterEventFunc_(e);
        }
    }

    virtual void hoverLeaveEvent(ptgl::GraphicsItemHoverEvent* e) override {
        GraphicsItemType::hoverLeaveEvent(e);
        if (hoverLeaveEventFunc_) {
            hoverLeaveEventFunc_(e);
        }
    }

    virtual void hoverMoveEvent(ptgl::GraphicsItemHoverEvent* e) override {
        GraphicsItemType::hoverMoveEvent(e);
        if (hoverMoveEventFunc_) {
            hoverMoveEventFunc_(e);
        }
    }

    // Mouse event
    virtual void mousePressEvent(ptgl::GraphicsItemMouseEvent* e) override {
        GraphicsItemType::mousePressEvent(e);
        if (mousePressEventFunc_) {
            mousePressEventFunc_(e);
        }
    }

    virtual void mouseMoveEvent(ptgl::GraphicsItemMouseEvent* e) override {
        GraphicsItemType::mouseMoveEvent(e);
        if (mouseMoveEventFunc_) {
            mouseMoveEventFunc_(e);
        }
    }

    virtual void mouseReleaseEvent(ptgl::GraphicsItemMouseEvent* e) override {
        GraphicsItemType::mouseReleaseEvent(e);
        if (mouseReleaseEventFunc_) {
            mouseReleaseEventFunc_(e);
        }
    }

    // Select event
    virtual void selectEnterEvent(ptgl::GraphicsItemSelectEvent* e) override {
        GraphicsItemType::selectEnterEvent(e);
        if (selectEnterEventFunc_) {
            selectEnterEventFunc_(e);
        }
    }

    virtual void selectLeaveEvent(ptgl::GraphicsItemSelectEvent* e) override {
        GraphicsItemType::selectLeaveEvent(e);
        if (selectLeaveEventFunc_) {
            selectLeaveEventFunc_(e);
        }
    }

    std::function<void(GraphicsView*)> attachedGraphicsWindowFunc_;
    std::function<void(GraphicsView*)> detachedGraphicsWindowFunc_;

    std::function<void(void)> prevProcessFunc_;
    std::function<void(void)> postProcessFunc_;

    std::function<void(ptgl::Renderer2D*)> renderBackground2DSceneFunc_;
    std::function<void(ptgl::Renderer3D*)> renderSceneFunc_;
    std::function<void(ptgl::Renderer3D*)> renderOverlaySceneFunc_;
    std::function<void(ptgl::Renderer2D*)> render2DSceneFunc_;
    std::function<void(ptgl::TextRenderer*)> renderTextSceneFunc_;

    // Hover event
    std::function<void(ptgl::GraphicsItemHoverEvent*)> hoverEnterEventFunc_;
    std::function<void(ptgl::GraphicsItemHoverEvent*)> hoverLeaveEventFunc_;
    std::function<void(ptgl::GraphicsItemHoverEvent*)> hoverMoveEventFunc_;

    // Mouse event
    std::function<void(ptgl::GraphicsItemMouseEvent*)> mousePressEventFunc_;
    std::function<void(ptgl::GraphicsItemMouseEvent*)> mouseMoveEventFunc_;
    std::function<void(ptgl::GraphicsItemMouseEvent*)> mouseReleaseEventFunc_;

    // Select event
    std::function<void(ptgl::GraphicsItemSelectEvent*)> selectEnterEventFunc_;
    std::function<void(ptgl::GraphicsItemSelectEvent*)> selectLeaveEventFunc_;
};

} /* namespace ptgl */

#endif /* PTGL_CORE_QUICKGRAPHICSITEM_H_ */
