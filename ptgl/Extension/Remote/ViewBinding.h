#pragma once
#include "RemoteReceiver.h"
#include "ptgl/Core/GraphicsView.h"
#include "ptgl/GUI/Widget.h"
#include "ptgl/Plot/PlotGraphicsView.h"
namespace ptgl::ext::remote {
// Own on the view thread, and destroy before the bound view.
// Call applyPending() from the frame's pre-process, even while plot updates pause.
class ViewBinding {
public:
    ViewBinding(GraphicsView& view,RemoteReceiver& receiver,ptgl::remote::ViewKind kind);
    ~ViewBinding();
    void applyPending();
    std::size_t numLayers() const;
    gui::WidgetPtr widget(ptgl::remote::Id id) const;
    std::uint64_t appliedRevision() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
