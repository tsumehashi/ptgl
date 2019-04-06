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
    const auto remvitr = std::remove(addedDockWidgets_.begin(), addedDockWidgets_.end(), widget);
    for (auto itr = remvitr; itr != addedDockWidgets_.end(); ++itr) {
        Widget::removeWidget(*itr);
    }

    addedDockWidgets_.erase(remvitr, addedDockWidgets_.end());

    DockWidget::removeWidget(widget);
}

}
} /* namespace ptgl */
