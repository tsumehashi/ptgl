#pragma once
#include "Protocol.h"
#include "Detail/WebSocket.h"
#include <atomic>
#include <chrono>
#include <deque>
#include <map>
#include <mutex>
#include <random>
#include <thread>
#include <tuple>

namespace ptgl::remote {
// One application thread owns control/publication. I/O runs on an internal thread.
// Accepted transfers ownership; it does not guarantee delivery or display.
class RemotePublisher {
    using Key=std::tuple<Id,MessageType,Id>;
    using Clock=std::chrono::steady_clock;
    struct Pending { std::shared_ptr<Packet> packet; bool replaceable=false; Id object=0; };
    struct Client {
        detail::Handle handle;
        std::uint64_t id=0,sequence=0,inSequence=0,lastEvent=0;
        Id view=0; bool ready=false;
        std::size_t queued=0,inFlight=0;
        Clock::time_point lastAck=Clock::now(), opened=Clock::now();
        std::deque<Pending> pending;
        std::deque<std::shared_ptr<Packet>> snapshot;
    };
    struct ViewState {
        ViewDefinition definition;
        GuiTree gui; std::uint64_t guiRevision=0;
        PlotDefinition plot; bool hasPlot=false;
        bool hasSamples=false; std::uint64_t nextSample=0; std::int64_t lastTime=0;
        std::map<Id,std::uint64_t> nextXy;
    };
public:
    RemotePublisher() {
        std::random_device random;
        session_=(std::uint64_t(random())<<32)^random();
        if (!session_) session_=1;
    }
    ~RemotePublisher() { stop(); }
    RemotePublisher(const RemotePublisher&)=delete;
    RemotePublisher& operator=(const RemotePublisher&)=delete;
    bool listen(ListenOptions options={}) {
        stop();
        try {
            endpoint_=std::make_unique<detail::WsServer>(); detail::configureSocket(*endpoint_);
            endpoint_->set_reuse_addr(true);
            endpoint_->set_validate_handler([this](detail::Handle) { std::lock_guard<std::mutex> lock(mutex_); return clients_.size()<Limits::maxClients; });
            endpoint_->set_open_handler([this](detail::Handle h) {
                std::lock_guard<std::mutex> lock(mutex_);
                if(clients_.size()>=Limits::maxClients) {
                    detail::ErrorCode ec; endpoint_->close(h,websocketpp::close::status::policy_violation,"Client limit",ec); return;
                }
                Client c; c.handle=h; c.id=++peerCounter_; clients_.emplace(h,std::move(c));
            });
            endpoint_->set_close_handler([this](detail::Handle h) { std::lock_guard<std::mutex> lock(mutex_); clients_.erase(h); });
            endpoint_->set_message_handler([this](detail::Handle h,detail::WsServer::message_ptr m) { receive(h,m); });
            endpoint_->listen(asio::ip::tcp::endpoint(asio::ip::make_address(options.address),options.port));
            detail::ErrorCode localError; port_=endpoint_->get_local_endpoint(localError).port();
            if(localError) throw std::runtime_error(localError.message());
            endpoint_->start_accept(); running_=true; tick();
            worker_=std::thread([this] { try { endpoint_->run(); } catch(const std::exception& e) { setError(e.what()); } running_=false; });
            return true;
        } catch(const std::exception& e) { setError(e.what()); running_=false; endpoint_.reset(); return false; }
    }
    void stop() {
        running_=false;
        if(endpoint_) endpoint_->stop();
        if(worker_.joinable()) worker_.join();
        endpoint_.reset();
        std::lock_guard<std::mutex> lock(mutex_); clients_.clear(); events_.clear();
    }
    bool isOpen() const { return running_; }
    std::uint16_t port() const { return port_; }
    std::string lastError() const { std::lock_guard<std::mutex> lock(mutex_); return error_; }
    Statistics statistics() const {
        std::lock_guard<std::mutex> lock(mutex_); auto s=stats_; s.clients=clients_.size();
        for(auto& c:clients_) s.queuedBytes+=c.second.queued; return s;
    }
    PublishResult defineView(ViewDefinition d) {
        return guarded([&] {
            auto data=detail::encodeView(d); detail::decodeView(d.id,data);
            std::lock_guard<std::mutex> lock(mutex_);
            detail::require(views_.count(d.id) || views_.size()<Limits::maxViews,"Too many views");
            if(views_.count(d.id)) detail::require(views_.at(d.id).definition.kind==d.kind,"Cannot change view kind");
            auto result=retain(d.id,MessageType::defineView,0,std::move(data));
            if(result==PublishResult::Accepted) views_[d.id].definition=std::move(d);
            return result;
        });
    }
    PublishResult submitFrame(Id view,Id layer,Frame frame) {
        return guarded([&] {
            detail::require(layer!=0,"Invalid layer"); Frame::validate(frame.data());
            detail::Writer w; w.u32(layer); w.bytes(frame.data());
            std::lock_guard<std::mutex> lock(mutex_); requireView(view,ViewKind::scene3D);
            std::size_t count=0; for(auto& p:state_) if(std::get<0>(p.first)==view && std::get<1>(p.first)==MessageType::replaceLayer) ++count;
            detail::require(count<Limits::maxLayers || state_.count({view,MessageType::replaceLayer,layer}),"Too many layers");
            return retain(view,MessageType::replaceLayer,layer,std::move(w.data));
        });
    }
    PublishResult clearLayer(Id view,Id layer) {
        return guarded([&] {
            std::lock_guard<std::mutex> lock(mutex_); requireView(view,ViewKind::scene3D);
            auto it=state_.find({view,MessageType::replaceLayer,layer});
            if(it!=state_.end()) { stateBytes_-=it->second->byteSize(); state_.erase(it); }
            detail::Writer w; w.u32(layer); broadcast(make(view,MessageType::clearLayer,std::move(w.data)),false,layer);
            return PublishResult::Accepted;
        });
    }
    PublishResult defineGui(Id view,GuiTree tree) {
        return guarded([&] {
            auto data=detail::encodeGui(tree); detail::decodeGui(data);
            std::lock_guard<std::mutex> lock(mutex_); detail::require(views_.count(view),"Unknown view");
            auto result=retain(view,MessageType::defineGui,0,std::move(data));
            if(result==PublishResult::Accepted) { views_[view].gui=std::move(tree); views_[view].guiRevision=revision_; }
            return result;
        });
    }
    PublishResult definePlot(Id view,PlotDefinition plot) {
        return guarded([&] {
            auto data=detail::encodePlot(plot); detail::decodePlot(data);
            std::lock_guard<std::mutex> lock(mutex_); requireView(view,ViewKind::plot);
            auto result=retain(view,MessageType::definePlot,0,std::move(data));
            if(result==PublishResult::Accepted) {
                auto& v=views_[view]; v.plot=std::move(plot); v.hasPlot=true; v.hasSamples=false; v.nextXy.clear();
                for(auto it=state_.begin();it!=state_.end();) {
                    if(std::get<0>(it->first)==view && std::get<1>(it->first)==MessageType::setXyData) { stateBytes_-=it->second->byteSize(); it=state_.erase(it); } else ++it;
                }
            }
            return result;
        });
    }
    PublishResult appendSamples(Id view,const SampleBatch& samples) {
        return guarded([&] {
            auto data=detail::encodeSamples(samples); auto b=detail::decodeSamples(data);
            std::lock_guard<std::mutex> lock(mutex_); requireView(view,ViewKind::plot); auto& v=views_[view];
            detail::require(v.hasPlot && b.channels==v.plot.channels,"Unknown plot/channels");
            detail::require(!v.hasSamples || (b.firstIndex>=v.nextSample && b.timesNs.front()>v.lastTime),"Overlapping sample batch");
            if(!running_) return PublishResult::Closed;
            v.hasSamples=true; v.nextSample=b.firstIndex+b.timesNs.size(); v.lastTime=b.timesNs.back();
            broadcast(make(view,MessageType::appendSamples,std::move(data)),false,0,b.timesNs.size());
            return PublishResult::Accepted;
        });
    }
    PublishResult setXyData(Id view,const XyBatch& data) { return xy(view,data,false); }
    PublishResult appendXy(Id view,const XyBatch& data) { return xy(view,data,true); }
    bool pollEvent(GuiEvent& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        while(!events_.empty()) {
            auto e=std::move(events_.front()); events_.pop_front();
            bool live=false; for(auto& c:clients_) if(c.second.id==e.peerId && c.second.ready) live=true;
            if(live && views_.count(e.viewId) && views_.at(e.viewId).guiRevision==e.revision) { event=std::move(e); return true; }
        }
        return false;
    }
    // Confirms an optimistic GUI edit, or restores the sender's previous value.
    PublishResult eventResult(const GuiEvent& event,bool accepted) {
        return guarded([&] {
            std::lock_guard<std::mutex> lock(mutex_);
            auto vi=views_.find(event.viewId); detail::require(vi!=views_.end() && vi->second.guiRevision==event.revision,"Stale GUI event");
            bool live=false; for(auto& c:clients_) if(c.second.ready && c.second.id==event.peerId) live=true;
            detail::require(live,"Disconnected GUI event");
            return updateGuiLocked(event.viewId,event.widgetId,event.value,event.text,accepted,event.eventId);
        });
    }
    PublishResult updateGui(Id view,Id widget,double value,const std::string& text={}) {
        return guarded([&] {
            std::lock_guard<std::mutex> lock(mutex_);
            detail::require(views_.count(view),"Unknown GUI view");
            return updateGuiLocked(view,widget,value,text,true,0);
        });
    }
private:
    PublishResult updateGuiLocked(Id view,Id widget,double value,const std::string& text,bool accepted,std::uint64_t eventId) {
        auto& v=views_.at(view); auto tree=v.gui;
        for(auto& d:tree.widgets) if(d.id==widget) {
            if(accepted) { d.value=value; if(d.kind==WidgetKind::text) d.text=text; }
            auto bytes=detail::encodeGui(tree); detail::decodeGui(bytes);
            auto retained=state_.at({view,MessageType::defineGui,0});
            if(stateBytes_-retained->byteSize()+48+bytes.size()>Limits::maxState) return PublishResult::TooLarge;
            auto replacement=std::make_shared<Packet>(*retained); replacement->payload=std::move(bytes);
            GuiEvent confirmed; confirmed.widgetId=widget; confirmed.value=d.value; confirmed.text=d.text; confirmed.eventId=eventId;
            auto packet=make(view,MessageType::guiValue,detail::encodeEvent(confirmed)); packet->revision=v.guiRevision;
            stateBytes_=stateBytes_-retained->byteSize()+replacement->byteSize();
            state_[{view,MessageType::defineGui,0}]=replacement; v.gui=std::move(tree);
            broadcast(packet,false,0); return PublishResult::Accepted;
        }
        return PublishResult::InvalidArgument;
    }
    template<class Function> PublishResult guarded(Function&& function) {
        try { return function(); } catch(const std::bad_alloc&) { return PublishResult::TooLarge; }
        catch(const std::exception&) { return PublishResult::InvalidArgument; }
    }
    void setError(std::string text) { std::lock_guard<std::mutex> lock(mutex_); error_=std::move(text); }
    void requireView(Id view,ViewKind kind) { detail::require(views_.count(view) && views_.at(view).definition.kind==kind,"Unknown view/type"); }
    std::shared_ptr<Packet> make(Id view,MessageType type,Bytes bytes={}) {
        auto p=std::make_shared<Packet>(); p->viewId=view; p->type=type; p->sessionId=session_; p->revision=++revision_; p->payload=std::move(bytes); return p;
    }
    PublishResult retain(Id view,MessageType type,Id object,Bytes bytes) {
        Key key{view,type,object}; auto old=state_.find(key); std::size_t oldSize=old==state_.end()?0:old->second->byteSize();
        if(bytes.size()>Limits::maxPayload || stateBytes_-oldSize+48+bytes.size()>Limits::maxState) return PublishResult::TooLarge;
        auto p=make(view,type,std::move(bytes)); stateBytes_=stateBytes_-oldSize+p->byteSize(); state_[key]=p;
        broadcast(p,type==MessageType::replaceLayer,object); return PublishResult::Accepted;
    }
    bool enqueue(Client& c,std::shared_ptr<Packet> p,bool replaceable=false,Id object=0) {
        if(replaceable) for(auto it=c.pending.rbegin();it!=c.pending.rend();++it) {
            auto& pending=*it; if(!pending.replaceable) break;
            if(pending.object==object && pending.packet->type==p->type) {
                if(c.queued-pending.packet->byteSize()+p->byteSize()>Limits::maxQueued) return false;
                c.queued=c.queued-pending.packet->byteSize()+p->byteSize(); pending.packet=std::move(p); return true;
            }
        }
        if(c.queued+p->byteSize()>Limits::maxQueued || c.pending.size()>=4096) return false;
        c.queued+=p->byteSize(); c.pending.push_back({std::move(p),replaceable,object}); return true;
    }
    void disconnect(Client& c,const char* reason) {
        c.ready=false; c.pending.clear(); c.snapshot.clear(); c.queued=0;
        detail::ErrorCode ec; endpoint_->close(c.handle,websocketpp::close::status::policy_violation,reason,ec);
    }
    void broadcast(const std::shared_ptr<Packet>& p,bool replaceable,Id object,std::size_t samples=0) {
        for(auto& item:clients_) {
            auto& c=item.second; if(!c.ready || c.view!=p->viewId) continue;
            if(!enqueue(c,p,replaceable,object)) {
                if(samples) stats_.droppedSamples+=samples;
                else disconnect(c,"Receiver queue exceeded");
            }
        }
    }
    PublishResult xy(Id view,const XyBatch& b,bool append) {
        return guarded([&] {
            auto bytes=detail::encodeXy(b); detail::decodeXy(bytes);
            std::lock_guard<std::mutex> lock(mutex_); requireView(view,ViewKind::plot); auto& v=views_[view];
            detail::require(v.hasPlot,"Unknown plot"); const SeriesDefinition* found=nullptr;
            for(auto& p:v.plot.panels) for(auto& s:p.series) if(s.id==b.seriesId) found=&s;
            detail::require(found && (append?found->kind==SeriesKind::trajectory:(found->kind==SeriesKind::xy || found->kind==SeriesKind::scatter)),"Invalid XY series");
            if(!append) {
                detail::require(b.points.size()<=found->capacity,"Static XY capacity exceeded");
                return retain(view,MessageType::setXyData,b.seriesId,std::move(bytes));
            }
            if(!running_) return PublishResult::Closed;
            detail::require(!v.nextXy.count(b.seriesId) || b.firstIndex>=v.nextXy[b.seriesId],"Overlapping XY batch");
            v.nextXy[b.seriesId]=b.firstIndex+b.points.size();
            broadcast(make(view,MessageType::appendXy,std::move(bytes)),false,0,b.points.size()); return PublishResult::Accepted;
        });
    }
    void receive(detail::Handle h,const detail::WsServer::message_ptr& message) {
        std::lock_guard<std::mutex> lock(mutex_); auto it=clients_.find(h); if(it==clients_.end()) return; auto& c=it->second;
        try {
            detail::require(message->get_opcode()==websocketpp::frame::opcode::binary,"Binary required");
            auto& raw=message->get_payload(); auto p=detail::decode(raw.data(),raw.size());
            detail::require(p.sequence==c.inSequence+1,"Invalid sequence"); c.inSequence=p.sequence;
            if(!c.ready) {
                detail::require(p.type==MessageType::hello && p.sessionId==0,"Hello required");
                detail::Reader r(p.payload); c.view=r.u32(); auto limit=r.u32(); r.end();
                detail::require(views_.count(c.view) && limit>=Limits::maxPayload,"Unknown view or insufficient message limit");
                c.ready=true;
                // Capture immutable retained packets. Send incrementally under the same flow window.
                c.snapshot.push_back(make(c.view,MessageType::welcome));
                c.snapshot.push_back(make(c.view,MessageType::snapshotBegin));
                for(auto& s:state_) if(std::get<0>(s.first)==c.view) c.snapshot.push_back(s.second);
                c.snapshot.push_back(make(c.view,MessageType::snapshotEnd));
            } else {
                detail::require(p.sessionId==session_ && p.viewId==c.view,"Stale session/view");
                if(p.type==MessageType::flowAck) {
                    detail::Reader r(p.payload); auto bytes=r.u64(); r.end(); detail::require(bytes<=c.inFlight,"Invalid flow acknowledgement");
                    c.inFlight-=std::size_t(bytes); c.lastAck=Clock::now();
                } else if(p.type==MessageType::guiEvent) {
                    auto e=detail::decodeEvent(p); auto& v=views_.at(c.view);
                    detail::require(e.eventId>c.lastEvent && e.revision==v.guiRevision,"Stale GUI event");
                    auto d=std::find_if(v.gui.widgets.begin(),v.gui.widgets.end(),[&](const auto& w){ return w.id==e.widgetId; });
                    detail::require(d!=v.gui.widgets.end() && d->enabled && d->visible && d->kind!=WidgetKind::label && d->kind!=WidgetKind::panel,"Invalid GUI target");
                    auto candidate=*d; candidate.parentId=0; candidate.value=e.value;
                    if(candidate.kind==WidgetKind::text) candidate.text=e.text;
                    detail::decodeGui(detail::encodeGui(GuiTree{{candidate}}));
                    Id parent=d->parentId;
                    while(parent) {
                        auto ancestor=std::find_if(v.gui.widgets.begin(),v.gui.widgets.end(),[&](const auto& w){return w.id==parent;});
                        detail::require(ancestor!=v.gui.widgets.end() && ancestor->enabled && ancestor->visible,"Inactive GUI parent");
                        parent=ancestor->parentId;
                    }
                    detail::require(events_.size()<Limits::maxEvents,"GUI event queue exceeded");
                    c.lastEvent=e.eventId; e.peerId=c.id; events_.push_back(std::move(e));
                } else throw detail::ProtocolError("Unexpected client message");
            }
            ++stats_.received;
        } catch(const std::exception&) { ++stats_.rejected; disconnect(c,"Invalid protocol"); }
    }
    void tick() {
        if(!running_) return;
        endpoint_->set_timer(5,[this](detail::ErrorCode ec) {
            if(ec || !running_) return;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                for(auto& item:clients_) {
                    auto& c=item.second;
                    if((!c.ready && Clock::now()-c.opened>std::chrono::seconds(5)) ||
                       (c.inFlight && Clock::now()-c.lastAck>std::chrono::seconds(10))) { disconnect(c,"Receiver timeout"); continue; }
                    detail::ErrorCode error; auto connection=endpoint_->get_con_from_hdl(c.handle,error);
                    if(error) continue;
                    unsigned count=0;
                    while(c.ready && (!c.snapshot.empty() || !c.pending.empty()) && count++<32) {
                        const bool snapshot=!c.snapshot.empty();
                        auto p=*(snapshot?c.snapshot.front():c.pending.front().packet);
                        if(c.inFlight+p.byteSize()>Limits::maxInFlight || connection->get_buffered_amount()+p.byteSize()>Limits::maxInFlight) break;
                        p.sequence=++c.sequence; auto bytes=detail::encode(p);
                        endpoint_->send(c.handle,bytes.data(),bytes.size(),websocketpp::frame::opcode::binary,error);
                        if(error) { disconnect(c,"Send failed"); break; }
                        if(!c.inFlight) c.lastAck=Clock::now();
                        c.inFlight+=p.byteSize();
                        if(snapshot) c.snapshot.pop_front();
                        else { c.queued-=p.byteSize(); c.pending.pop_front(); }
                        ++stats_.sent;
                    }
                }
            }
            tick();
        });
    }
    mutable std::mutex mutex_;
    std::unique_ptr<detail::WsServer> endpoint_; std::thread worker_; std::atomic<bool> running_{false};
    std::uint16_t port_=0;
    std::uint64_t session_=0,revision_=0,peerCounter_=0;
    std::map<detail::Handle,Client,std::owner_less<detail::Handle>> clients_;
    std::map<Id,ViewState> views_; std::map<Key,std::shared_ptr<Packet>> state_;
    std::size_t stateBytes_=0; std::deque<GuiEvent> events_; Statistics stats_; std::string error_;
};
} // namespace ptgl::remote
