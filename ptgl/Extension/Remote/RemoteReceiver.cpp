#include "RemoteReceiver.h"
#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#ifdef __EMSCRIPTEN__
#include <emscripten/websocket.h>
#else
#include "ptgl/Remote/Detail/WebSocket.h"
#include <condition_variable>
#include <thread>
#endif

namespace ptgl::ext::remote {
namespace wire=ptgl::remote;
namespace codec=ptgl::remote::detail;
struct RemoteReceiver::Impl {
    using Clock=std::chrono::steady_clock;
    explicit Impl(wire::Id v):view(v) {}
    wire::Id view;
    mutable std::mutex mutex;
    std::atomic<bool> stopped{true},connected{false},invalid{false};
    std::string url,error;
    std::uint64_t session=0,inSequence=0,outSequence=0,eventId=0,epoch=0,ack=0;
    std::size_t queued=0;
    std::deque<wire::Packet> inbox,outbox;
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_WEBSOCKET_T socket=0;
    Clock::time_point nextTry{},openedAt{};
#else
    std::shared_ptr<codec::WsClient> active;
    codec::Handle handle;
    std::thread thread;
    std::condition_variable wake;
#endif
    void opened() {
        std::lock_guard<std::mutex> lock(mutex);
        connected=false; invalid=false; session=0; inSequence=outSequence=eventId=ack=0;
        queued=0; inbox.clear(); outbox.clear(); ++epoch;
        codec::Writer w; w.u32(view); w.u32(std::uint32_t(wire::Limits::maxPayload));
        wire::Packet p; p.type=wire::MessageType::hello; p.viewId=view; p.payload=std::move(w.data); outbox.push_back(std::move(p));
    }
    void failed(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex); error=message; connected=false; invalid=true;
    }
    void received(const void* data,std::size_t size) {
        try {
            auto p=codec::decode(data,size);
            std::lock_guard<std::mutex> lock(mutex);
            codec::require(!invalid && p.viewId==view && p.sequence==inSequence+1,"Invalid view or sequence");
            if(!session) {
                codec::require(p.type==wire::MessageType::welcome && p.sessionId && p.payload.empty(),"Welcome required");
                session=p.sessionId; connected=true; error.clear();
            } else codec::require(p.sessionId==session && p.type!=wire::MessageType::welcome,"Invalid session");
            codec::require(queued+p.byteSize()<=wire::Limits::maxQueued && inbox.size()<4096,"Receive queue exceeded");
            inSequence=p.sequence; queued+=p.byteSize(); inbox.push_back(std::move(p));
        } catch(const std::exception& e) { failed(e.what()); }
    }
    std::vector<wire::Bytes> output() {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<wire::Bytes> result;
        if(invalid) return result;
        if(ack && session) {
            codec::Writer w; w.u64(ack); ack=0;
            wire::Packet p; p.type=wire::MessageType::flowAck; p.payload=std::move(w.data);
            p.viewId=view; p.sessionId=session; p.sequence=++outSequence; result.push_back(codec::encode(p));
        }
        for(unsigned i=0;i<32 && !outbox.empty();++i) {
            auto p=std::move(outbox.front()); outbox.pop_front();
            p.sessionId=session; p.sequence=++outSequence; result.push_back(codec::encode(p));
        }
        return result;
    }
#ifndef __EMSCRIPTEN__
    void timer(const std::shared_ptr<codec::WsClient>& client) {
        client->set_timer(5,[this,weak=std::weak_ptr<codec::WsClient>(client)](codec::ErrorCode ec) {
            auto c=weak.lock(); if(!c || ec || stopped) return;
            if(invalid) { c->stop(); return; }
            if(!handle.expired()) {
                codec::ErrorCode errorCode;
                auto connection=c->get_con_from_hdl(handle,errorCode);
                if(!errorCode && connection->get_state()==websocketpp::session::state::open && connection->get_buffered_amount()<wire::Limits::maxPayload) {
                    for(auto& data:output()) { c->send(handle,data.data(),data.size(),websocketpp::frame::opcode::binary,errorCode); if(errorCode) break; }
                    if(errorCode) { failed(errorCode.message()); c->stop(); return; }
                }
            }
            timer(c);
        });
    }
    void run() {
        while(!stopped) {
            try {
                auto client=std::make_shared<codec::WsClient>(); codec::configureSocket(*client);
                client->set_open_handler([this](codec::Handle h) { handle=h; opened(); });
                client->set_message_handler([this](codec::Handle,const codec::WsClient::message_ptr& m) {
                    if(m->get_opcode()!=websocketpp::frame::opcode::binary) { failed("Binary messages required"); return; }
                    auto& raw=m->get_payload(); received(raw.data(),raw.size());
                });
                client->set_close_handler([this,weak=std::weak_ptr<codec::WsClient>(client)](codec::Handle) { connected=false; if(auto c=weak.lock()) c->stop(); });
                client->set_fail_handler([this,weak=std::weak_ptr<codec::WsClient>(client)](codec::Handle) { failed("WebSocket connection failed"); if(auto c=weak.lock()) c->stop(); });
                {
                    std::lock_guard<std::mutex> lock(mutex); active=client;
                    if(stopped) break;
                }
                codec::ErrorCode ec; auto connection=client->get_connection(url,ec);
                if(ec) throw std::runtime_error(ec.message());
                invalid=false; handle.reset(); client->connect(connection); timer(client); client->run();
            } catch(const std::exception& e) { failed(e.what()); }
            connected=false;
            std::unique_lock<std::mutex> lock(mutex); active.reset();
            wake.wait_for(lock,std::chrono::seconds(1),[this]{return stopped.load();});
        }
    }
#else
    static EM_BOOL onOpen(int,const EmscriptenWebSocketOpenEvent*,void* user) { static_cast<Impl*>(user)->opened(); return EM_TRUE; }
    static EM_BOOL onMessage(int,const EmscriptenWebSocketMessageEvent* e,void* user) {
        auto self=static_cast<Impl*>(user);
        if(e->isText) self->failed("Binary messages required"); else self->received(e->data,e->numBytes);
        return EM_TRUE;
    }
    static EM_BOOL onClose(int,const EmscriptenWebSocketCloseEvent*,void* user) {
        auto self=static_cast<Impl*>(user); self->connected=false; self->invalid=true; return EM_TRUE;
    }
    static EM_BOOL onError(int,const EmscriptenWebSocketErrorEvent*,void* user) { static_cast<Impl*>(user)->failed("WebSocket connection failed"); return EM_TRUE; }
    void deleteSocket() {
        if(socket) { emscripten_websocket_close(socket,1000,"Closing"); emscripten_websocket_delete(socket); socket=0; }
    }
    void pump() {
        if(stopped) return;
        if(socket && !session && Clock::now()-openedAt>std::chrono::seconds(8)) invalid=true;
        if(invalid && socket) { deleteSocket(); connected=false; nextTry=Clock::now()+std::chrono::seconds(1); }
        if(!socket && Clock::now()>=nextTry) {
            EmscriptenWebSocketCreateAttributes a{}; a.url=url.c_str(); a.createOnMainThread=EM_TRUE;
            invalid=false; session=0; openedAt=Clock::now(); socket=emscripten_websocket_new(&a);
            if(socket<=0) { socket=0; failed("WebSocket creation failed"); nextTry=Clock::now()+std::chrono::seconds(1); return; }
            emscripten_websocket_set_onopen_callback(socket,this,onOpen);
            emscripten_websocket_set_onmessage_callback(socket,this,onMessage);
            emscripten_websocket_set_onclose_callback(socket,this,onClose);
            emscripten_websocket_set_onerror_callback(socket,this,onError);
        }
        if(socket) {
            unsigned short state=0; emscripten_websocket_get_ready_state(socket,&state);
            std::size_t buffered=0; emscripten_websocket_get_buffered_amount(socket,&buffered);
            if(state==1 && buffered<wire::Limits::maxPayload) for(auto& b:output())
                if(emscripten_websocket_send_binary(socket,b.data(),std::uint32_t(b.size()))!=EMSCRIPTEN_RESULT_SUCCESS) { failed("WebSocket send failed"); break; }
        }
    }
#endif
    void stop() {
        stopped=true; connected=false;
#ifdef __EMSCRIPTEN__
        deleteSocket();
#else
        { std::lock_guard<std::mutex> lock(mutex); if(active) active->stop(); }
        wake.notify_all(); if(thread.joinable()) thread.join();
#endif
        std::lock_guard<std::mutex> lock(mutex); inbox.clear(); outbox.clear(); queued=ack=0;
    }
};
RemoteReceiver::RemoteReceiver(wire::Id viewId):impl_(std::make_unique<Impl>(viewId)) { codec::require(viewId!=0,"Invalid view ID"); }
RemoteReceiver::~RemoteReceiver() { impl_->stop(); }
bool RemoteReceiver::connect(const std::string& url) {
    impl_->stop();
    if(url.compare(0,5,"ws://")!=0) { impl_->failed("This build supports ws:// URLs"); return false; }
    impl_->url=url; impl_->stopped=false; impl_->invalid=false;
#ifdef __EMSCRIPTEN__
    impl_->nextTry=Impl::Clock::now(); impl_->pump();
#else
    impl_->thread=std::thread([this]{impl_->run();});
#endif
    return true;
}
void RemoteReceiver::disconnect() { impl_->stop(); }
void RemoteReceiver::poll() {
#ifdef __EMSCRIPTEN__
    impl_->pump();
#endif
}
bool RemoteReceiver::isConnected() const { return impl_->connected && !impl_->invalid; }
std::string RemoteReceiver::lastError() const { std::lock_guard<std::mutex> lock(impl_->mutex); return impl_->error; }
std::uint64_t RemoteReceiver::generation() const { std::lock_guard<std::mutex> lock(impl_->mutex); return impl_->epoch; }
std::vector<wire::Packet> RemoteReceiver::drain() {
    poll(); std::lock_guard<std::mutex> lock(impl_->mutex);
    std::vector<wire::Packet> packets;
    while(!impl_->inbox.empty()) { impl_->ack+=impl_->inbox.front().byteSize(); packets.push_back(std::move(impl_->inbox.front())); impl_->inbox.pop_front(); }
    impl_->queued=0; return packets;
}
bool RemoteReceiver::sendEvent(wire::GuiEvent event) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if(!impl_->connected || impl_->invalid || impl_->outbox.size()>=wire::Limits::maxEvents) return false;
    try {
        wire::Packet p; p.type=wire::MessageType::guiEvent; p.viewId=impl_->view; p.revision=event.revision;
        event.eventId=++impl_->eventId; p.payload=codec::encodeEvent(event); codec::decodeEvent(p);
        impl_->outbox.push_back(std::move(p)); return true;
    } catch(const std::exception&) { return false; }
}
void RemoteReceiver::fail(const std::string& reason) { impl_->failed(reason); }
wire::Id RemoteReceiver::viewId() const { return impl_->view; }
}
