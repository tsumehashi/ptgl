#ifndef PTGL_GUI_SCENEEDITORPANEL_H_
#define PTGL_GUI_SCENEEDITORPANEL_H_
#include "Controls.h"
#include "ptgl/Core/ObjectScene.h"
namespace ptgl
{
class StyledGraphicsView;
}
namespace ptgl::gui
{
// Reusable editor. The referenced view must outlive the panel. Use on the view thread.
class SceneEditorPanel : public Panel
{
  public:
    explicit SceneEditorPanel(GraphicsView &view);
    void setPage(int page);
    void refresh();
    const std::string &lastError() const { return error_; }
    void setOnAddObjectFunction(std::function<void(int)> f) { addObject_ = std::move(f); }

  protected:
    void prevProcess() override { refresh(); }

  private:
    struct Binding {
        std::shared_ptr<DoubleSpinBox> control;
        std::function<double()> get;
    };
    void rebuild();
    void buildObjects();
    void buildProperties();
    void buildRendering();
    void attempt(const std::function<void()> &action);
    void number(const std::string &name, std::function<double()> get, std::function<void(double)> set,
                double min = -1e6, double max = 1e6, double step = .1, int decimals = 3);
    void flag(const std::string &name, std::function<bool()> get, std::function<void(bool)> set);
    GraphicsView &view_;
    std::shared_ptr<ComboBox> pages_;
    std::shared_ptr<ScrollArea> scroll_;
    std::shared_ptr<FormLayout> form_;
    std::shared_ptr<Label> errorLabel_;
    std::vector<Binding> bindings_;
    std::vector<std::function<void()>> sync_;
    std::vector<SceneObjectPtr> objects_;
    SceneObjectPtr selected_;
    size_t shapeIndex_ = size_t(-1);
    bool rebuild_ = true;
    std::string error_;
    std::function<void(int)> addObject_;
};
} // namespace ptgl::gui
#endif
