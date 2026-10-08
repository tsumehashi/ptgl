#include <ptgl/Remote/RemotePublisher.h>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <thread>
using namespace ptgl::remote;
int main(int argc,char** argv) {
    unsigned port=9002;double seconds=0;
    for(int i=1;i+1<argc;++i) {
        if(std::string(argv[i])=="--port")port=unsigned(std::strtoul(argv[++i],nullptr,10));
        else if(std::string(argv[i])=="--seconds")seconds=std::strtod(argv[++i],nullptr);
    }
    try {
        RemotePublisher publisher;
        publisher.defineView({1,ViewKind::scene3D,"Remote 3D + GUI"});
        publisher.defineView({2,ViewKind::plot,"Remote 32 channels / 1 kHz"});
        publisher.defineView({3,ViewKind::plot,"Remote XY + trajectory"});
        GuiTree gui;
        WidgetDefinition label;label.id=1;label.text="Network controls";label.width=210;gui.widgets.push_back(label);
        WidgetDefinition pause;pause.id=2;pause.kind=WidgetKind::checkBox;pause.text="Pause source";pause.y=50;pause.width=210;gui.widgets.push_back(pause);
        WidgetDefinition rate;rate.id=3;rate.kind=WidgetKind::slider;rate.text="Speed";rate.y=100;rate.width=210;rate.minimum=1;rate.maximum=8;rate.value=2;gui.widgets.push_back(rate);
        WidgetDefinition hint;hint.id=4;hint.text="Speed";hint.y=80;gui.widgets.push_back(hint);
        for(Id view=1;view<=3;++view) publisher.defineGui(view,gui);
        PlotDefinition plot;plot.channels=32;plot.columns=2;plot.leftMargin=240;plot.title="32 channels at 1 kHz";plot.capacity=60000;
        for(unsigned group=0;group<8;++group) {
            PlotPanel panel;panel.title="Joints "+std::to_string(group*4+1)+"-"+std::to_string(group*4+4);panel.yLabel="deg";panel.yMin=-100;panel.yMax=100;
            for(unsigned j=0;j<4;++j){SeriesDefinition s;s.id=group*4+j+1;s.channel=s.id-1;s.label="J"+std::to_string(s.id);s.color={.12f+.18f*j,.4f,.8f-.18f*j,1};panel.series.push_back(s);}
            plot.panels.push_back(panel);
        }
        if(publisher.definePlot(2,plot)!=PublishResult::Accepted) return 2;
        PlotDefinition xy;xy.capacity=1;xy.leftMargin=240;xy.title="Static XY and live trajectory";
        PlotPanel panel;panel.xy=true;panel.equalAspect=true;panel.xMin=panel.yMin=-1.5;panel.xMax=panel.yMax=1.5;
        SeriesDefinition fixed;fixed.kind=SeriesKind::xy;fixed.label="Reference";fixed.color={.7f,.7f,.7f,1};panel.series.push_back(fixed);
        SeriesDefinition trail;trail.id=2;trail.kind=SeriesKind::trajectory;trail.label="Trajectory";trail.capacity=6000;trail.lineWidth=3;panel.series.push_back(trail);xy.panels.push_back(panel);publisher.definePlot(3,xy);
        XyBatch circle;for(int i=0;i<=180;++i){double t=i*6.28318530718/180;circle.points.push_back({std::cos(t),std::sin(t),false});}publisher.setXyData(3,circle);
        if(!publisher.listen({"0.0.0.0",std::uint16_t(port)})){std::cerr<<publisher.lastError()<<"\n";return 1;}
        std::cout<<"Remote publisher: ws://<this-pc>:"<<publisher.port()<<" (views: 1=3D, 2=time, 3=XY)\n"<<std::flush;
        bool paused=false;double speed=2,phase=0;
        const auto start=std::chrono::steady_clock::now();std::uint64_t tick=0,xyIndex=0;
        while(true) {
            const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
            if(seconds>0 && elapsed>=seconds)break;
            GuiEvent event;while(publisher.pollEvent(event)) {
                if(event.widgetId==2)paused=event.value!=0;
                if(event.widgetId==3)speed=event.value;
                publisher.eventResult(event,true);
                for(Id v=1;v<=3;++v) if(v!=event.viewId) publisher.updateGui(v,event.widgetId,event.value,event.text);
            }
            auto now=std::uint64_t(elapsed*1000);
            if(now>tick) {
                // Bound catch-up work after suspension; skipped indices remain a visible gap.
                if(now-tick>100) tick=now-100;
                SampleBatch b;b.firstIndex=tick;b.channels=32;
                while(tick<now) {
                    if(!paused) {
                        phase+=speed*.001;
                        b.timesNs.push_back(std::int64_t(tick)*1000000);
                        for(unsigned c=0;c<32;++c)b.values.push_back(float(80*std::sin(phase*(1+.02*c)+.2*c)));
                    }
                    ++tick;
                }
                if(!b.timesNs.empty()) publisher.appendSamples(2,b);
                Frame f;f.drawAxis({0,0,0},{0,0,0,1},2).setColor(.15,.48,.85).setMaterial(.3,.04);
                f.drawBox({0,0,1},{0,0,std::sin(phase*.5),std::cos(phase*.5)},{1.2,.5,1.6});
                f.setColor(.85,.35,.15).drawSphere({2*std::cos(phase),2*std::sin(phase),.5},.35);
                publisher.submitFrame(1,1,std::move(f));
                if(!paused){XyBatch bxy;bxy.seriesId=2;bxy.firstIndex=xyIndex++;bxy.points.push_back({std::cos(phase),std::sin(phase*1.5),false});publisher.appendXy(3,bxy);}
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    } catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
