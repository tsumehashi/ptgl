#pragma once
#include "Frame.h"
#include "Gui.h"
#include "Plot.h"
namespace ptgl::remote {
enum class MessageType : std::uint16_t {
    hello=1,welcome,flowAck,snapshotBegin,snapshotEnd,defineView,replaceLayer,clearLayer,
    defineGui,guiValue,definePlot,appendSamples,setXyData,appendXy,guiEvent,error
};
struct Packet {
    MessageType type=MessageType::hello; Id viewId=0;
    std::uint64_t sessionId=0,sequence=0,revision=0;
    Bytes payload;
    std::size_t byteSize() const { return 48+payload.size(); }
};
namespace detail {
inline Bytes encode(const Packet& p) {
    require(p.payload.size()<=Limits::maxPayload,"Message too large");
    Writer w; w.u32(0x52475450); w.u16(1); w.u16(0); w.u16(std::uint16_t(p.type)); w.u16(0);
    w.u32(48); w.u32(std::uint32_t(p.payload.size())); w.u32(p.viewId); w.u64(p.sessionId); w.u64(p.sequence); w.u64(p.revision); w.bytes(p.payload);
    return std::move(w.data);
}
inline Packet decode(const void* data,std::size_t size) {
    require(size>=48 && size<=48+Limits::maxPayload,"Invalid message size"); Reader r(data,size);
    require(r.u32()==0x52475450 && r.u16()==1 && r.u16()==0,"Unsupported protocol");
    Packet p; p.type=MessageType(r.u16()); require(unsigned(p.type)>=1 && unsigned(p.type)<=16 && r.u16()==0,"Unsupported message/flags");
    require(r.u32()==48,"Unsupported header"); auto n=r.u32(); p.viewId=r.u32(); p.sessionId=r.u64(); p.sequence=r.u64(); p.revision=r.u64();
    require(n==r.remaining(),"Payload length mismatch"); p.payload=r.bytes(n); return p;
}
inline Bytes encodeView(const ViewDefinition& d) { Writer w; w.u8(std::uint8_t(d.kind)); w.text(d.title); return std::move(w.data); }
inline ViewDefinition decodeView(Id id,const Bytes& b) { Reader r(b); ViewDefinition v; v.id=id; v.kind=ViewKind(r.u8()); v.title=r.text(); r.end(); require(id && (v.kind==ViewKind::scene3D || v.kind==ViewKind::plot),"Invalid view"); return v; }
inline Bytes encodeEvent(const GuiEvent& e) { Writer w; w.u64(e.eventId); w.u32(e.widgetId); w.f64(e.value); w.text(e.text); return std::move(w.data); }
inline GuiEvent decodeEvent(const Packet& p) { Reader r(p.payload); GuiEvent e; e.viewId=p.viewId; e.revision=p.revision; e.eventId=r.u64(); e.widgetId=r.u32(); e.value=r.finite(); e.text=r.text(); r.end(); require(e.widgetId && std::abs(e.value)<=1000000,"Invalid GUI event"); return e; }
}
}
