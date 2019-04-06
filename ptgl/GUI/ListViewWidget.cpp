#include "ListViewWidget.h"
#include <algorithm>
#include "ptgl/Core/Renderer2D.h"

namespace ptgl {
namespace gui {

// ListElementWidget
ListElementWidget::ListElementWidget()
{
    setSize(100, 20);
}

ListElementWidget::ListElementWidget(const std::string& typeName, const std::string& name)
    : typeName_(typeName)
{
    setSize(100, 20);
    setName(name);
}

ListElementWidget::~ListElementWidget()
{

}

void ListElementWidget::updatePos()
{
    // do nothing
}

void ListElementWidget::render2DScene(ptgl::Renderer2D* r)
{
    if (!isEnabled() || !isVisible() || !isExpanded()) return;

    if (!parentNode()) return;

    this->updatePos();
    int x = this->x();
    int y = this->y();

    int w = parentNode()->width();
    int h = this->height();

    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
//    r->setFillColor(0.7,0.7,0.7, 0.9);
    if (isChecked()) {
        r->setFillColor(0.9,0.7,0.7, 0.9);
    } else {
        r->setFillColor(0.7,0.7,0.7, 0.9);
    }
    r->setStrokeColor(0.3, 0.3, 0.3);
    r->setStrokeWeight(1);

    r->drawRect(x,y,w,h);

    // render text
    r->setTextColor(1,1,1);
    int textSize = this->height() * 0.9;
    r->setTextSize(textSize);
    int buttonSpacing = 3;
    int buttonSize = 10;
    int rankWidth = (buttonSize + buttonSpacing) * parentNode()->rank();
    int tw = buttonSize + 2*buttonSpacing + rankWidth + buttonSize + 2*buttonSpacing;

    int tx = x + tw;
    int ty = y + r->textHeight();

    int tn = (w - tw)/r->textWidth();
    if (tn > 0) {
        std::string text = name();
        r->drawText(tx, ty, text.substr(0, std::min<int>(tn, text.size())));
    }
}

// ListNodeWidget
ListNodeWidget::ListNodeWidget()
{
    setSize(100, 20);
    init();
}

ListNodeWidget::ListNodeWidget(const std::string& typeName, const std::string& name)
    : typeName_(typeName)
{
    setSize(100, 20);
    setName(name);
    init();
}

ListNodeWidget::~ListNodeWidget()
{

}

void ListNodeWidget::init()
{
    expandButton_ = std::make_shared<ExpandButton>(this);
    expandButton_->setSize(10,10);
    expandButton_->setCheckable(true);
    expandButton_->setChecked(isExpanded_);
    expandButton_->setOnToggledFunction([&](bool toggle){
        setExpanded(toggle);
    });
    addWidget(expandButton_);

    expandElementsButton_ = std::make_shared<ExpandElementsButton>(this);
    expandElementsButton_->setSize(10,10);
    expandElementsButton_->setCheckable(true);
    expandElementsButton_->setChecked(isElementsExpanded_);
    expandElementsButton_->setOnToggledFunction([&](bool toggle){
        setElementsExpanded(toggle);
    });
    addWidget(expandElementsButton_);
}

void ListNodeWidget::setListView(ListViewWidget* listView)
{
    for (auto&& item : elements_) {
        item->setListView(listView);
    }

    for (auto&& child : childrenNode_) {
        child->setListView(listView);
    }
}

void ListNodeWidget::addChildNode(ListNodeWidgetPtr node)
{
    if (!node) return;
    node->parentNode_ = this;
    node->setListView(listView_);
    childrenNode_.push_back(node);
    Widget::addWidget(node);
}

void ListNodeWidget::removeChildNode(ListNodeWidgetPtr item)
{
    auto itr = std::find(childrenNode_.begin(), childrenNode_.end(), item);
    if (itr != childrenNode_.end()) {
        (*itr)->parentNode_ = nullptr;
        (*itr)->listView_ = nullptr;
        childrenNode_.erase(itr);
    }
    Widget::removeWidget(item);
}

void ListNodeWidget::addElement(ListElementWidgetPtr element)
{
    if (!element) return;

    element->parentNode_ = this;
    elements_.push_back(element);
    Widget::addWidget(element);
}

void ListNodeWidget::removeElement(ListElementWidgetPtr element)
{
    auto itr = std::find(elements_.begin(), elements_.end(), element);
    if (itr != elements_.end()) {
        (*itr)->parentNode_ = nullptr;
        elements_.erase(itr);
    }
    Widget::removeWidget(element);
}

int ListNodeWidget::rank() const
{
    return rank_;
}

bool ListNodeWidget::isRootNode() const
{
    return !parentNode_;
}

void ListNodeWidget::traverse(ListNodeWidgetPtr item, std::function<void (ListNodeWidgetPtr)> func)
{
    if (!item) return;
    if (!func) return;

    func(item);
    for (auto ptr : item->childrenNode_) {
        ListNodeWidget::traverse(ptr, func);
    }
}

void ListNodeWidget::setSelected(bool on)
{
    isSelected_ = on;
    for (auto&& ptr : elements_) {
        ptr->setSelected(on);
    }
}

void ListNodeWidget::setExpanded(bool on)
{
    isExpanded_ = on;
    expandButton_->setChecked(on);
    // close children
    if (!on) {
        setElementsExpanded(on);
        for (auto ptr : childrenNode_) {
            ListNodeWidget::traverse(ptr, [on](ListNodeWidgetPtr item){
                item->setExpanded(on);
            });
        }
    }
}

void ListNodeWidget::setElementsExpanded(bool on)
{
    isElementsExpanded_ = on;
    expandElementsButton_->setChecked(on);
    for (auto&& ptr : elements_) {
        ptr->setExpanded(on);
    }
}

void ListNodeWidget::updatePos()
{
    // do nothing
}

void ListNodeWidget::render2DScene(ptgl::Renderer2D* r)
{
    if (!isEnabled() || !isVisible()) return;
    if (parentNode() && !parentNode()->isExpanded()) {
        return;
    }

    this->updatePos();
    int x = this->x();
    int y = this->y();

    int w = this->width();
    int h = this->height();

    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
//    r->setFillColor(0.6,0.6,0.6, 0.9);

    if (isChecked()) {
        r->setFillColor(0.8,0.6,0.6, 0.9);
    } else {
        r->setFillColor(0.6,0.6,0.6, 0.9);
    }

    r->setStrokeColor(0.3, 0.3, 0.3);
    r->setStrokeWeight(1);

    r->drawRect(x,y,w,h);

    // render text
    r->setTextColor(1,1,1);
    int textSize = this->height() * 0.9;
    r->setTextSize(textSize);

    // set ExpandButton
    int buttonSpacing = 3;
    int rankWidth = (expandButton_->width() + buttonSpacing) * rank();
    {
        int ex = buttonSpacing;
        int ey = expandElementsButton_->height()/2;
        expandElementsButton_->setLocalPos(ex, ey);
    }

    {
        int ex = expandElementsButton_->width() + 2*buttonSpacing + rankWidth;
        int ey = expandButton_->height()/2;
        expandButton_->setLocalPos(ex, ey);
    }

    // draw text
    {
        int tw = expandElementsButton_->width() + 2*buttonSpacing + rankWidth + expandButton_->width() + 2*buttonSpacing;
        int tx = x + tw;
        int ty = y + r->textHeight();

        int tn = (width() - tw)/r->textWidth();
        if (tn > 0) {
            std::string text = name();
            r->drawText(tx, ty, text.substr(0, std::min<int>(tn, text.size())));
        }
    }
}

// ListNodeWidget::ExpandButton
ListNodeWidget::ExpandButton::ExpandButton(ListNodeWidget* parentNode)
{
    parentNode_ = parentNode;
}

void ListNodeWidget::ExpandButton::render2DScene(ptgl::Renderer2D* r)
{
    if (!isEnabled() || !isVisible()) return;
    if (!parentNode_) return;
    if (parentNode_->parentNode() && !parentNode_->parentNode()->isExpanded()) {
        return;
    }

    this->updatePos();

    int x = this->x();
    int y = this->y();

    int w = this->width();
    int h = this->height();

    r->setStrokeColor(0.3, 0.3, 0.3, 0.8);
    r->setStrokeWeight(1);

    r->setFillColor(color_);
    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
    r->drawRect(x,y,w,h);

    if (parentNode_->numChildNodes() > 0) {
        int sp = (w - (w*0.6))/2;
        // -
        {
            int x1 = x + sp;
            int x2 = x + w - sp;
            int y1 = y + h/2;
            int y2 = y + h/2;
            r->drawLine(x1, y1, x2, y2);
        }

        // + expanded
        if (!(isDown() || isChecked())) {
            int x1 = x + w/2;
            int x2 = x + w/2;
            int y1 = y + sp;
            int y2 = y + h - sp;
            r->drawLine(x1, y1, x2, y2);
        }
    }
}

// ListNodeWidget::ExpandElementsButton
ListNodeWidget::ExpandElementsButton::ExpandElementsButton(ListNodeWidget* parentNode)
{
    parentNode_ = parentNode;
}

void ListNodeWidget::ExpandElementsButton::render2DScene(ptgl::Renderer2D* r)
{
    if (!isEnabled() || !isVisible()) return;
    if (!parentNode_) return;
    if (parentNode_->parentNode() && !parentNode_->parentNode()->isExpanded()) {
        return;
    }

    this->updatePos();

    int x = this->x();
    int y = this->y();

    int w = this->width();
    int h = this->height();

    r->setStrokeColor(0.3, 0.3, 0.3, 0.8);
    r->setStrokeWeight(1);

    r->setFillColor(color_);
    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
    int rd = w/2;
    r->drawCircle(x+w/2,y+h/2, rd);

    if (parentNode_->numElements() > 0) {
        int sp = (w - (w*0.6))/2;
        // -
        {
            int x1 = x + sp;
            int x2 = x + w - sp;
            int y1 = y + h/2;
            int y2 = y + h/2;
            r->drawLine(x1, y1, x2, y2);
        }

        // + expanded
        if (!(isDown() || isChecked())) {
            int x1 = x + w/2;
            int x2 = x + w/2;
            int y1 = y + sp;
            int y2 = y + h - sp;
            r->drawLine(x1, y1, x2, y2);
        }
    }
}

// ListViewWidget
ListViewWidget::ListViewWidget()
{
    setSize(100, 200);
    verticalScrollBar_->setHeight(height());
    addWidget(verticalScrollBar_);
}

ListViewWidget::~ListViewWidget()
{

}

void ListViewWidget::setRootListNode(ListNodeWidgetPtr node)
{
    if (!node) return;

    rootNode_ = node;
    rootNode_->setListView(this);
    Widget::addWidget(node);

    // resize
    rootNode_->rank_ = 0;
    ListNodeWidget::traverse(rootNode_, [&](ListNodeWidgetPtr node){
        node->setWidth(width());
        // calc rank
        if (auto parent = node->parentNode()) {
            node->rank_ = parent->rank() + 1;
        }
    });
}

void ListViewWidget::setWidth(int width)
{
    Widget::setWidth(width);
    int nodeWidth = width;
    if (isEnabledScrollBar_) {
        nodeWidth = width - verticalScrollBar_->width();
    }
    ListNodeWidget::traverse(rootNode_, [&](ListNodeWidgetPtr node){
        node->setWidth(nodeWidth);
    });
}

void ListViewWidget::setHeight(int height)
{
    Widget::setHeight(height);
    verticalScrollBar_->setHeight(height);
    fixedHeight_ = height;
}

void ListViewWidget::setSize(int width, int height)
{
    Widget::setSize(width, height);

    // set width
    int nodeWidth = width;
    if (isEnabledScrollBar_) {
        nodeWidth = width - verticalScrollBar_->width();
    }
    ListNodeWidget::traverse(rootNode_, [&](ListNodeWidgetPtr node){
        node->setWidth(nodeWidth);
    });

    // set height
    verticalScrollBar_->setHeight(height);
    fixedHeight_ = height;
}

void ListViewWidget::setHeightInternal(int height)
{
    if (isEnabledScrollBar_) {
        Widget::setHeight(fixedHeight_);
        verticalScrollBar_->setHeight(fixedHeight_);
    } else {
        Widget::setHeight(height);
        verticalScrollBar_->setHeight(height);
    }
    verticalScrollBar_->setScrolledAreaSize(height);
}

void ListViewWidget::render2DScene(ptgl::Renderer2D* r)
{
    if (!isEnabled() || !isVisible()) return;

    this->updatePos();
    int x = this->x();
    int y = this->y();

    int w = this->width();
    int h = this->height();

    // set ScrollBar
    int nodeWidth = w;
    verticalScrollBar_->setVisible(isEnabledScrollBar_);
    if (isEnabledScrollBar_) {
        int scrollLocalX = (w - verticalScrollBar_->width());
        verticalScrollBar_->setLocalPos(scrollLocalX, 0);
        nodeWidth = w - verticalScrollBar_->width();
        if (rootNode_) {
            verticalScrollBar_->setScrollDeltaValue(rootNode_->height());
        }
    }

    int startAreaY = y + verticalScrollBar_->scrollValue();
    if (startAreaY > (y + verticalScrollBar_->scrolledAreaSize() - h)) {
        startAreaY = y + verticalScrollBar_->scrolledAreaSize() - h;
    }
    if (verticalScrollBar_->height() > verticalScrollBar_->scrolledAreaSize()) {
        startAreaY = y;
    }
    int endAreaY = startAreaY + h;

    int px = x;
    int py = y;
    // update children
    ListNodeWidget::traverse(rootNode_, [&](ListNodeWidgetPtr node){

        if (node->isSelected() && node->isEnabled()) {
            if (!node->parentNode() || (node->parentNode() && node->parentNode()->isExpanded())) {
                // draw scroll area
                if ((startAreaY <= py) && ((py + node->height()) <= endAreaY)) {
                    node->setWidth(nodeWidth);
                    node->setPos(px, py - startAreaY + y);
                    node->setVisible(true);
                } else {
                    node->setVisible(false);
                }
                py += node->height();
            }

            // elements
            for (auto&& element : node->elements()) {
                if (element->isSelected() && element->isEnabled()) {
                    if (element->isExpanded()) {
                        // draw scroll area
                        if ((startAreaY <= py) && ((py + element->height()) <= endAreaY)) {
                            element->setWidth(nodeWidth);
                            element->setPos(px, py - startAreaY + y);
                            element->setVisible(true);
                        } else {
                            element->setVisible(false);
                        }
                        py += element->height();
                    }
                }
            }
        } else {
            node->setLocalPos(0,0);
            node->setPos(0,0);
            node->updatePos();
        }
    });

    int rh = py - y;
    setHeightInternal(rh);
    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
//    r->setFillColor(0.6,0.6,0.6, 0.5);
    r->setFillColor(0.8,0.8,0.8, 0.5);
    r->setStrokeColor(0.3, 0.3, 0.3);
    r->setStrokeWeight(1);

    if (isEnabledScrollBar_) {
        r->drawRect(x,y,w,fixedHeight_);
    } else {
        r->drawRect(x,y,w,rh);
    }

}

}
} /* namespace ptgl */
