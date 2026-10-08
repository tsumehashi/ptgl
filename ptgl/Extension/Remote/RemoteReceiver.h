#pragma once
#include "ptgl/Remote/Protocol.h"
#include <memory>
namespace ptgl::ext::remote {
// Network transport only. drain() and sendEvent() are called on the owner/view thread.
// One connection subscribes to one view; create another receiver for another view.
class RemoteReceiver {
public:
    explicit RemoteReceiver(ptgl::remote::Id viewId=1);
    ~RemoteReceiver();
    RemoteReceiver(const RemoteReceiver&)=delete;
    RemoteReceiver& operator=(const RemoteReceiver&)=delete;
    bool connect(const std::string& url);
    void disconnect();
    void poll(); // Nonblocking browser reconnect/output pump; harmless on native.
    bool isConnected() const;
    std::string lastError() const;
    std::uint64_t generation() const;
    std::vector<ptgl::remote::Packet> drain();
    bool sendEvent(ptgl::remote::GuiEvent event);
    void fail(const std::string& reason);
    ptgl::remote::Id viewId() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
