#pragma once
#include "ViewBinding.h"
namespace ptgl::ext::remote {
class RemotePlotGraphicsView : public plot::PlotGraphicsView {
public:
    explicit RemotePlotGraphicsView(ptgl::remote::Id viewId=1);
    RemotePlotGraphicsView(GraphicsDriverPtr driver,ptgl::remote::Id viewId=1);
    ~RemotePlotGraphicsView() override;
    bool connect(const std::string& url) { return receiver_.connect(url); }
    RemoteReceiver& receiver() { return receiver_; }
    ViewBinding& binding() { return binding_; }
protected:
    void executePrevProcess() override;
private:
    RemoteReceiver receiver_;
    ViewBinding binding_;
};
}
