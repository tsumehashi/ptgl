#include "ScrollBar.h"
#include "ptgl/Core/GraphicsItemEvent.h"
#include "ptgl/Core/Renderer2D.h"

namespace ptgl {
namespace gui {

// VerticalScrollBar
VerticalScrollBar::VerticalScrollBar() {
    setSize(10, 100);
    setScrolledAreaSize(height());
    init();
}

VerticalScrollBar::~VerticalScrollBar() {

}

void VerticalScrollBar::init()
{
    valueUpButton_ = std::make_shared<PushButton>();
    valueUpButton_->setSize(width(), width());
    valueUpButton_->setOnClickedFunction([&](bool){
        setScrollValue(scrollValue() + scrollDeltaValue());
    });
    addWidget(valueUpButton_);

    valueDownButton_ = std::make_shared<PushButton>();
    valueDownButton_->setSize(width(), width());
    valueDownButton_->setOnClickedFunction([&](bool){
        setScrollValue(scrollValue() - scrollDeltaValue());
    });
    addWidget(valueDownButton_);

    barWidget_ = std::make_shared<BarWidget>(this);
    barWidget_->setSize(width(), (height() - 2*width()));
    addWidget(barWidget_);
}

void VerticalScrollBar::setSize(int width, int height)
{
    AbstractScrollBar::setSize(width, height);
    setScrollValue(scrollValue());
}

void VerticalScrollBar::render2DScene(ptgl::Renderer2D* r)
{
    if (!isEnabled() || !isVisible()) return;

    this->updatePos();

    int x = this->x();
    int y = this->y();

    int w = this->width();
    int h = this->height();

    // draw background
    r->setStrokeColor(0.3, 0.3, 0.3, 0.8);
    r->setStrokeWeight(1);

    r->setFillColor(0.8,0.8,0.8, 0.9);
    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
    r->drawRect(x,y,w,h);

    // set valueDownButton
    valueUpButton_->setLocalY(h - valueUpButton_->height());

    // set BarWidget
    const int upBottunOffset = valueDownButton_->height();
    const int minBarSize = 4;
    const int barAreaSize = h - 2*w;
    if (scrolledAreaSize() > h) {
        float barRatio = scrollAreaSize() / (float)scrolledAreaSize();
        if (barRatio > 1.0) barRatio = 1.0;
        int barSize = barRatio * barAreaSize;
        if (barSize > barAreaSize) {
            barSize = barAreaSize;
        } else if (barSize < minBarSize){
            barSize = minBarSize;
        }
        const int scrolledAreaLength = scrolledAreaSize() - scrollAreaSize();
        const float barPosRatio = scrollValue() / (float)scrolledAreaLength;
        const int barPosAreaSize = barAreaSize - barSize;
        const int barLocalY = upBottunOffset + barPosRatio * barPosAreaSize;
        barWidget_->setHeight(barSize);
        barWidget_->setLocalY(barLocalY);
    } else {
        barWidget_->setHeight(barAreaSize);
        barWidget_->setLocalY(upBottunOffset);
    }
}

void VerticalScrollBar::setScrollValueFromBarLocalY(int localY)
{
    const int h = this->height();
    const int w = this->width();

    if (scrolledAreaSize() <= h) {
        // max length
        return;
    }

    const int minBarSize = 4;
    const int barAreaSize = h - 2*w;

    float barRatio = scrollAreaSize() / (float)scrolledAreaSize();
    if (barRatio > 1.0) barRatio = 1.0;
    int barSize = barRatio * barAreaSize;
    if (barSize > barAreaSize) {
        barSize = barAreaSize;
    } else if (barSize < minBarSize){
        barSize = minBarSize;
    }
    const int scrolledAreaLength = scrolledAreaSize() - scrollAreaSize();
    const int barPosAreaSize = barAreaSize - barSize;
    int v = (scrolledAreaLength / (float)barPosAreaSize) * localY;
    if (scrollDeltaValue() > 0) {
        int nv = v / scrollDeltaValue();
        v = nv * scrollDeltaValue();
    }
    setScrollValue(v);
}

// VerticalScrollBar::BarWidget
VerticalScrollBar::BarWidget::BarWidget(VerticalScrollBar* scrollBar)
{
    scrollBar_ = scrollBar;
}

void VerticalScrollBar::BarWidget::render2DScene(ptgl::Renderer2D* r)
{
    if (!isEnabled() || !isVisible()) return;

    this->updatePos();

    int x = this->x();
    int y = this->y();

    int w = this->width();
    int h = this->height();

    r->setStrokeColor(0.3, 0.3, 0.3, 0.8);
    r->setStrokeWeight(1);

    r->setFillColor(0.7,0.7,0.8, 0.9);
    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
    r->drawRect(x,y,w,h);
}

void VerticalScrollBar::BarWidget::mousePressEvent(ptgl::GraphicsItemMouseEvent* e)
{
    (void)e;
    moveStartBarLocalY_ = this->localY();
}

void VerticalScrollBar::BarWidget::mouseMoveEvent(ptgl::GraphicsItemMouseEvent* e)
{
    int localY = moveStartBarLocalY_ + e->moveY() - scrollBar_->valueDownButton_->width();
    scrollBar_->setScrollValueFromBarLocalY(localY);
}

// HorizontalScrollBar
HorizontalScrollBar::HorizontalScrollBar() {
    setSize(100, 10);
    setScrolledAreaSize(width());
    init();
}

HorizontalScrollBar::~HorizontalScrollBar() {

}

void HorizontalScrollBar::init()
{
    valueUpButton_ = std::make_shared<PushButton>();
    valueUpButton_->setSize(height(), height());
    valueUpButton_->setOnClickedFunction([&](bool){
        setScrollValue(scrollValue() + scrollDeltaValue());
    });
    addWidget(valueUpButton_);

    valueDownButton_ = std::make_shared<PushButton>();
    valueDownButton_->setSize(height(), height());
    valueDownButton_->setOnClickedFunction([&](bool){
        setScrollValue(scrollValue() - scrollDeltaValue());
    });
    addWidget(valueDownButton_);

    barWidget_ = std::make_shared<BarWidget>(this);
    barWidget_->setSize((width() - 2*height()), height());
    addWidget(barWidget_);
}

void HorizontalScrollBar::setSize(int width, int height)
{
    AbstractScrollBar::setSize(width, height);
    setScrollValue(scrollValue());
}

void HorizontalScrollBar::render2DScene(ptgl::Renderer2D* r)
{
    if (!isEnabled() || !isVisible()) return;

    this->updatePos();

    int x = this->x();
    int y = this->y();

    int w = this->width();
    int h = this->height();

    // draw background
    r->setStrokeColor(0.3, 0.3, 0.3, 0.8);
    r->setStrokeWeight(1);

    r->setFillColor(0.8,0.8,0.8, 0.9);
    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
    r->drawRect(x,y,w,h);

    // set valueDownButton
    valueUpButton_->setLocalX(w - valueUpButton_->width());

    // set BarWidget
    int upBottunOffset = valueDownButton_->width();
    const int minBarSize = 4;
    const int barAreaSize = w - 2*h;
    if (scrolledAreaSize() > h) {
        float barRatio = scrollAreaSize() / (float)scrolledAreaSize();
        if (barRatio > 1.0) barRatio = 1.0;
        int barSize = barRatio * barAreaSize;
        if (barSize > barAreaSize) {
            barSize = barAreaSize;
        } else if (barSize < minBarSize){
            barSize = minBarSize;
        }
        const int scrolledAreaLength = scrolledAreaSize() - scrollAreaSize();
        const float barPosRatio = scrollValue() / (float)scrolledAreaLength;
        const int barPosAreaSize = barAreaSize - barSize;
        const int barLocalX = upBottunOffset + barPosRatio * barPosAreaSize;
        barWidget_->setWidth(barSize);
        barWidget_->setLocalX(barLocalX);
    } else {
        barWidget_->setWidth(barAreaSize);
        barWidget_->setLocalX(upBottunOffset);
    }
}

void HorizontalScrollBar::setScrollValueFromBarLocalX(int localX)
{
    const int h = this->height();
    const int w = this->width();

    if (scrolledAreaSize() <= w) {
        // max length
        return;
    }

    const int minBarSize = 4;
    const int barAreaSize = w - 2*h;

    float barRatio = scrollAreaSize() / (float)scrolledAreaSize();
    if (barRatio > 1.0) barRatio = 1.0;
    int barSize = barRatio * barAreaSize;
    if (barSize > barAreaSize) {
        barSize = barAreaSize;
    } else if (barSize < minBarSize){
        barSize = minBarSize;
    }
    const int scrolledAreaLength = scrolledAreaSize() - scrollAreaSize();
    const int barPosAreaSize = barAreaSize - barSize;
    int v = (scrolledAreaLength / (float)barPosAreaSize) * localX;
    if (scrollDeltaValue() > 0) {
        int nv = v / scrollDeltaValue();
        v = nv * scrollDeltaValue();
    }
    setScrollValue(v);
}

// HorizontalScrollBar::BarWidget
HorizontalScrollBar::BarWidget::BarWidget(HorizontalScrollBar* scrollBar)
{
    scrollBar_ = scrollBar;
}

void HorizontalScrollBar::BarWidget::render2DScene(ptgl::Renderer2D* r)
{
    if (!isEnabled() || !isVisible()) return;

    this->updatePos();

    int x = this->x();
    int y = this->y();

    int w = this->width();
    int h = this->height();

    r->setStrokeColor(0.3, 0.3, 0.3, 0.8);
    r->setStrokeWeight(1);

    r->setFillColor(0.7,0.7,0.8, 0.9);
    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
    r->drawRect(x,y,w,h);
}

void HorizontalScrollBar::BarWidget::mousePressEvent(ptgl::GraphicsItemMouseEvent* e)
{
    (void)e;
    moveStartBarLocalX_ = this->localX();
}

void HorizontalScrollBar::BarWidget::mouseMoveEvent(ptgl::GraphicsItemMouseEvent* e)
{
    int localX = moveStartBarLocalX_ + e->moveX() - scrollBar_->valueDownButton_->width();
    scrollBar_->setScrollValueFromBarLocalX(localX);
}

}
} /* namespace ptgl */
