#include "ButtonPanel.h"
#include <algorithm>

namespace ptgl {
namespace gui {

ButtonPanel::ButtonPanel()
    : DockWidget()
{
    setWindowTitle("ButtonPanel");
}

ButtonPanel::ButtonPanel(const std::string& title)
    : DockWidget(title)
{
    setWindowTitle(title);
}

ButtonPanel::~ButtonPanel()
{

}

PushButtonPtr ButtonPanel::addButton(const std::string& name, bool checkable)
{
    PushButtonPtr ptr = findButton(name);
    if (!ptr) {    // not find
        PushButtonPtr button = std::make_shared<PushButton>(name);
        buttons_.push_back(button);
        nameToButtonMap_[name] = button;

        button->setCheckable(checkable);

        addWidget(button);
        ptr = button;
    }
    return ptr;
}

PushButtonPtr ButtonPanel::button(const std::string& name)
{
    return addButton(name);
}

PushButtonPtr ButtonPanel::findButton(const std::string& name) const
{
    auto itr = nameToButtonMap_.find(name);
    return itr != nameToButtonMap_.end() ? itr->second : nullptr;
}

void ButtonPanel::setRadioButtonGroup(const std::vector<std::string>& names)
{
    radioButtonGroupMap_.clear();
    for (auto& name : names) {
        auto fitr = nameToButtonMap_.find(name);
        if (fitr != nameToButtonMap_.end()) {
            radioButtonGroupMap_.insert({name, fitr->second});
        }
    }
}

void ButtonPanel::setRadioButtonPressed(const std::string& name)
{
    auto fitr = radioButtonGroupMap_.find(name);
    if (fitr != radioButtonGroupMap_.end()) {
        auto fptr = fitr->second;
        for (auto itr : radioButtonGroupMap_) {
            auto ptr = itr.second;
            if (ptr == fptr) {
                ptr->setDown(true);
                ptr->setChecked(true);
            } else {
                ptr->setDown(false);
                ptr->setChecked(false);
            }
        }
    }
}

void ButtonPanel::removeButton(const std::string& name)
{
    auto itr = std::find_if(buttons_.begin(), buttons_.end(), [&](PushButtonPtr button){ return name == button->name(); });
    if (itr != buttons_.end()) {
        removeWidget(*itr);
        buttons_.erase(itr);
        nameToButtonMap_.erase(name);
    }
}

void ButtonPanel::clearButtons()
{
    for (auto& ptr : buttons_) {
        removeWidget(ptr);
    }
    buttons_.clear();
    nameToButtonMap_.clear();
}

}
} /* namespace ptgl */
