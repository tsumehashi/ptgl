#ifndef PTGL_GUI_SPINSLIDERPANEL_H_
#define PTGL_GUI_SPINSLIDERPANEL_H_

#include <vector>
#include <unordered_map>
#include "DockWidget.h"
#include "SpinSlider.h"

namespace ptgl {
namespace gui {

class SpinSliderPanel;
typedef std::shared_ptr<SpinSliderPanel> SpinSliderPanelPtr;

class SpinSliderPanel : public DockWidget  {
public:
    SpinSliderPanel();
    SpinSliderPanel(const std::string& title);
    virtual ~SpinSliderPanel();

    void setShowText(bool on);
    void setShowTextLength(int length);

    virtual SpinSliderPtr addSlider(const std::string& name);
    virtual SpinSliderPtr addSlider(const std::string& name, int minimum, int maximum, int value);
    SpinSliderPtr slider(const std::string& name);
    SpinSliderPtr findSlider(const std::string& name) const;

    size_t numSliders() const { return sliders_.size(); }
    SpinSliderPtr slider(size_t index) const { return sliders_[index]; }

    const std::vector<SpinSliderPtr>& sliders() const { return sliders_; }

    virtual void removeSlider(const std::string& name);

    virtual void clearSliders();

protected:
    std::vector<SpinSliderPtr> sliders_;
    std::unordered_map<std::string, SpinSliderPtr> nameToSliderMap_;

    bool showText_;
    int showTextLength_;    //0:nolimit
};

}
} /* namespace ptgl */

#endif /* PTGL_GUI_SPINSLIDERPANEL_H_ */
