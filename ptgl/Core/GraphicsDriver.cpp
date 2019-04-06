#include "GraphicsDriver.h"
#include "GraphicsView.h"

namespace ptgl {

GraphicsDriver::GraphicsDriver()
{

}

GraphicsDriver::~GraphicsDriver()
{

}

void GraphicsDriver::initialize(GraphicsView* view)
{
    view_ = view;
}

void GraphicsDriver::executeGraphicsViewInitializeEvent()
{
    if (view_) view_->executeGraphicsViewInitializeEvent();
}

void GraphicsDriver::executeGraphicsViewRenderEvent()
{
    if (view_) view_->executeGraphicsViewRenderEvent();
}

void GraphicsDriver::executeGraphicsViewResizeEvent(int width, int height)
{
    if (view_) view_->executeGraphicsViewResizeEvent(width, height);
}

void GraphicsDriver::executeGraphicsViewPrevProcessEvent()
{
    if (view_) view_->executeGraphicsViewPrevProcessEvent();
}

void GraphicsDriver::executeGraphicsViewPostProcessEvent()
{
    if (view_) view_->executeGraphicsViewPostProcessEvent();
}

void GraphicsDriver::executeGraphicsViewPrevEventProcessEvent()
{
    if (view_) view_->executeGraphicsViewPrevEventProcessEvent();
}

void GraphicsDriver::executeGraphicsViewPostEventProcessEvent()
{
    if (view_) view_->executeGraphicsViewPostEventProcessEvent();
}

void GraphicsDriver::executeGraphicsViewMousePressEvent(MouseEvent* e)
{
    if (view_) view_->executeGraphicsViewMousePressEvent(e);
}

void GraphicsDriver::executeGraphicsViewMouseMoveEvent(MouseEvent* e)
{
    if (view_) view_->executeGraphicsViewMouseMoveEvent(e);
}

void GraphicsDriver::executeGraphicsViewMouseReleaseEvent(MouseEvent* e)
{
    if (view_) view_->executeGraphicsViewMouseReleaseEvent(e);
}

void GraphicsDriver::executeGraphicsViewWheelEvent(WheelEvent* e)
{
    if (view_) view_->executeGraphicsViewWheelEvent(e);
}

void GraphicsDriver::executeGraphicsViewKeyPressEvent(KeyEvent* e)
{
    if (view_) view_->executeGraphicsViewKeyPressEvent(e);
}

void GraphicsDriver::executeGraphicsViewDropEvent(DropEvent* e)
{
    if (view_) view_->executeGraphicsViewDropEvent(e);
}


MouseEvent* GraphicsDriver::getGraphicsViewMouseEvent()
{
    return view_ ? view_->getGraphicsViewMouseEvent() : nullptr;
}

WheelEvent* GraphicsDriver::getGraphicsViewWheelEvent()
{
    return view_ ? view_->getGraphicsViewWheelEvent() : nullptr;
}

KeyEvent* GraphicsDriver::getGraphicsViewKeyEvent()
{
    return view_ ? view_->getGraphicsViewKeyEvent() : nullptr;
}

DropEvent* GraphicsDriver::getGraphicsViewDropEvent()
{
    return view_ ? view_->getGraphicsViewDropEvent() : nullptr;
}

} /* namespace ptgl */
