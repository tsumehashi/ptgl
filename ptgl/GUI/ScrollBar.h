#ifndef PTGL_GUI_SCROLLBAR_H_
#define PTGL_GUI_SCROLLBAR_H_

#include "AbstractScrollBar.h"
#include "PushButton.h"

namespace ptgl {
namespace gui {

class VerticalScrollBar;
using VerticalScrollBarPtr = std::shared_ptr<VerticalScrollBar>;

class HorizontalScrollBar;
using HorizontalScrollBarPtr = std::shared_ptr<HorizontalScrollBar>;

class VerticalScrollBar : public AbstractScrollBar {
public:
    VerticalScrollBar();
    virtual ~VerticalScrollBar();

    virtual int scrollAreaSize() const override {
        return height();
    }

    virtual void setSize(int width, int height) override;

protected:

    class BarWidget : public Widget {
    public:
        BarWidget(VerticalScrollBar* scrollBar);
        virtual ~BarWidget() {}

    protected:
        virtual void render2DScene(ptgl::Renderer2D* r) override;
        virtual void mousePressEvent(ptgl::GraphicsItemMouseEvent* e) override;
        virtual void mouseMoveEvent(ptgl::GraphicsItemMouseEvent* e) override;

        VerticalScrollBar* scrollBar_ = nullptr;
        int moveStartBarLocalY_ = 0;
    };

    virtual void render2DScene(ptgl::Renderer2D* r) override;

    void init();

    void setScrollValueFromBarLocalY(int localY);

    PushButtonPtr valueUpButton_;
    PushButtonPtr valueDownButton_;
    std::shared_ptr<BarWidget> barWidget_;
};

class HorizontalScrollBar : public AbstractScrollBar {
public:
    HorizontalScrollBar();
    virtual ~HorizontalScrollBar();

    virtual int scrollAreaSize() const override {
        return width();
    }

    virtual void setSize(int width, int height) override;

protected:

    class BarWidget : public Widget {
    public:
        BarWidget(HorizontalScrollBar* scrollBar);
        virtual ~BarWidget() {}

    protected:
        virtual void render2DScene(ptgl::Renderer2D* r) override;
        virtual void mousePressEvent(ptgl::GraphicsItemMouseEvent* e) override;
        virtual void mouseMoveEvent(ptgl::GraphicsItemMouseEvent* e) override;

        HorizontalScrollBar* scrollBar_ = nullptr;
        int moveStartBarLocalX_ = 0;
    };

    virtual void render2DScene(ptgl::Renderer2D* r) override;

    void init();

    void setScrollValueFromBarLocalX(int localX);

    PushButtonPtr valueUpButton_;
    PushButtonPtr valueDownButton_;
    std::shared_ptr<BarWidget> barWidget_;
};

}
} /* namespace ptgl */

#endif /* PTGL_GUI_SCROLLBAR_H_ */
