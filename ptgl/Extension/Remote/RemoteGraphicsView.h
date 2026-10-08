#pragma once
#include "ViewBinding.h"
namespace ptgl::ext::remote {
class RemoteGraphicsView : public GraphicsView {
public:
    explicit RemoteGraphicsView(ptgl::remote::Id viewId=1);
    RemoteGraphicsView(GraphicsDriverPtr driver,ptgl::remote::Id viewId=1);
    ~RemoteGraphicsView() override;
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
