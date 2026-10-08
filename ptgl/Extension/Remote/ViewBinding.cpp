#include "ViewBinding.h"
#include "ptgl/Core/RenderItem.h"
#include "ptgl/GUI/Layout.h"
#include "ptgl/GUI/PushButton.h"
#include "ptgl/GUI/Slider.h"
#include "ptgl/GUI/SpinBox.h"
#include "ptgl/GUI/TextEditWidget.h"
#include <map>
#include <optional>

namespace ptgl::ext::remote {
namespace w=ptgl::remote;
namespace c=ptgl::remote::detail;
namespace {
class RecordedItem final : public GraphicsItem {
public:
    Render3DItem commands;
    RecordedItem() { setPickable(false); }
    void renderScene(Renderer3D* renderer) override { renderer->renderRecorded(commands); }
};
std::shared_ptr<RecordedItem> recording(const w::Bytes& bytes) {
    w::Frame::validate(bytes); auto item=std::make_shared<RecordedItem>(); auto& dst=item->commands;
    dst.setColor(1,1,1,1); dst.setLineWidth(1); dst.setPointSize(1); dst.setMaterial({.35,.04});
    c::Reader r(bytes);
    while(r.remaining()) {
        auto op=w::DrawOp(r.u16()); r.u16(); auto length=r.u32(); double v[10]{};
        for(unsigned i=0;i<length/8;++i) v[i]=r.f64();
        Eigen::Matrix3d rotation=Eigen::Matrix3d::Identity();
        if(op==w::DrawOp::box || op==w::DrawOp::cylinder || op==w::DrawOp::axis || op==w::DrawOp::transform)
            rotation=Eigen::Quaterniond(v[6],v[3],v[4],v[5]).normalized().toRotationMatrix();
        switch(op) {
        case w::DrawOp::color: dst.setColor(v[0],v[1],v[2],v[3]); break;
        case w::DrawOp::lineWidth: dst.setLineWidth(v[0]); break;
        case w::DrawOp::pointSize: dst.setPointSize(v[0]); break;
        case w::DrawOp::point: dst.drawPoint(v); break;
        case w::DrawOp::line: dst.drawLine(v,v+3); break;
        case w::DrawOp::box: dst.drawBox(v,rotation.data(),v+7); break;
        case w::DrawOp::sphere: dst.drawSphere(v,rotation.data(),v[3]); break;
        case w::DrawOp::cylinder: dst.drawCylinder(v,rotation.data(),v[7],v[8]); break;
        case w::DrawOp::axis: dst.drawAxis(v,rotation.data(),v[7]); break;
        case w::DrawOp::pushMatrix: dst.pushMatrix(); break;
        case w::DrawOp::popMatrix: dst.popMatrix(); break;
        case w::DrawOp::identity: dst.identiry(); break;
        case w::DrawOp::translate: dst.translate(v[0],v[1],v[2]); break;
        case w::DrawOp::scale: dst.scale(v[0],v[1],v[2]); break;
        case w::DrawOp::transform: dst.transform(Eigen::Vector3d(v),rotation); break;
        case w::DrawOp::material: dst.setMaterial({v[0],v[1]}); break;
        }
    }
    c::require(dst.valid() && dst.byteSize()<=RenderCommandSender::MaxFrameBytes,"Converted frame too large");
    return item;
}
}
struct ViewBinding::Impl {
    struct Model {
        std::optional<w::ViewDefinition> definition;
        std::map<w::Id,std::shared_ptr<RecordedItem>> layers;
        w::GuiTree gui; std::uint64_t guiRevision=0;
        std::map<w::Id,gui::WidgetPtr> widgets;
        std::unique_ptr<plot::Figure> pendingFigure;
        std::optional<w::PlotDefinition> plotDefinition;
        std::map<w::Id,std::pair<std::size_t,std::size_t>> series;
        std::map<w::Id,std::uint32_t> xyCapacity;
        std::uint64_t nextSample=0; std::int64_t origin=0,lastTime=0; bool sampled=false;
        std::map<w::Id,std::uint64_t> nextXy;
    };
    GraphicsView& view; RemoteReceiver& receiver; w::ViewKind kind;
    plot::PlotGraphicsView* plotView;
    Model active; std::unique_ptr<Model> staging;
    std::shared_ptr<int> lifetime=std::make_shared<int>(0);
    bool ready=false,wasConnected=false; std::uint64_t revision=0; float dpi=-1;
    Impl(GraphicsView& v,RemoteReceiver& r,w::ViewKind k):view(v),receiver(r),kind(k),plotView(dynamic_cast<plot::PlotGraphicsView*>(&v)) {
        c::require((kind==w::ViewKind::plot)==(plotView!=nullptr),"Binding/view type mismatch");
    }
    void detach(Model& m) {
        for(auto& e:m.layers) view.removeGraphicsItem(e.second);
        for(auto& d:m.gui.widgets) if(!d.parentId) { m.widgets.at(d.id)->setEnabled(false); view.removeGraphicsItem(m.widgets.at(d.id)); }
    }
    void attach(Model& m) {
        if(m.definition) view.setWindowTitle(m.definition->title);
        for(auto& e:m.layers) view.addGraphicsItem(e.second);
        for(auto& d:m.gui.widgets) if(!d.parentId) view.addGraphicsItem(m.widgets.at(d.id));
        if(m.pendingFigure) { plotView->figure()=std::move(*m.pendingFigure); m.pendingFigure.reset(); }
        if(m.plotDefinition) plotView->setPlotMargins(m.plotDefinition->leftMargin,0,0,0);
        dpi=-1; view.notifySceneChanged();
    }
    plot::Figure& figure(Model& m) { c::require(m.plotDefinition.has_value(),"Plot not defined"); return m.pendingFigure?*m.pendingFigure:plotView->figure(); }
    void value(gui::WidgetPtr widget,const w::WidgetDefinition& d) {
        if(auto b=std::dynamic_pointer_cast<gui::PushButton>(widget)) b->setCheckedSilently(d.value!=0);
        if(auto s=std::dynamic_pointer_cast<gui::AbstractSlider>(widget)) s->setValue(int(d.value),false);
        if(auto t=std::dynamic_pointer_cast<gui::TextEditWidget>(widget)) t->setText(d.text,false);
    }
    void defineGui(Model& m,const w::Packet& p) {
        auto tree=c::decodeGui(p.payload); std::map<w::Id,gui::WidgetPtr> widgets;
        for(auto& d:tree.widgets) {
            gui::WidgetPtr widget;
            auto send=[this,weak=std::weak_ptr<int>(lifetime),id=d.id,guiRevision=p.revision](double value,const std::string& text) {
                if(weak.expired()) return;
                w::GuiEvent event; event.widgetId=id; event.value=value; event.text=text; event.revision=guiRevision;
                if(!receiver.sendEvent(std::move(event))) receiver.fail("GUI event could not be queued");
            };
            switch(d.kind) {
            case w::WidgetKind::label: widget=std::make_shared<gui::Label>(d.text); break;
            case w::WidgetKind::panel: widget=std::make_shared<gui::Panel>(d.text); break;
            case w::WidgetKind::button:
            case w::WidgetKind::checkBox: {
                auto b=std::make_shared<gui::PushButton>(d.text); b->setCheckable(d.kind==w::WidgetKind::checkBox);
                b->setOnClickedFunction([send](bool value){send(value?1:0,{});}); widget=b; break;
            }
            case w::WidgetKind::slider:
            case w::WidgetKind::number: {
                std::shared_ptr<gui::AbstractSlider> s;
                if(d.kind==w::WidgetKind::slider) s=std::make_shared<gui::Slider>(d.text);
                else s=std::make_shared<gui::SpinBox>(d.text);
                s->setRange(int(d.minimum),int(d.maximum));
                s->setOnValueChangedFunction([send](int value){send(value,{});}); widget=s; break;
            }
            case w::WidgetKind::text: {
                auto t=std::make_shared<gui::TextEditWidget>(); t->setOnTextChangedFunction([send](const auto& value,const auto&){send(0,value);}); widget=t; break;
            }
            }
            value(widget,d); widget->setVisible(d.visible); widget->setEnabled(d.enabled);
            widget->setPreferredSize(int(d.width),int(d.height));
            widgets.emplace(d.id,widget); if(d.parentId) widgets.at(d.parentId)->addWidget(widget);
        }
        if(&m==&active) for(auto& d:m.gui.widgets) if(!d.parentId) { m.widgets.at(d.id)->setEnabled(false); view.removeGraphicsItem(m.widgets.at(d.id)); }
        m.gui=std::move(tree); m.widgets=std::move(widgets); m.guiRevision=p.revision;
        if(&m==&active) for(auto& d:m.gui.widgets) if(!d.parentId) view.addGraphicsItem(m.widgets.at(d.id));
        dpi=-1;
    }
    void definePlot(Model& m,const w::Packet& p) {
        c::require(kind==w::ViewKind::plot,"Plot message in 3D view");
        auto d=c::decodePlot(p.payload);
        auto f=std::make_unique<plot::Figure>(d.channels,d.capacity,d.windowSeconds);
        f->setTitle(d.title).setColumns(d.columns); f->setLiveZoomEnabled(true);
        std::map<w::Id,std::pair<std::size_t,std::size_t>> series;
        std::map<w::Id,std::uint32_t> xyCapacity;
        for(auto& panel:d.panels) {
            auto& a=panel.xy?f->addXyAxes(panel.title,panel.xLabel,panel.yLabel):f->addAxes(panel.title,panel.yLabel,{panel.yMin,panel.yMax});
            if(panel.xy) a.setXRange({panel.xMin,panel.xMax}).setYRange({panel.yMin,panel.yMax}).setEqualAspectEnabled(panel.equalAspect);
            for(auto& def:panel.series) {
                plot::Series* s=nullptr;
                switch(def.kind) {
                case w::SeriesKind::time: s=&a.addSeries(def.channel,def.label); break;
                case w::SeriesKind::xy: s=&a.addXySeries({},def.label); break;
                case w::SeriesKind::scatter: s=&a.addScatterSeries({},def.label); break;
                case w::SeriesKind::trajectory: s=&a.addTrajectory(def.capacity,def.label); break;
                }
                s->setColor(def.color[0],def.color[1],def.color[2],def.color[3]).setLineWidth(def.lineWidth).setVisible(def.visible);
                series[def.id]={f->numAxes()-1,a.numSeries()-1};
                if(def.kind!=w::SeriesKind::time) xyCapacity[def.id]=def.capacity;
            }
        }
        m.plotDefinition=std::move(d); m.series=std::move(series); m.xyCapacity=std::move(xyCapacity); m.sampled=false; m.nextXy.clear();
        if(&m==&active) { plotView->figure()=std::move(*f); plotView->setPlotMargins(m.plotDefinition->leftMargin,0,0,0); }
        else m.pendingFigure=std::move(f);
    }
    void apply(Model& m,const w::Packet& p) {
        switch(p.type) {
        case w::MessageType::defineView: {
            auto d=c::decodeView(p.viewId,p.payload); c::require(d.kind==kind,"View kind mismatch");
            m.definition=std::move(d); if(&m==&active) view.setWindowTitle(m.definition->title); break;
        }
        case w::MessageType::replaceLayer: {
            c::require(kind==w::ViewKind::scene3D,"3D message in plot view"); c::Reader r(p.payload); auto id=r.u32();
            c::require(id && (m.layers.count(id) || m.layers.size()<w::Limits::maxLayers),"Layer limit");
            auto item=recording(r.bytes(r.remaining()));
            std::size_t bytes=item->commands.byteSize();
            for(auto& e:m.layers) if(e.first!=id) bytes+=e.second->commands.byteSize();
            c::require(bytes<=w::Limits::maxState/2,"Scene memory limit");
            if(&m==&active) { if(m.layers.count(id)) view.removeGraphicsItem(m.layers.at(id)); view.addGraphicsItem(item); }
            m.layers[id]=std::move(item); break;
        }
        case w::MessageType::clearLayer: {
            c::Reader r(p.payload); auto id=r.u32(); r.end(); auto it=m.layers.find(id);
            if(it!=m.layers.end()) { if(&m==&active) view.removeGraphicsItem(it->second); m.layers.erase(it); } break;
        }
        case w::MessageType::defineGui: defineGui(m,p); break;
        case w::MessageType::guiValue: {
            auto event=c::decodeEvent(p); c::require(event.revision==m.guiRevision,"Stale GUI value"); bool found=false;
            for(auto& d:m.gui.widgets) if(d.id==event.widgetId) {
                if(d.kind==w::WidgetKind::slider || d.kind==w::WidgetKind::number)
                    c::require(event.value>=d.minimum && event.value<=d.maximum,"GUI value out of range");
                auto candidate=d; candidate.value=event.value; if(d.kind==w::WidgetKind::text) candidate.text=event.text;
                candidate.parentId=0; c::decodeGui(c::encodeGui(w::GuiTree{{candidate}}));
                candidate.parentId=d.parentId; d=std::move(candidate);
                value(m.widgets.at(d.id),d); found=true; break;
            }
            c::require(found,"Unknown GUI value target"); break;
        }
        case w::MessageType::definePlot: definePlot(m,p); break;
        case w::MessageType::appendSamples: {
            c::require(kind==w::ViewKind::plot && !staging,"Samples during snapshot or wrong view");
            auto b=c::decodeSamples(p.payload); auto& f=figure(m);
            c::require(b.channels==m.plotDefinition->channels,"Wrong sample channels");
            c::require(!m.sampled || (b.firstIndex>=m.nextSample && b.timesNs.front()>m.lastTime),"Overlapping samples");
            if(!m.sampled) m.origin=b.timesNs.front();
            if(m.sampled && b.firstIndex!=m.nextSample) f.breakBeforeNextSample();
            double previous=f.data().newestTime();
            for(auto t:b.timesNs) { double time=double(t-m.origin)*1e-9; c::require(!std::isfinite(previous) || time>previous,"Sample time precision exceeded"); previous=time; }
            for(std::size_t i=0;i<b.timesNs.size();++i)
                c::require(f.append(double(b.timesNs[i]-m.origin)*1e-9,b.values.data()+i*b.channels,b.channels),"Sample append failed");
            m.sampled=true; m.nextSample=b.firstIndex+b.timesNs.size(); m.lastTime=b.timesNs.back(); break;
        }
        case w::MessageType::setXyData:
        case w::MessageType::appendXy: {
            c::require(kind==w::ViewKind::plot,"XY in 3D view"); auto b=c::decodeXy(p.payload); auto& f=figure(m);
            c::require(m.series.count(b.seriesId),"Unknown XY series"); auto index=m.series.at(b.seriesId); auto& s=f.axes(index.first).series(index.second);
            if(p.type==w::MessageType::setXyData) {
                c::require(s.isXy() && !s.isTrajectory(),"Not a static XY series");
                c::require(b.points.size()<=m.xyCapacity.at(b.seriesId),"Static XY capacity exceeded");
                std::vector<plot::XYPoint> points; for(auto& point:b.points) points.push_back({point.x,point.y,point.breakBefore}); s.setXyData(std::move(points));
            } else {
                c::require(s.isTrajectory() && !staging,"Not a trajectory or streaming during snapshot");
                bool gap=m.nextXy.count(b.seriesId) && b.firstIndex!=m.nextXy[b.seriesId];
                c::require(!m.nextXy.count(b.seriesId) || b.firstIndex>=m.nextXy[b.seriesId],"Overlapping trajectory");
                for(auto& point:b.points) { s.appendXy(point.x,point.y,point.breakBefore || gap); gap=false; }
                m.nextXy[b.seriesId]=b.firstIndex+b.points.size();
            }
            break;
        }
        default: throw c::ProtocolError("Unexpected server message");
        }
        if(&m==&active) view.notifySceneChanged();
    }
    void updateGui() {
        const bool connected=receiver.isConnected() && ready;
        if(wasConnected && !connected) for(auto& d:active.gui.widgets) if(!d.parentId) view.clearItemInput(active.widgets.at(d.id));
        wasConnected=connected;
        const float scale=std::max(.01f,view.pixelRatio());
        for(auto& d:active.gui.widgets) {
            auto widget=active.widgets.at(d.id); widget->setEnabled(d.enabled && connected);
            if(dpi!=scale && !d.parentId) {
                auto theme=gui::Theme::light(); theme.scale*=scale; widget->setTheme(theme);
                widget->setPos(int(d.x*scale),int(d.y*scale)); widget->setSize(int(d.width*scale),int(d.height*scale));
            }
        }
        dpi=scale;
    }
    void process() {
        auto packets=receiver.drain();
        try {
            for(auto& p:packets) {
                if(p.type==w::MessageType::welcome) { staging.reset(); ready=false; continue; }
                if(p.type==w::MessageType::snapshotBegin) {
                    c::require(p.payload.empty() && !staging,"Nested snapshot"); staging=std::make_unique<Model>(); continue;
                }
                if(p.type==w::MessageType::snapshotEnd) {
                    c::require(p.payload.empty() && staging && staging->definition,"Incomplete snapshot");
                    detach(active); active=std::move(*staging); staging.reset(); attach(active); ready=true; revision=p.revision; continue;
                }
                c::require(staging || ready,"Snapshot required");
                apply(staging?*staging:active,p); if(!staging) revision=p.revision;
            }
        } catch(const std::exception& e) { staging.reset(); ready=false; receiver.fail(e.what()); }
        updateGui();
    }
};
ViewBinding::ViewBinding(GraphicsView& view,RemoteReceiver& receiver,w::ViewKind kind):impl_(std::make_unique<Impl>(view,receiver,kind)) {}
ViewBinding::~ViewBinding() { impl_->lifetime.reset(); impl_->detach(impl_->active); }
void ViewBinding::applyPending() { impl_->process(); }
std::size_t ViewBinding::numLayers() const { return impl_->active.layers.size(); }
gui::WidgetPtr ViewBinding::widget(w::Id id) const { auto i=impl_->active.widgets.find(id); return i==impl_->active.widgets.end()?nullptr:i->second; }
std::uint64_t ViewBinding::appliedRevision() const { return impl_->revision; }
}
