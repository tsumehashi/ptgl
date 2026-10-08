#include <ptgl/Remote/RemotePublisher.h>
#include <iostream>
#include <condition_variable>
#include <stdexcept>
int secondTranslationUnit();
using namespace ptgl::remote;
void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
int main() {
    try {
        check(secondTranslationUnit()==7,"Multiple translation units");
        RemotePublisher publisher; check(publisher.defineView({1,ViewKind::scene3D,"Sender"})==PublishResult::Accepted,"Define view");
        GuiTree gui; WidgetDefinition button; button.kind=WidgetKind::button; gui.widgets.push_back(button); publisher.defineGui(1,gui);
        Frame f; for(unsigned i=0;i<90000;++i) f.drawPoint({double(i),0,0});
        for(Id id=1;id<=6;++id) check(publisher.submitFrame(1,id,f)==PublishResult::Accepted,"Large retained frame");
        check(publisher.updateGui(1,1,1000001)==PublishResult::InvalidArgument,"Invalid GUI update");
        check(publisher.listen({"127.0.0.1",0}),"Listen");
        detail::WsClient client; detail::configureSocket(client);
        std::mutex mutex; std::condition_variable changed; bool snapshot=false,confirmed=false; std::size_t snapshotBytes=0; unsigned layers=0; std::uint64_t session=0,sequence=0,guiRevision=0;
        client.set_open_handler([&](detail::Handle h) {
            Packet p; p.type=MessageType::hello; p.viewId=1; p.sequence=++sequence;
            detail::Writer w; w.u32(1); w.u32(std::uint32_t(Limits::maxPayload)); p.payload=std::move(w.data);
            auto bytes=detail::encode(p); client.send(h,bytes.data(),bytes.size(),websocketpp::frame::opcode::binary);
        });
        client.set_message_handler([&](detail::Handle h,detail::WsClient::message_ptr msg) {
            auto& raw=msg->get_payload(); auto p=detail::decode(raw.data(),raw.size()); session=p.sessionId;
            if(p.type==MessageType::defineGui) guiRevision=p.revision;
            if(p.type==MessageType::replaceLayer) { ++layers; snapshotBytes+=p.byteSize(); }
            if(p.type==MessageType::guiValue) { std::lock_guard<std::mutex> lock(mutex); confirmed=true; changed.notify_all(); }
            Packet ack; ack.type=MessageType::flowAck; ack.sessionId=session; ack.viewId=1; ack.sequence=++sequence;
            detail::Writer w; w.u64(p.byteSize()); ack.payload=std::move(w.data); auto b=detail::encode(ack);
            client.send(h,b.data(),b.size(),websocketpp::frame::opcode::binary);
            if(p.type==MessageType::snapshotEnd) {
                GuiEvent e; e.widgetId=1; e.eventId=1; e.value=1;
                Packet event; event.type=MessageType::guiEvent; event.sessionId=session; event.viewId=1; event.sequence=++sequence; event.revision=guiRevision; event.payload=detail::encodeEvent(e);
                b=detail::encode(event); client.send(h,b.data(),b.size(),websocketpp::frame::opcode::binary);
                {std::lock_guard<std::mutex> lock(mutex); snapshot=true;} changed.notify_all();
            }
        });
        detail::ErrorCode ec; auto connection=client.get_connection("ws://127.0.0.1:"+std::to_string(publisher.port()),ec); check(!ec,"Client setup"); client.connect(connection);
        std::thread worker([&]{client.run();});
        bool ok; {std::unique_lock<std::mutex> lock(mutex); ok=changed.wait_for(lock,std::chrono::seconds(5),[&]{return snapshot;});}
        GuiEvent event; bool received=false;
        for(int i=0;i<200 && !received;++i) { received=publisher.pollEvent(event); if(!received) std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
        auto result=received?publisher.eventResult(event,true):PublishResult::Closed;
        bool confirmedOk; {std::unique_lock<std::mutex> lock(mutex); confirmedOk=changed.wait_for(lock,std::chrono::seconds(5),[&]{return confirmed;});}
        client.stop(); worker.join(); publisher.stop();
        check(ok,"Snapshot timeout"); check(layers==6 && snapshotBytes>Limits::maxQueued,"Large incremental snapshot"); check(confirmedOk,"GUI confirmation delivery"); check(received && event.widgetId==1,"GUI return event"); check(result==PublishResult::Accepted,"GUI confirmation");
        check(publisher.statistics().rejected==0,"Unexpected protocol rejection");
        std::cout<<"Remote sender loopback and ODR tests passed\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<"\n"; return 1; }
}
