#ifndef PTGL_CORE_QUICKGRAPHICSVIEW_H_
#define PTGL_CORE_QUICKGRAPHICSVIEW_H_

#include <functional>
#include "GraphicsView.h"
#include "Graphics2DView.h"

namespace ptgl {

template <typename GraphicsViewType>
class TemplateQuickGraphicsView;

typedef TemplateQuickGraphicsView<GraphicsView> QuickGraphicsView;
typedef TemplateQuickGraphicsView<Graphics2DView> QuickGraphics2DView;

template <typename GraphicsViewType>
class TemplateQuickGraphicsView : public GraphicsViewType {
public:
    TemplateQuickGraphicsView(std::unique_ptr<GraphicsDriver> driver) : GraphicsViewType(std::move(driver)) {

    }
    virtual ~TemplateQuickGraphicsView() {

    }

    void setInitProcessFunction(std::function<void(void)> func) {
        initProcessFunc_ = func;
    }

    void setPrevProcessFunction(std::function<void(void)> func) {
        prevProcessFunc_ = func;
    }

    void setPostProcessFunction(std::function<void(void)> func) {
        postProcessFunc_ = func;
    }

    void setPrevEventProcessFunction(std::function<void(void)> func) {
        prevEventProcessFunc_ = func;
    }

    void setPostEventProcessFunction(std::function<void(void)> func) {
        postEventProcessFunc_ = func;
    }

