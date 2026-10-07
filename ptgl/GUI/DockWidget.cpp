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
    if (showWidgets_ == toggle)
        return;
    showWidgets_ = toggle;
    for (auto &w : addedWidgets_) {
        if (!toggle) {
            collapsedVisibility_[w.get()] = w->isLocallyVisible();
            w->setVisible(false);
        } else {
            auto it = collapsedVisibility_.find(w.get());
            if (it != collapsedVisibility_.end())
                w->setVisible(it->second);
        }
    }
    if (toggle)
        collapsedVisibility_.clear();
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
    collapsedVisibility_.clear();
}

void DockWidget::addWidget(WidgetPtr widget)
{
    if (!widget || std::find(addedWidgets_.begin(), addedWidgets_.end(), widget) != addedWidgets_.end())
        return;
    Widget::addWidget(widget);
    addedWidgets_.push_back(widget);
    if (!showWidgets_) {
        collapsedVisibility_[widget.get()] = widget->isLocallyVisible();
        widget->setVisible(false);
    }
}

void DockWidget::removeWidget(WidgetPtr widget)
{
    if (auto it = collapsedVisibility_.find(widget.get()); it != collapsedVisibility_.end()) {
        widget->setVisible(it->second);
        collapsedVisibility_.erase(it);
    }
    Widget::removeWidget(widget);
    addedWidgets_.erase(std::remove(addedWidgets_.begin(), addedWidgets_.end(), widget), addedWidgets_.end());
}

void DockWidget::layout()
{
    updatePos();
    int title = enableTitleBar_ ? titleBarHeight_ : 0;
    titleBarButton_->setVisible(enableTitleBar_);
    titleBarButton_->setLocalPos(width() - 20, titleBarHeight_ / 4);
    int yy = title + padding_;
    if (showWidgets_)
        for (auto &child : addedWidgets_)
            if (child->isLocallyVisible()) {
                child->setSize(std::max(0, width() - 2 * padding_), child->height());
                child->setLocalPos(padding_, yy);
                yy += child->height() + padding_;
            }
    setHeight(showWidgets_ ? yy : title);
}
void DockWidget::render2DScene(ptgl::Renderer2D *r)
{
    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
    r->setStrokeWeight(1);
    r->setStrokeColor(theme().border);
    r->setFillColor(theme().background);
    int title = enableTitleBar_ ? titleBarHeight_ : 0;
    if (height() > title)
        r->drawRect(x(), y() + title, width(), height() - title);
    if (enableTitleBar_) {
        r->setFillColor(theme().titleBackground);
        r->drawRect(x(), y(), width(), title);
    }
    if (enableTitleBar_ && !windowTitle_.empty()) {
        r->setTextColor(theme().titleText);
        r->setTextSize(theme().pixels(theme().fontSize));
        r->drawText(x() + 10, y() + titleBarHeight_ - 3, windowTitle_);
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
