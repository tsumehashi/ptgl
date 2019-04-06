#ifndef PTGL_GUI_DOCKWIDGETPANEL_H_
#define PTGL_GUI_DOCKWIDGETPANEL_H_

#include "DockWidget.h"

namespace ptgl {
namespace gui {

class DockWidgetPanel;
typedef std::shared_ptr<DockWidgetPanel> DockWidgetPanelPtr;

class DockWidgetPanel : public DockWidget {
public:
    DockWidgetPanel();
    DockWidgetPanel(const std::string& windowTitle);
    virtual ~DockWidgetPanel();

    virtual void addDockWidget(DockWidgetPtr widget);
    virtual void removeDockWidget(DockWidgetPtr widget);

protected:
    std::vector<WidgetPtr> addedDockWidgets_;
};

}
} /* namespace ptgl */

#endif /* PTGL_GUI_DOCKWIDGETPANEL_H_ */