    void setRenderScenePostProcessFunction(std::function<void(ptgl::Renderer3D*)> func) {
        renderScenePostProcessFunc_ = func;
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

    void setMousePressEventFunction(std::function<void(ptgl::MouseEvent*)> func) {
        mousePressEventFunc_ = func;
    }

    void setMouseMoveEventFunction(std::function<void(ptgl::MouseEvent*)> func) {
        mouseMoveEventFunc_ = func;
    }

    void setMouseReleaseEventFunction(std::function<void(ptgl::MouseEvent*)> func) {
        mouseReleaseEventFunc_ = func;
    }

    void setWheelEventFunction(std::function<void(ptgl::WheelEvent*)> func) {
        wheelEventFunc_ = func;
    }

    void setKeyPressEventFunction(std::function<void(ptgl::KeyEvent*)> func) {
        keyPressEventFunc_ = func;
    }

    void setDropEventFunction(std::function<void(ptgl::DropEvent*)> func) {
        dropEventFunc_ = func;
    }

    void setPickingUpEventFunction(std::function<void(ptgl::PickingEvent*)> func) {
        pickingUpEventFunc_ = func;
    }

    void setResizeEventFunction(std::function<void(int, int)> func) {
        resizeEventFunc_ = func;
    }

    // drawWorldGrid
    void setDrawWorldGrid(bool on) {
        drawWorldGrid_ = on;
    }

    void setWorldGridColor(double r, double g, double b) {
        worldGridColor_ = {{r, g, b}};
    }

protected:

    // execute initialize process
    virtual void initProcess() override {
        GraphicsViewType::initProcess();
        if (initProcessFunc_) {
            initProcessFunc_();
        }
    }

    // execute pre/post process
    virtual void prevProcess() override {
        GraphicsViewType::prevProcess();
        if (prevProcessFunc_) {
            prevProcessFunc_();
        }
    };
    virtual void postProcess() override {
        GraphicsViewType::postProcess();
        if (postProcessFunc_) {
            postProcessFunc_();
        }
    };

    // execute pre/post process (event)
    virtual void prevEventProcess() override {
        GraphicsViewType::prevEventProcess();
        if (prevEventProcessFunc_) {
            prevEventProcessFunc_();
        }
    };
    virtual void postEventProcess() override {
        GraphicsViewType::postEventProcess();
        if (postEventProcessFunc_) {
            postEventProcessFunc_();
        }
    };


    // execute render scene post process
    virtual void renderScenePostProcess(ptgl::Renderer3D* r) override {
        GraphicsViewType::renderScenePostProcess(r);
        if (renderScenePostProcessFunc_) {
            renderScenePostProcessFunc_(r);
        }
    };

    // render scene
    virtual void renderBackground2DScene(ptgl::Renderer2D* r) override {
        GraphicsViewType::renderBackground2DScene(r);
        if (renderBackground2DSceneFunc_) {
            renderBackground2DSceneFunc_(r);
        }
    };

    virtual void renderScene(ptgl::Renderer3D* r) override {
        GraphicsViewType::renderScene(r);

        if (drawWorldGrid_) {
            drawWorldGrid(r);
        }

        if (renderSceneFunc_) {
            renderSceneFunc_(r);
        }
    };

    virtual void renderOverlayScene(ptgl::Renderer3D* r) override {
        GraphicsViewType::renderOverlayScene(r);
        if (renderOverlaySceneFunc_) {
            renderOverlaySceneFunc_(r);
        }
    };

    virtual void render2DScene(ptgl::Renderer2D* r) override {
        GraphicsViewType::render2DScene(r);
        if (render2DSceneFunc_) {
            render2DSceneFunc_(r);
        }
    };

    virtual void renderTextScene(ptgl::TextRenderer* r) override {
        GraphicsViewType::renderTextScene(r);
        if (renderTextSceneFunc_) {
            renderTextSceneFunc_(r);
        }
    };

    virtual void mousePressEvent(MouseEvent* e) override {
        GraphicsViewType::mousePressEvent(e);
        if (mousePressEventFunc_) {
            mousePressEventFunc_(e);
        }
    }

    virtual void mouseMoveEvent(MouseEvent* e) override {
        GraphicsViewType::mouseMoveEvent(e);
        if (mouseMoveEventFunc_) {
            mouseMoveEventFunc_(e);
        }
    }

    virtual void mouseReleaseEvent(MouseEvent* e) override {
        GraphicsViewType::mouseReleaseEvent(e);
        if (mouseReleaseEventFunc_) {
            mouseReleaseEventFunc_(e);
        }
    }

    virtual void wheelEvent(WheelEvent* e) override {
        GraphicsViewType::wheelEvent(e);
        if (wheelEventFunc_) {
            wheelEventFunc_(e);
        }
    }

    virtual void keyPressEvent(KeyEvent* e) override {
        GraphicsViewType::keyPressEvent(e);
        if (keyPressEventFunc_) {
            keyPressEventFunc_(e);
        }
    }

    virtual void dropEvent(DropEvent* e) override {
        GraphicsViewType::dropEvent(e);
        if (dropEventFunc_) {
            dropEventFunc_(e);
        }
    }

    virtual void pickingUpEvent(PickingEvent* e) override {
        GraphicsViewType::pickingUpEvent(e);
        if (pickingUpEventFunc_) {
            pickingUpEventFunc_(e);
        }
    }

    virtual void resizeEvent(int width, int height) override {
        GraphicsViewType::resizeEvent(width, height);
        if (resizeEventFunc_) {
            resizeEventFunc_(width, height);
        }
    }

    void drawWorldGrid(ptgl::Renderer3D* r) {
        r->setColor(worldGridColor_[0], worldGridColor_[1], worldGridColor_[2]);
        r->setLineWidth(1);
        r->drawWorldGrid();
        r->setDepthTest(false);
        r->setLineWidth(2);
        r->drawWorldAxis(10);
        r->setDepthTest(true);
    }

    std::function<void(void)> initProcessFunc_;

    std::function<void(void)> prevProcessFunc_;
    std::function<void(void)> postProcessFunc_;

    std::function<void(void)> prevEventProcessFunc_;
    std::function<void(void)> postEventProcessFunc_;

    std::function<void(ptgl::Renderer3D*)> renderScenePostProcessFunc_;

    std::function<void(ptgl::Renderer2D*)> renderBackground2DSceneFunc_;
    std::function<void(ptgl::Renderer3D*)> renderSceneFunc_;
    std::function<void(ptgl::Renderer3D*)> renderOverlaySceneFunc_;
    std::function<void(ptgl::Renderer2D*)> render2DSceneFunc_;
    std::function<void(ptgl::TextRenderer*)> renderTextSceneFunc_;

    std::function<void(ptgl::MouseEvent*)> mousePressEventFunc_;
    std::function<void(ptgl::MouseEvent*)> mouseMoveEventFunc_;
    std::function<void(ptgl::MouseEvent*)> mouseReleaseEventFunc_;

    std::function<void(ptgl::WheelEvent*)> wheelEventFunc_;

    std::function<void(ptgl::KeyEvent*)> keyPressEventFunc_;

    std::function<void(ptgl::DropEvent*)> dropEventFunc_;

    std::function<void(ptgl::PickingEvent*)> pickingUpEventFunc_;

    std::function<void(int, int)> resizeEventFunc_;

    // drawWorldGrid
    bool drawWorldGrid_ = true;
    std::array<double, 3> worldGridColor_ = std::array<double, 3>{1.0, 1.0, 1.0};

};

} /* namespace ptgl */

#endif /* PTGL_CORE_QUICKGRAPHICSVIEW_H_ */
