#include "RemoteGraphicsView.h"
#include "ptgl/Driver/GLFWGraphicsDriver.h"
namespace ptgl::ext::remote {
RemoteGraphicsView::RemoteGraphicsView(ptgl::remote::Id id)
    :RemoteGraphicsView(std::make_unique<GLFWGraphicsDriver>(GLFWGraphicsDriver::ExecutionMode::CallingThread),id) {}
RemoteGraphicsView::RemoteGraphicsView(GraphicsDriverPtr driver,ptgl::remote::Id id)
    :GraphicsView(std::move(driver)),receiver_(id),binding_(*this,receiver_,ptgl::remote::ViewKind::scene3D) {
    setBackgroundColor(.965,.973,.980); setSwapInterval(1); setFrameRate(0);
}
RemoteGraphicsView::~RemoteGraphicsView() { terminate(); waitUntilStopped(); receiver_.disconnect(); }
void RemoteGraphicsView::executePrevProcess() { binding_.applyPending(); GraphicsView::executePrevProcess(); }
}
