#include <ptgl/Extension/Remote/RemoteGraphicsView.h>
#include <ptgl/Extension/Remote/RemotePlotGraphicsView.h>
#include <cstdlib>
#include <iostream>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
template<class Base> class Viewer final : public Base {
public:
    Viewer(unsigned id,unsigned frames):Base(id),frames_(frames) {}
protected:
    void postProcess() override {
#ifdef __EMSCRIPTEN__
        if(this->currentFrame()%30==0) {
            auto status=this->receiver().isConnected()?std::string("Connected"):this->receiver().lastError();
            if(status.empty()) status="Connecting";
            EM_ASM({const status=document.getElementById('status');if(status)status.textContent=UTF8ToString($0);},status.c_str());
        }
#endif
        if(frames_ && this->currentFrame()+1>=frames_)this->terminate();
    }
private:unsigned frames_;
};
int main(int argc,char** argv) {
    std::string url="ws://127.0.0.1:9002";unsigned id=1,frames=0;
    for(int i=1;i+1<argc;++i) {
        if(std::string(argv[i])=="--url")url=argv[++i];
        else if(std::string(argv[i])=="--view")id=unsigned(std::strtoul(argv[++i],nullptr,10));
        else if(std::string(argv[i])=="--smoke-frames")frames=unsigned(std::strtoul(argv[++i],nullptr,10));
    }
    try {
        if(id==1) {
            Viewer<ptgl::ext::remote::RemoteGraphicsView> view(id,frames);view.setWindowSize(1280,800);
            if(!view.connect(url))return 1;view.initialize();view.execute();
            if(frames && !view.binding().appliedRevision()){std::cerr<<view.receiver().lastError()<<"\n";return 2;}
        } else {
            Viewer<ptgl::ext::remote::RemotePlotGraphicsView> view(id,frames);view.setWindowSize(1280,800);
            if(!view.connect(url))return 1;view.initialize();view.execute();
            if(frames && !view.binding().appliedRevision()){std::cerr<<view.receiver().lastError()<<"\n";return 2;}
        }
    } catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
