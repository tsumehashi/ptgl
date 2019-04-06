#include "SpinSliderPanel.h"
#include <algorithm>

namespace ptgl {
namespace gui {

SpinSliderPanel::SpinSliderPanel()
    : DockWidget()
{
    setWindowTitle("SpinSliderPanel");

    showText_ = true;
    showTextLength_ = 7;
}

SpinSliderPanel::SpinSliderPanel(const std::string& title)
    : DockWidget(title)
{
    setWindowTitle(title);

    showText_ = true;
    showTextLength_ = 7;
}

SpinSliderPanel::~SpinSliderPanel()
{

}

void SpinSliderPanel::setShowText(bool on)
{
    showText_ = on;
    for (auto& item : sliders_) {
        item->setShowText(showText_);
    }
}

void SpinSliderPanel::setShowTextLength(int length)
{
    showTextLength_ = length;
    for (auto& item : sliders_) {
        item->setShowTextLength(showTextLength_);
    }
}

SpinSliderPtr SpinSliderPanel::addSlider(const std::string& name)
{
    SpinSliderPtr ptr = findSlider(name);
    if (!ptr) {    // not find
        SpinSliderPtr slider = std::make_shared<SpinSlider>(name);
        sliders_.push_back(slider);
        nameToSliderMap_[name] = slider;

        slider->setShowText(showText_);
        slider->setShowTextLength(showTextLength_);

        addWidget(slider);
        ptr = slider;
    }
    return ptr;
}

SpinSliderPtr SpinSliderPanel::addSlider(const std::string& name, int minimum, int maximum, int value)
{
    auto slider = addSlider(name);
    slider->setRange(minimum, maximum);
    slider->setValue(value);
    return slider;
}

SpinSliderPtr SpinSliderPanel::slider(const std::string& name)
{
    return findSlider(name);
}

SpinSliderPtr SpinSliderPanel::findSlider(const std::string& name) const
{
    auto itr = nameToSliderMap_.find(name);
    return itr != nameToSliderMap_.end() ? itr->second : nullptr;
}

void SpinSliderPanel::removeSlider(const std::string& name)
{
    auto itr = std::find_if(sliders_.begin(), sliders_.end(), [&](SpinSliderPtr slider){ return name == slider->name(); });
    if (itr != sliders_.end()) {
        removeWidget(*itr);
        sliders_.erase(itr);
        nameToSliderMap_.erase(name);
    }
}

void SpinSliderPanel::clearSliders()
{
    for (auto& ptr : sliders_) {
        removeWidget(ptr);
    }
    sliders_.clear();
    nameToSliderMap_.clear();
}

}
} /* namespace ptgl */
