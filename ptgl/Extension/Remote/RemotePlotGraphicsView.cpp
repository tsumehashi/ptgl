#include "RemotePlotGraphicsView.h"
#include "ptgl/Driver/GLFWGraphicsDriver.h"
namespace ptgl::ext::remote {
RemotePlotGraphicsView::RemotePlotGraphicsView(ptgl::remote::Id id)
    :RemotePlotGraphicsView(std::make_unique<GLFWGraphicsDriver>(GLFWGraphicsDriver::ExecutionMode::CallingThread),id) {}
RemotePlotGraphicsView::RemotePlotGraphicsView(GraphicsDriverPtr driver,ptgl::remote::Id id)
    :PlotGraphicsView(std::move(driver)),receiver_(id),binding_(*this,receiver_,ptgl::remote::ViewKind::plot) {}
RemotePlotGraphicsView::~RemotePlotGraphicsView() { terminate(); waitUntilStopped(); receiver_.disconnect(); }
void RemotePlotGraphicsView::executePrevProcess() { binding_.applyPending(); PlotGraphicsView::executePrevProcess(); }
}
