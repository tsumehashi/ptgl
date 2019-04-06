#include "DockWidget.h"
#include "ptgl/Core/GraphicsView.h"
#include "ptgl/Core/Renderer2D.h"
#include "ptgl/Core/TextRenderer.h"
#include "ptgl/Core/GraphicsItemEvent.h"

namespace ptgl {
namespace gui {

DockWidget::DockWidget()
{
    init();

    setWindowTitle("DockWidget");
    setSize(180, 380);
}

DockWidget::DockWidget(const std::string& windowTitle)
{
    init();

    setWindowTitle(windowTitle);
    setSize(180, 380);
}

DockWidget::~DockWidget()
{
    clearWidgets();
}

void DockWidget::init()
{
    titleBarButton_ = std::make_shared<PushButton>();
    Widget::addWidget(titleBarButton_);
    titleBarButton_->setText("");
    titleBarButton_->setSize(10, 10);
    titleBarButton_->setCheckable(true);
    titleBarButton_->setChecked(true);

    titleBarButton_->setOnToggledFunction([&](bool toggle){ this->titleBarButtonToggled(toggle); });
    titleBarButton_->setOnPressedFunction([&](){ this->titleBarButtonPressed(); });

    enableTitleBar_ = true;
    titleBarHeight_ = 20;
    padding_ = 4;
    showWidgets_ = true;
}

void DockWidget::titleBarButtonToggled(bool toggle)
{
    showWidgets_ = toggle;

}

void DockWidget::titleBarButtonPressed()
{

}

void DockWidget::setEnableTitleBar(bool enable)
{
    enableTitleBar_ = enable;
}

void DockWidget::clearWidgets()
{
    for (auto widget : addedWidgets_) {
        Widget::removeWidget(widget);
    }

    addedWidgets_.clear();
}

void DockWidget::addWidget(WidgetPtr widget)
{
    Widget::addWidget(widget);
    addedWidgets_.push_back(widget);
}

void DockWidget::removeWidget(WidgetPtr widget)
{
    const auto remvitr = std::remove(addedWidgets_.begin(), addedWidgets_.end(), widget);
    for (auto itr = remvitr; itr != addedWidgets_.end(); ++itr) {
        Widget::removeWidget(*itr);
    }

    addedWidgets_.erase(remvitr, addedWidgets_.end());
}

void DockWidget::render2DScene(ptgl::Renderer2D* r)
{
    this->updatePos();
    int x = this->x();
    int y = this->y();

    int w = this->width();
    int h = this->height();
    int hy = 0;

    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
    r->setStrokeWeight(1);
    r->setStrokeColor(0.3, 0.3, 0.3);

    // draw title bar
    if (isEnabledTitleBar()) {
        titleBarButton_->setVisible(true);
        titleBarButton_->setLocalPos(w - 20, titleBarHeight_/4);

        double tc[] = {0.2, 0.2, 0.2, 0.7};
        r->setFillColor(tc[0], tc[1], tc[2], tc[3]);
        r->drawRect(x, y, w, titleBarHeight_);

        y += titleBarHeight_;
        hy = titleBarHeight_;
    } else {
        titleBarButton_->setVisible(false);
    }

    // draw widgets container
    if (!showWidgets_) {
        for (size_t i = 0; i < this->addedWidgets_.size(); ++i) {
            WidgetPtr child = addedWidgets_[i];
            if (child) {
                child->setVisible(false);
            }
        }

        // update size
        setSize(w, hy);
    } else {
        for (size_t i = 0; i < this->addedWidgets_.size(); ++i) {
            WidgetPtr child = addedWidgets_[i];
            if (child) {
                child->setVisible(true);
            }
        }

        int padx = padding_;
        int pady = padding_;
        int yy = hy + pady;
        w = this->width();
        h = pady;

        for (size_t i = 0; i < this->addedWidgets_.size(); ++i) {
            WidgetPtr child = addedWidgets_[i];
            if (child) {
                int cw = this->width() - padx*2;
                child->setSize(cw, child->height());
                child->setLocalPos(padx, yy);
                yy += pady + child->height();
                h += child->height() + pady;
            }
        }

        // draw background
        if (isEnabledTitleBar()) {
            r->setFillColor(0.6,0.6,0.6, 0.5);

            r->setStrokeColor(0.3, 0.3, 0.3);
            r->setStrokeWeight(1);

            r->drawRect(x,y,w,h);
        }

        // update size
        int uw = x+w - this->x();
        int uh = y+h - this->y();
        setSize(uw, uh);
    }

    // render text
    if (isEnabledTitleBar() && !windowTitle_.empty()) {
        int textSize = titleBarHeight_ * 0.9;
        int tx = this->x() + 10;
        int ty = this->y() + titleBarHeight_ - (titleBarHeight_ - textSize)/2;

        r->setTextColor(1,1,1);
        r->setTextSize(textSize);
        r->drawText(tx, ty, windowTitle_);
    }
}

void DockWidget::mouseMoveEvent(ptgl::GraphicsItemMouseEvent* e)
{
    if (enableMove_) {
        int nx = this->x() + e->dx();
        int ny = this->y() + e->dy();

        bool move = true;
        if ((nx + this->width()) < 10) {
            move = false;
        }
        if ((nx + 10) > this->graphicsWindow()->width()) {
            move = false;
        }
        if (ny < 0) {
            move = false;
        }
        if ((ny + 10) > this->graphicsWindow()->height()) {
            move = false;
        }
        if (move) {
            setPos(this->x() + e->dx(), this->y() + e->dy());
        }
    }
}

void DockWidget::TitleBarButton::render2DScene(ptgl::Renderer2D* r)
{
    this->updatePos();
    int x = this->x();
    int y = this->y();

    r->setFillColor(0.9, 0.9, 0.9);
    r->drawCircle(x, y, 8);
}

}
} /* namespace ptgl */
