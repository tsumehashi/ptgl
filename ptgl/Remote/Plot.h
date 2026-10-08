#pragma once
#include "Detail/Codec.h"
#include <set>
namespace ptgl::remote {
enum class SeriesKind : std::uint8_t { time=1, xy, scatter, trajectory };
struct SeriesDefinition {
    Id id=1; SeriesKind kind=SeriesKind::time; std::uint32_t channel=0;
    std::string label; std::array<float,4> color{.137f,.404f,.69f,1};
    float lineWidth=2; std::uint32_t capacity=60000; bool visible=true;
};
struct PlotPanel {
    std::string title, xLabel, yLabel;
    double xMin=-1,xMax=1,yMin=-100,yMax=100;
    bool xy=false, equalAspect=false;
    std::vector<SeriesDefinition> series;
};
struct PlotDefinition {
    std::uint32_t channels=1,capacity=60000,columns=1;
    double windowSeconds=60;
    std::string title="Remote plot";
    float leftMargin=0;
    std::vector<PlotPanel> panels;
};
struct SampleBatch {
    std::uint64_t firstIndex=0;
    std::uint32_t channels=1;
    std::vector<std::int64_t> timesNs;
    std::vector<float> values; // Row-major, timesNs.size() * channels.
};
struct XyPoint { double x=0,y=0; bool breakBefore=false; };
struct XyBatch { Id seriesId=1; std::uint64_t firstIndex=0; std::vector<XyPoint> points; };
namespace detail {
inline Bytes encodePlot(const PlotDefinition& d) {
    Writer w; w.u32(d.channels); w.u32(d.capacity); w.u32(d.columns); w.f64(d.windowSeconds); w.text(d.title); w.f32(d.leftMargin);
    require(d.panels.size()<=64,"Too many panels"); w.u32(std::uint32_t(d.panels.size()));
    for(const auto& p:d.panels) {
        w.text(p.title); w.text(p.xLabel); w.text(p.yLabel); w.f64(p.xMin); w.f64(p.xMax); w.f64(p.yMin); w.f64(p.yMax); w.u8(p.xy); w.u8(p.equalAspect);
        require(p.series.size()<=128,"Too many series"); w.u32(std::uint32_t(p.series.size()));
        for(const auto& s:p.series) {
            w.u32(s.id); w.u8(std::uint8_t(s.kind)); w.u32(s.channel); w.text(s.label);
            for(float c:s.color) w.f32(c); w.f32(s.lineWidth); w.u32(s.capacity); w.u8(s.visible);
        }
    }
    return std::move(w.data);
}
inline PlotDefinition decodePlot(const Bytes& data) {
    Reader r(data); PlotDefinition d;
    d.channels=r.u32(); d.capacity=r.u32(); d.columns=r.u32(); d.windowSeconds=r.finite(); d.title=r.text(); d.leftMargin=r.f32();
    require(d.channels>0 && d.channels<=128 && d.capacity>0 && d.capacity<=1000000 && d.columns>0 && d.columns<=16,"Invalid plot dimensions");
    // Conservative admission estimate includes the existing Plot summary tree.
    std::uint64_t budget=std::uint64_t(d.capacity)*(16+16*d.channels);
    require(budget<=Limits::maxState && d.windowSeconds>0 && d.windowSeconds<=86400,"Plot memory/window limit");
    require(std::isfinite(d.leftMargin) && d.leftMargin>=0 && d.leftMargin<=4096,"Invalid plot margin");
    auto n=r.u32(); require(n<=64,"Too many panels"); std::set<Id> ids;
    while(n--) {
        PlotPanel p; p.title=r.text(); p.xLabel=r.text(); p.yLabel=r.text(); p.xMin=r.finite(); p.xMax=r.finite(); p.yMin=r.finite(); p.yMax=r.finite(); p.xy=r.boolean(); p.equalAspect=r.boolean();
        require(p.xMin<p.xMax && p.yMin<p.yMax && std::isfinite(p.xMax-p.xMin) && std::isfinite(p.yMax-p.yMin),"Invalid axes");
        auto m=r.u32(); require(m<=128 && ids.size()+m<=128,"Too many series");
        while(m--) {
            SeriesDefinition s; s.id=r.u32(); s.kind=SeriesKind(r.u8()); s.channel=r.u32(); s.label=r.text();
            for(float& c:s.color) { c=r.f32(); require(std::isfinite(c) && c>=0 && c<=1,"Invalid plot color"); }
            s.lineWidth=r.f32(); s.capacity=r.u32(); s.visible=r.boolean();
            require(s.id && ids.insert(s.id).second && unsigned(s.kind)>=1 && unsigned(s.kind)<=4,"Invalid series");
            require((s.kind==SeriesKind::time)==!p.xy && s.channel<d.channels,"Invalid series axes");
            require(std::isfinite(s.lineWidth) && s.lineWidth>0 && s.lineWidth<=64,"Invalid stroke");
            require(s.capacity>0 && s.capacity<=Limits::maxPoints,"Invalid series capacity");
            if(s.kind!=SeriesKind::time) budget+=std::uint64_t(s.capacity)*32;
            require(budget<=Limits::maxState,"Plot memory limit");
            p.series.push_back(std::move(s));
        }
        d.panels.push_back(std::move(p));
    }
    r.end(); return d;
}
inline Bytes encodeSamples(const SampleBatch& b) {
    require(b.channels && b.channels<=128 && b.timesNs.size()<=Limits::maxPoints,"Invalid batch dimensions");
    require(b.values.size()==b.timesNs.size()*b.channels,"Batch shape mismatch");
    require(b.timesNs.size()*(8+4*b.channels)+16<=Limits::maxPayload,"Batch too large");
    Writer w; w.u64(b.firstIndex); w.u32(b.channels); w.u32(std::uint32_t(b.timesNs.size()));
    for(auto t:b.timesNs) w.i64(t); for(auto v:b.values) w.f32(v);
    return std::move(w.data);
}
inline SampleBatch decodeSamples(const Bytes& data) {
    Reader r(data); SampleBatch b; b.firstIndex=r.u64(); b.channels=r.u32(); auto n=r.u32();
    require(n>0 && n<=Limits::maxPoints && b.channels>0 && b.channels<=128,"Invalid batch dimensions");
    require(std::uint64_t(n)*(8+4*b.channels)==r.remaining() && b.firstIndex<=UINT64_MAX-n,"Batch shape/sequence overflow");
    for(std::uint32_t i=0;i<n;++i) { auto t=r.i64(); require(t>=0 && (i==0 || t>b.timesNs.back()),"Non-monotonic sample time"); b.timesNs.push_back(t); }
    b.values.reserve(std::size_t(n)*b.channels);
    for(std::size_t i=0;i<std::size_t(n)*b.channels;++i) { float v=r.f32(); require(!std::isinf(v),"Infinite sample"); b.values.push_back(v); }
    r.end(); return b;
}
inline Bytes encodeXy(const XyBatch& b) {
    require(b.points.size()<=Limits::maxPoints,"Too many XY points");
    Writer w; w.u32(b.seriesId); w.u64(b.firstIndex); w.u32(std::uint32_t(b.points.size()));
    for(const auto& p:b.points) { w.f64(p.x); w.f64(p.y); w.u8(p.breakBefore); }
    return std::move(w.data);
}
inline XyBatch decodeXy(const Bytes& data) {
    Reader r(data); XyBatch b; b.seriesId=r.u32(); b.firstIndex=r.u64(); auto n=r.u32();
    require(b.seriesId && n<=Limits::maxPoints && std::uint64_t(n)*17==r.remaining() && b.firstIndex<=UINT64_MAX-n,"Invalid XY batch");
    while(n--) { XyPoint p; p.x=r.finite(); p.y=r.finite(); p.breakBefore=r.boolean(); b.points.push_back(p); }
    r.end(); return b;
}
}
}
