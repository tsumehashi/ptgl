#pragma once
#include "Detail/Codec.h"
#include <map>
namespace ptgl::remote {
enum class WidgetKind : std::uint8_t { label=1, button, checkBox, slider, number, text, panel };
struct WidgetDefinition {
    Id id=1, parentId=0; WidgetKind kind=WidgetKind::label;
    std::string text;
    float x=10,y=10,width=180,height=30;
    bool visible=true, enabled=true;
    double value=0, minimum=0, maximum=100;
};
struct GuiTree { std::vector<WidgetDefinition> widgets; };
namespace detail {
inline Bytes encodeGui(const GuiTree& tree) {
    require(tree.widgets.size()<=Limits::maxWidgets,"Too many widgets");
    Writer w; w.u32(std::uint32_t(tree.widgets.size()));
    for(const auto& d:tree.widgets) {
        w.u32(d.id); w.u32(d.parentId); w.u8(std::uint8_t(d.kind)); w.text(d.text);
        w.f32(d.x); w.f32(d.y); w.f32(d.width); w.f32(d.height);
        w.u8(d.visible); w.u8(d.enabled); w.f64(d.value); w.f64(d.minimum); w.f64(d.maximum);
    }
    return std::move(w.data);
}
inline GuiTree decodeGui(const Bytes& data) {
    Reader r(data); GuiTree tree; auto n=r.u32(); require(n<=Limits::maxWidgets,"Too many widgets");
    std::map<Id,WidgetKind> parents;
    while(n--) {
        WidgetDefinition d; d.id=r.u32(); d.parentId=r.u32(); d.kind=WidgetKind(r.u8()); d.text=r.text();
        d.x=r.f32(); d.y=r.f32(); d.width=r.f32(); d.height=r.f32();
        d.visible=r.boolean(); d.enabled=r.boolean(); d.value=r.finite(); d.minimum=r.finite(); d.maximum=r.finite();
        require(d.id && !parents.count(d.id) && unsigned(d.kind)>=1 && unsigned(d.kind)<=7,"Invalid widget");
        require(!d.parentId || (parents.count(d.parentId) && parents.at(d.parentId)==WidgetKind::panel),"Parent must precede child and be panel");
        for(float v:{d.x,d.y,d.width,d.height}) require(std::isfinite(v) && std::abs(v)<=100000,"Invalid widget geometry");
        require(d.width>0 && d.height>0 && d.minimum<=d.maximum,"Invalid widget range");
        require(d.minimum>=-1000000 && d.maximum<=1000000 && std::abs(d.value)<=1000000,"Widget value too large");
        if(d.kind==WidgetKind::slider || d.kind==WidgetKind::number) require(d.value>=d.minimum && d.value<=d.maximum,"Value out of range");
        if(d.kind==WidgetKind::slider || d.kind==WidgetKind::number)
            require(std::floor(d.value)==d.value && std::floor(d.minimum)==d.minimum && std::floor(d.maximum)==d.maximum,"Integer control requires integer values");
        if(d.kind==WidgetKind::checkBox) require(d.value==0 || d.value==1,"Invalid checkbox value");
        parents[d.id]=d.kind; tree.widgets.push_back(std::move(d));
    }
    r.end(); return tree;
}
}
}
