#ifndef PTGL_GUI_BUTTONPANEL_H_
#define PTGL_GUI_BUTTONPANEL_H_

#include <vector>
#include <unordered_map>
#include "DockWidget.h"
#include "PushButton.h"

namespace ptgl {
namespace gui {

class ButtonPanel;
typedef std::shared_ptr<ButtonPanel> ButtonPanelPtr;

class ButtonPanel : public DockWidget {
public:
    ButtonPanel();
    ButtonPanel(const std::string& title);
    virtual ~ButtonPanel();

    virtual PushButtonPtr addButton(const std::string& name, bool checkable = true);
    PushButtonPtr button(const std::string& name);
    PushButtonPtr findButton(const std::string& name) const;

    size_t numButtons() const { return buttons_.size(); }
    PushButtonPtr button(size_t index) const { return buttons_[index]; }

    const std::vector<PushButtonPtr>& buttons() const { return buttons_; }

    void setRadioButtonGroup(const std::vector<std::string>& names);
    const std::unordered_map<std::string, PushButtonPtr>& getRadioButtonGroupMap() const { return radioButtonGroupMap_; }
    void setRadioButtonPressed(const std::string& name);

    virtual void removeButton(const std::string& name);

    virtual void clearButtons();

protected:
    std::vector<PushButtonPtr> buttons_;
    std::unordered_map<std::string, PushButtonPtr> nameToButtonMap_;
    std::unordered_map<std::string, PushButtonPtr> radioButtonGroupMap_;
};

}
} /* namespace ptgl */

#endif /* PTGL_GUI_BUTTONPANEL_H_ */
