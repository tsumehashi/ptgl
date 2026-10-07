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

    int tn = (w - tw) / std::max(1, r->textWidth());
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
    listView_ = listView;
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
    for (auto p = this; p; p = p->parentNode_)
        if (p == node.get())
            throw std::invalid_argument("Cyclic list hierarchy");
    if (node->parentNode_ == this)
        return;
    if (node->parentNode_)
        node->parentNode_->removeChildNode(node);
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
        (*itr)->setListView(nullptr);
        childrenNode_.erase(itr);
    }
    Widget::removeWidget(item);
}

void ListNodeWidget::addElement(ListElementWidgetPtr element)
{
    if (!element) return;
    if (element->parentNode_ == this)
        return;
    if (element->parentNode_)
        element->parentNode_->removeElement(element);

    element->parentNode_ = this;
    element->setListView(listView_);
    elements_.push_back(element);
    Widget::addWidget(element);
}

void ListNodeWidget::removeElement(ListElementWidgetPtr element)
{
    auto itr = std::find(elements_.begin(), elements_.end(), element);
    if (itr != elements_.end()) {
        (*itr)->parentNode_ = nullptr;
        (*itr)->setListView(nullptr);
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

void ListNodeWidget::layout()
{
    // set ExpandButton
    int buttonSpacing = 3;
    int rankWidth = (expandButton_->width() + buttonSpacing) * rank();
    {
        int ex = buttonSpacing;
        int ey = expandElementsButton_->height() / 2;
        expandElementsButton_->setLocalPos(ex, ey);
    }

    {
        int ex = expandElementsButton_->width() + 2 * buttonSpacing + rankWidth;
        int ey = expandButton_->height() / 2;
        expandButton_->setLocalPos(ex, ey);
    }
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

    int buttonSpacing = 3;
    int rankWidth = (expandButton_->width() + buttonSpacing) * rank();

    // draw text
    {
        int tw = expandElementsButton_->width() + 2*buttonSpacing + rankWidth + expandButton_->width() + 2*buttonSpacing;
        int tx = x + tw;
        int ty = y + r->textHeight();

        int tn = (width() - tw) / std::max(1, r->textWidth());
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
    setClipChildren(true);
    verticalScrollBar_->setHeight(height());
    addWidget(verticalScrollBar_);
}

ListViewWidget::~ListViewWidget()
{

}

void ListViewWidget::setRootListNode(ListNodeWidgetPtr node)
{
    if (rootNode_ == node)
        return;
    if (rootNode_) {
        rootNode_->setListView(nullptr);
        Widget::removeWidget(rootNode_);
    }
    rootNode_ = node;
    if (!node)
        return;
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

void ListViewWidget::layout()
{
    updatePos();
    verticalScrollBar_->setVisible(isEnabledScrollBar_);
    verticalScrollBar_->setLocalPos(width() - verticalScrollBar_->width(), 0);
    int nodeWidth = std::max(0, width() - (isEnabledScrollBar_ ? verticalScrollBar_->width() : 0));
    std::vector<WidgetPtr> rows;
    int contentHeight = 0;
    ListNodeWidget::traverse(rootNode_, [&](ListNodeWidgetPtr node) {
        auto parent = node->parentNode();
        node->rank_ = parent ? parent->rank() + 1 : 0;
        bool shown = node->isSelected() && node->isEnabled() &&
                     (!parent || (parent->isVisible() && parent->isExpanded()));
        // Keep offscreen ancestors visible: clipping hides their own row without hiding descendants.
        node->setVisible(shown);
        if (shown) {
            rows.push_back(node);
            contentHeight += node->height();
        }
        for (auto element : node->elements()) {
            bool showElement =
                shown && element->isSelected() && element->isEnabled() && element->isExpanded();
            element->setVisible(showElement);
            if (showElement) {
                rows.push_back(element);
                contentHeight += element->height();
            }
        }
    });
    setHeightInternal(contentHeight);
    if (rootNode_)
        verticalScrollBar_->setScrollDeltaValue(rootNode_->height());
    int rowY = y() - (isEnabledScrollBar_ ? verticalScrollBar_->scrollValue() : 0);
    for (auto row : rows) {
        row->setWidth(nodeWidth);
        row->setPos(x(), rowY);
        rowY += row->height();
    }
}

void ListViewWidget::render2DScene(ptgl::Renderer2D *r)
{
    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
    r->setFillColor(0.8, 0.8, 0.8, 0.5);
    r->setStrokeColor(0.3, 0.3, 0.3);
    r->setStrokeWeight(1);
    r->drawRect(x(), y(), width(), height());
}
}
} /* namespace ptgl */
