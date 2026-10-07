#include "DockWidgetPanel.h"
#include <algorithm>

namespace ptgl {
namespace gui {

DockWidgetPanel::DockWidgetPanel()
{

}

DockWidgetPanel::~DockWidgetPanel()
{

}

DockWidgetPanel::DockWidgetPanel(const std::string& windowTitle)
    : DockWidget(windowTitle)
{

}

void DockWidgetPanel::addDockWidget(DockWidgetPtr widget)
{
    DockWidget::addWidget(widget);

    addedDockWidgets_.push_back(widget);

    widget->setEnableMove(false);
}

void DockWidgetPanel::removeDockWidget(DockWidgetPtr widget)
{
    addedDockWidgets_.erase(std::remove(addedDockWidgets_.begin(), addedDockWidgets_.end(), widget),
                            addedDockWidgets_.end());
    DockWidget::removeWidget(widget);
}

}
} /* namespace ptgl */
