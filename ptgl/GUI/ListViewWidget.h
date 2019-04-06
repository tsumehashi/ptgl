#ifndef PTGL_GUI_LISTVIEWWIDGET_H_
#define PTGL_GUI_LISTVIEWWIDGET_H_

#include <vector>
#include "Widget.h"
#include "PushButton.h"
#include "ScrollBar.h"

namespace ptgl {
namespace gui {

class ListElementWidget;
class ListNodeWidget;
class ListViewWidget;

using ListElementWidgetPtr = std::shared_ptr<ListElementWidget>;
using ListNodeWidgetPtr = std::shared_ptr<ListNodeWidget>;
using ListViewWidgetPtr = std::shared_ptr<ListViewWidget>;

class Renderer2D;

class ListElementWidget : public Widget {
public:

    ListElementWidget();
    ListElementWidget(const std::string& typeName, const std::string& name);
    virtual ~ListElementWidget();

    void setTypeName(const std::string& name) { typeName_ = name; }
    const std::string& typeName() const { return typeName_; }

    virtual void setListView(ListViewWidget* listView) { listView_ = listView; }
    ListViewWidget* listView() const { return listView_; }

    ListNodeWidget* parentNode() const { return parentNode_; }

    void setSelected(bool on) { isSelected_ = on; }
    bool isSelected() const { return isSelected_; }

    virtual void setExpanded(bool on) { isExpanded_ = on; }
    bool isExpanded() const { return isExpanded_; }

    virtual void updatePos() override;

protected:
    virtual void render2DScene(ptgl::Renderer2D* r) override;

    friend class ListNodeWidget;
    ListViewWidget* listView_ = nullptr;
    ListNodeWidget* parentNode_ = nullptr;
    std::string typeName_;
    bool isSelected_ = false;
    bool isExpanded_ = false;
};

class ListNodeWidget : public Widget {
public:
    ListNodeWidget();
    ListNodeWidget(const std::string& typeName, const std::string& name);
    virtual ~ListNodeWidget();

    void setTypeName(const std::string& name) { typeName_ = name; }
    const std::string& typeName() const { return typeName_; }

    virtual void setListView(ListViewWidget* listView);
    ListViewWidget* listView() const { return listView_; }

    virtual void addChildNode(ListNodeWidgetPtr node);
    virtual void removeChildNode(ListNodeWidgetPtr item);

    ListNodeWidget* parentNode() const { return parentNode_; }

    std::vector<ListNodeWidgetPtr>& childNodes()  { return childrenNode_; }
    const std::vector<ListNodeWidgetPtr>& childNodes() const { return childrenNode_; }
    size_t numChildNodes() const { return childrenNode_.size(); }
    ListNodeWidgetPtr childNode(size_t i) const { return childrenNode_[i]; }

    virtual void addElement(ListElementWidgetPtr element);
    virtual void removeElement(ListElementWidgetPtr element);
    const std::vector<ListElementWidgetPtr>& elements() const { return elements_; }
    size_t numElements() const { return elements_.size(); }
    ListElementWidgetPtr element(size_t i) const { return elements_[i]; }

    virtual void setSelected(bool on);
    bool isSelected() const { return isSelected_; }

    virtual void setExpanded(bool on);
    bool isExpanded() const { return isExpanded_; }

    virtual void setElementsExpanded(bool on);
    bool isElementsExpanded() const { return isElementsExpanded_; }

    int rank() const;

    bool isRootNode() const;

    virtual void updatePos() override;

    static void traverse(ListNodeWidgetPtr item, std::function<void (ListNodeWidgetPtr)> func);

protected:
    friend class ListViewWidget;

    class ExpandButton : public PushButton {
    public:
        ExpandButton(ListNodeWidget* parentNode);
        virtual void render2DScene(ptgl::Renderer2D* r) override;
        virtual void renderPicking2DScene(ptgl::Renderer2D* r)  override { render2DScene(r); }

    protected:
        ListNodeWidget* parentNode_ = nullptr;
    };

    class ExpandElementsButton : public PushButton {
    public:
        ExpandElementsButton(ListNodeWidget* parentNode);
        virtual void render2DScene(ptgl::Renderer2D* r) override;
        virtual void renderPicking2DScene(ptgl::Renderer2D* r)  override { render2DScene(r); }

    protected:
        ListNodeWidget* parentNode_ = nullptr;
    };

    void init();

    virtual void render2DScene(ptgl::Renderer2D* r) override;

    ListViewWidget* listView_ = nullptr;
    std::string typeName_;
    bool isSelected_ = false;
    bool isExpanded_ = false;
    bool isElementsExpanded_ = false;

    ListNodeWidget* parentNode_ = nullptr;
    std::vector<ListNodeWidgetPtr> childrenNode_;
    int rank_ = 0;

    std::vector<ListElementWidgetPtr> elements_;

    PushButtonPtr expandButton_;
    PushButtonPtr expandElementsButton_;
};

class ListViewWidget : public Widget {
public:
    ListViewWidget();
    virtual ~ListViewWidget();

    void setRootListNode(ListNodeWidgetPtr node);
    ListNodeWidgetPtr rootListNode() { return rootNode_; }

    virtual void setWidth(int width) override;
    virtual void setHeight(int height) override;
    virtual void setSize(int width, int height) override;

    bool isEnabledScrollBar() const { return isEnabledScrollBar_; }
    void setEnabledScrollBar(bool on) { isEnabledScrollBar_ = on; }

protected:
    void setHeightInternal(int height);
    virtual void render2DScene(ptgl::Renderer2D* r) override;

    ListNodeWidgetPtr rootNode_;

    int fixedHeight_;
    bool isEnabledScrollBar_ = true;
    VerticalScrollBarPtr verticalScrollBar_ = std::make_shared<VerticalScrollBar>();
};

}
} /* namespace ptgl */

#endif /* PTGL_GUI_LISTVIEWWIDGET_H_ */
