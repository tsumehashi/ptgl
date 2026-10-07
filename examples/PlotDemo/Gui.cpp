#include "ptgl/Plot/PlotGraphicsView.h"
#include "ptgl/GUI/Layout.h"
#include "ptgl/GUI/PushButton.h"
#include "ptgl/GUI/Slider.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <stdexcept>

class JointView final : public ptgl::plot::PlotGraphicsView {
public:
    explicit JointView(unsigned smokeFrames) : PlotGraphicsView(2,60000), smokeFrames_(smokeFrames) {
        setWindowTitle("ptgl Plot + GUI");
        setPlotMargins(240,0,0,0);
        figure().setTitle("Joint angles").setLineWidth(3);
        auto& axes = figure().addAxes("Arm", "deg", {-100,100});
        axes.addSeries(0,"Shoulder"); axes.addSeries(1,"Elbow");
        figure().setLiveZoomEnabled(true);
        panel_ = std::make_shared<ptgl::gui::Panel>("Plot controls");
        auto pause = std::make_shared<ptgl::gui::PushButton>("Pause data");
        pauseButton_ = pause;
        pause->setCheckable(true);
        pause->setOnToggledFunction([this](bool on) { figure().setSourcePaused(on); });
        panel_->add(pause);
        auto follow = std::make_shared<ptgl::gui::PushButton>("Resume following");
        follow->setOnClickedFunction([this](bool) { figure().setFollowLatest(); });
        panel_->add(follow);
        auto visible = std::make_shared<ptgl::gui::PushButton>("Show shoulder");
        visibleButton_ = visible;
        visible->setCheckable(true); visible->setCheckedSilently(true);
        visible->setOnToggledFunction([this](bool on) { figure().axes(0).series(0).setVisible(on); });
        panel_->add(visible);
        auto width = std::make_shared<ptgl::gui::Slider>("Line width");
        width->setRange(1,8).setValue(3);
        width->setOnValueChangedFunction([this](int value) { figure().setLineWidth(float(value)); });
        panel_->add(std::make_shared<ptgl::gui::Label>("Line width"));
        panel_->add(width);
        panel_->add(std::make_shared<ptgl::gui::Label>("Wheel: zoom / Shift: Y"));
        addWidget(panel_);
        setUpdateFunction([tick = std::uint64_t{0}, remainder = 0.0](ptgl::plot::Figure& f, double elapsed) mutable {
            remainder += std::min(elapsed,.25)*1000;
            auto count = std::uint64_t(remainder); remainder -= double(count);
            while (count--) {
                double t = double(tick++)*.001;
                f.append(t, std::array<float,2>{float(80*std::sin(t)),float(50*std::cos(2*t))});
            }
        });
    }
    // Required when subclass fields/callbacks can be used by a background driver.
    ~JointView() override { terminate(); waitUntilStopped(); }
protected:
    void prevProcess() override {
        int scaleWidth = int(230*pixelRatio());
        panel_->setPos(0,0);
        panel_->setSize(scaleWidth,height());
        pauseButton_->setCheckedSilently(figure().isSourcePaused());
        visibleButton_->setCheckedSilently(figure().axes(0).series(0).isVisible());
    }
    void postProcess() override {
        if (smokeFrames_ && currentFrame()+1 >= smokeFrames_) terminate();
    }
private:
    std::shared_ptr<ptgl::gui::Panel> panel_;
    std::shared_ptr<ptgl::gui::PushButton> pauseButton_, visibleButton_;
    unsigned smokeFrames_;
};
int main(int argc, char** argv) {
    unsigned frames = 0;
    for (int i = 1; i+1 < argc; ++i)
        if (std::strcmp(argv[i],"--smoke-frames") == 0) frames = unsigned(std::strtoul(argv[i+1],nullptr,10));
    JointView view(frames);
    view.initialize();
    view.execute();
}
