#include <ptgl/Remote/Protocol.h>
#include <iostream>
#include <stdexcept>
using namespace ptgl::remote;
void check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
template<class F> void rejects(F f) { bool rejected=false; try {f();} catch(const std::exception&) {rejected=true;} check(rejected,"Invalid input accepted"); }
int main() {
    try {
        Packet p; p.viewId=0x12345678; p.sessionId=0x0102030405060708ull;
        auto bytes=detail::encode(p);
        check(bytes.size()==48 && bytes[0]=='P' && bytes[3]=='R' && bytes[20]==0x78 && bytes[24]==8 && bytes[31]==1,"Wire layout");
        auto read=detail::decode(bytes.data(),bytes.size()); check(read.viewId==p.viewId && read.sessionId==p.sessionId,"Header roundtrip");
        for(std::size_t i=0;i<48;++i) rejects([&]{detail::decode(bytes.data(),i);});
        auto corrupt=bytes; corrupt[16]=1; rejects([&]{detail::decode(corrupt.data(),corrupt.size());});
        Frame frame; frame.setColor(.2,.3,.4).pushMatrix().translate(1,2,3).drawBox({0,0,0},{0,0,0,1},{1,2,3}).popMatrix(); Frame::validate(frame.data());
        Frame unbalanced; unbalanced.pushMatrix(); rejects([&]{Frame::validate(unbalanced.data());});
        Frame quaternion; quaternion.drawBox({0,0,0},{0,0,0,0},{1,2,3}); rejects([&]{Frame::validate(quaternion.data());});
        GuiTree gui; WidgetDefinition d; d.text="Angle"; gui.widgets.push_back(d);
        check(detail::decodeGui(detail::encodeGui(gui)).widgets[0].text=="Angle","GUI roundtrip");
        gui.widgets.push_back(d); rejects([&]{detail::decodeGui(detail::encodeGui(gui));});
        check(!detail::validUtf8(std::string("\xc0\xaf",2)) && !detail::validUtf8(std::string("\xed\xa0\x80",3)),"Invalid UTF8");
        PlotDefinition plot; plot.channels=32; PlotPanel panel; panel.series.push_back({}); plot.panels.push_back(panel);
        auto decoded=detail::decodePlot(detail::encodePlot(plot)); check(decoded.channels==32 && decoded.capacity==60000,"32 channel admission");
        plot.capacity=1000000; rejects([&]{detail::decodePlot(detail::encodePlot(plot));});
        PlotDefinition tooMany; tooMany.capacity=1; PlotPanel xyPanel; xyPanel.xy=true;
        for(unsigned i=0;i<40;++i) { SeriesDefinition item;item.id=i+1;item.kind=SeriesKind::xy;item.capacity=100000;xyPanel.series.push_back(item); }
        tooMany.panels.push_back(xyPanel); rejects([&]{detail::decodePlot(detail::encodePlot(tooMany));});
        SampleBatch batch; batch.channels=2; batch.timesNs={100,200}; batch.values={1,2,3,4};
        check(detail::decodeSamples(detail::encodeSamples(batch)).values[3]==4,"Sample roundtrip");
        batch.timesNs[1]=100; rejects([&]{detail::decodeSamples(detail::encodeSamples(batch));});
        XyBatch xy; xy.points={{1,2,false},{3,4,true}}; check(detail::decodeXy(detail::encodeXy(xy)).points[1].breakBefore,"XY gap");
        std::cout<<"Remote protocol tests passed\n"; return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<"\n"; return 1; }
}
