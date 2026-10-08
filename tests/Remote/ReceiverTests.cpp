#include <ptgl/Remote/RemotePublisher.h>
#include <ptgl/Extension/Remote/RemoteGraphicsView.h>
#include <ptgl/Extension/Remote/RemotePlotGraphicsView.h>
#include <ptgl/Core/GraphicsDriver.h>
#include <ptgl/GUI/PushButton.h>
#include <iostream>
#include <stdexcept>
using namespace ptgl::remote;
void check(bool value,const char* text) { if(!value) throw std::runtime_error(text); }
class Driver final : public ptgl::GraphicsDriver {
    void execute() override {}
    void terminate() override {}
    bool terminated() override { return true; }
    void setWindowSize(int w,int h) override { width_=w;height_=h; }
    void setWindowTitle(const std::string& t) override { title_=t; }
    void setFrameRate(int f) override { fps_=f; }
    const std::string& windowTitle() const override { return title_; }
    int width() const override { return width_; } int height() const override { return height_; } int frameRate() const override { return fps_; }
    std::string title_; int width_=1000,height_=700,fps_=60;
};
class Scene : public ptgl::ext::remote::RemoteGraphicsView {
public: Scene():RemoteGraphicsView(std::make_unique<Driver>(),1) {} using RemoteGraphicsView::executePrevProcess;
};
class Plot : public ptgl::ext::remote::RemotePlotGraphicsView {
public: Plot():RemotePlotGraphicsView(std::make_unique<Driver>(),2) {} using RemotePlotGraphicsView::executePrevProcess;
};
int main() {
    try {
        ptgl::plot::StreamBuffer ring(1,8); float point=1;
        for(int i=0;i<7;++i) check(ring.append(i,&point,1),"Ring append");
        ring.breakBeforeNextSample();
        check(!ring.append(6,&point,1),"Invalid append");
        for(int i=7;i<12;++i) check(ring.append(i,&point,1),"Ring wrap");
        check(ring.sample(0,3).break_before,"Gap survives failed append and wrap");
        std::vector<ptgl::plot::Point> gapPoints;
        for(unsigned pixels:{1u,100u}) {
            ring.select(0,{4,11},pixels,gapPoints);
            check(std::count_if(gapPoints.begin(),gapPoints.end(),[](auto& p){return p.break_before;})==2,"Raw/downsample wrap gap");
        }
        ring.breakBeforeNextSample(); ring.clear(); ring.append(0,&point,1);
        check(!ring.sample(0,0).break_before,"Clear resets pending gap");
        RemotePublisher publisher; publisher.defineView({1,ViewKind::scene3D,"Scene"}); publisher.defineView({2,ViewKind::plot,"Plot"});
        Frame frame; frame.setColor(.1,.2,.3).drawSphere({0,0,0},1); publisher.submitFrame(1,1,std::move(frame));
        GuiTree gui; WidgetDefinition button; button.kind=WidgetKind::checkBox; button.text="Pause"; gui.widgets.push_back(button);
        publisher.defineGui(1,gui); publisher.defineGui(2,gui);
        PlotDefinition d; d.channels=32; d.capacity=128;
        PlotPanel time; for(unsigned i=0;i<32;++i) { SeriesDefinition s; s.id=i+1;s.channel=i;time.series.push_back(s); } d.panels.push_back(time);
        PlotPanel xy; xy.xy=true; SeriesDefinition s; s.id=100;s.kind=SeriesKind::xy;xy.series.push_back(s);s.id=101;s.kind=SeriesKind::trajectory;s.capacity=8;xy.series.push_back(s);d.panels.push_back(xy);
        check(publisher.definePlot(2,d)==PublishResult::Accepted,"Plot definition");
        XyBatch fixed;fixed.seriesId=100;fixed.points={{0,1,false},{1,0,false}};publisher.setXyData(2,fixed);
        check(publisher.listen({"127.0.0.1",0}),"Listen"); auto port=publisher.port();
        Scene scene; Plot plot; auto url="ws://127.0.0.1:"+std::to_string(port);scene.connect(url);plot.connect(url);
        auto await=[&](auto condition) {
            for(int i=0;i<1000;++i) {scene.executePrevProcess();plot.executePrevProcess();if(condition())return;std::this_thread::sleep_for(std::chrono::milliseconds(5));}
            throw std::runtime_error("Timeout: "+scene.receiver().lastError()+" / "+plot.receiver().lastError());
        };
        await([&]{return scene.binding().numLayers()==1 && plot.figure().numAxes()==2;});
        check(plot.figure().axes(1).series(0).xyData().size()==2,"Static XY snapshot");
        SampleBatch b;b.channels=32;
        for(int i=0;i<10;++i) {b.timesNs.push_back(1000000000000ll+i*1000000);for(int c=0;c<32;++c)b.values.push_back(float(i+c));}
        check(publisher.appendSamples(2,b)==PublishResult::Accepted,"Samples");
        await([&]{return plot.figure().data().size()==10;});
        b.firstIndex=20;for(auto& t:b.timesNs)t+=20000000;publisher.appendSamples(2,b);
        await([&]{return plot.figure().data().size()==20;});
        check(plot.figure().data().sample(0,10).break_before,"Transport gap boundary");
        std::vector<ptgl::plot::Point> selected;plot.figure().data().select(0,{0,.04},1,selected);
        check(std::count_if(selected.begin(),selected.end(),[](auto& p){return p.break_before;})>=2,"Downsampled gap");
        XyBatch trail;trail.seriesId=101;trail.points={{0,0,false},{1,1,false}};publisher.appendXy(2,trail);
        await([&]{return plot.figure().axes(1).series(1).xyData().size()==2;});
        trail.firstIndex=4;publisher.appendXy(2,trail);await([&]{return plot.figure().axes(1).series(1).xyData().size()==4;});
        check(plot.figure().axes(1).series(1).xyData().sample(2).break_before,"Trajectory gap");
        plot.figure().setSourcePaused(true); // Must not stop Remote processing.
        auto control=std::dynamic_pointer_cast<ptgl::gui::PushButton>(plot.binding().widget(1));check(bool(control),"GUI created");control->click();
        GuiEvent event;await([&]{return publisher.pollEvent(event);});check(event.widgetId==1 && event.value==1,"GUI return");
        check(publisher.eventResult(event,true)==PublishResult::Accepted,"GUI confirm");
        check(publisher.updateGui(2,1,0)==PublishResult::Accepted,"Sender GUI update");
        await([&]{return !control->isChecked();});
        auto oldEvent=event;auto generation=scene.receiver().generation();
        publisher.stop();await([&]{return !scene.receiver().isConnected();});
        check(!scene.binding().widget(1)->isEnabled(),"Disconnected GUI disabled");
        check(publisher.listen({"127.0.0.1",port}),"Restart");
        await([&]{return scene.receiver().generation()>generation && scene.receiver().isConnected() && scene.binding().widget(1)->isEnabled();});
        check(publisher.eventResult(oldEvent,true)==PublishResult::InvalidArgument,"Old event rejected after reconnect");
        check(scene.binding().numLayers()==1,"Scene restored");
        publisher.clearLayer(1,1);Frame replacement;replacement.drawSphere({1,0,0},2);publisher.submitFrame(1,1,std::move(replacement));
        await([&]{return publisher.statistics().queuedBytes==0;});
        for(int i=0;i<20;++i){scene.executePrevProcess();std::this_thread::sleep_for(std::chrono::milliseconds(5));}
        check(scene.binding().numLayers()==1,"Clear/replace order");
        check(publisher.statistics().rejected==0,"Unexpected server rejection");
        std::cout<<"Remote receiver, GUI, plot, gaps and reconnect tests passed\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<"\n";return 1;}
}
