#include "SceneEditorPanel.h"
#include "ptgl/Core/GraphicsView.h"
#include "ptgl/Core/StyledGraphicsView.h"
#include <type_traits>
#include <sstream>
namespace ptgl::gui
{
namespace
{
const std::vector<std::string> names{"Box",         "Sphere",           "Cylinder",    "Cone",
                                     "Rounded box", "Rounded cylinder", "Rounded cone"};
PrimitiveShape defaultShape(int i)
{
    switch (i) {
    case 0:
        return BoxShape{};
    case 1:
        return SphereShape{};
    case 2:
        return CylinderShape{};
    case 3:
        return ConeShape{};
    case 4:
        return RoundedBoxShape{};
    case 5:
        return RoundedCylinderShape{};
    default:
        return RoundedConeShape{};
    }
}
} // namespace
SceneEditorPanel::SceneEditorPanel(GraphicsView &view) : Panel("Scene editor"), view_(view)
{
    setSize(340, 680);
    pages_ = std::make_shared<ComboBox>();
    pages_->setItems({"Objects", "Properties", "Rendering"});
    pages_->setOnCurrentIndexChangedFunction([this](int) { rebuild_ = true; });
    add(pages_);
    scroll_ = std::make_shared<ScrollArea>();
    add(scroll_, 1);
    errorLabel_ = std::make_shared<Label>();
    errorLabel_->setPreferredSize(300, 24);
    add(errorLabel_);
    refresh();
}
void SceneEditorPanel::setPage(int p)
{
    pages_->setCurrentIndex(p);
    rebuild_ = true;
}
void SceneEditorPanel::attempt(const std::function<void()> &action)
{
    try {
        action();
        error_.clear();
    } catch (const std::exception &e) {
        error_ = e.what();
    }
    errorLabel_->setText(error_);
    errorLabel_->setToolTipText(error_);
    errorLabel_->setEnableToolTip(true);
    errorLabel_->setPickable(true);
}
void SceneEditorPanel::number(const std::string &name, std::function<double()> get,
                              std::function<void(double)> set, double min, double max, double step,
                              int decimals)
{
    auto control = std::make_shared<DoubleSpinBox>();
    control->setRange(min, max);
    control->setStep(step);
    control->setDecimals(decimals);
    control->setValue(get(), false);
    control->setOnValueChangedFunction(
        [this, get, set, weak = std::weak_ptr<DoubleSpinBox>(control)](double value) {
            attempt([&] { set(value); });
            if (auto c = weak.lock())
                c->setValue(get(), false);
        });
    form_->addRow(name, control);
    bindings_.push_back({control, std::move(get)});
}
void SceneEditorPanel::flag(const std::string &name, std::function<bool()> get, std::function<void(bool)> set)
{
    auto control = std::make_shared<CheckBox>(name);
    control->setChecked(get());
    control->setOnToggledFunction([this, set](bool on) { attempt([&] { set(on); }); });
    form_->add(control);
    sync_.push_back([control, get] {
        bool on = get();
        if (control->isChecked() != on) {
            control->setCheckedSilently(on);
        }
    });
}
void SceneEditorPanel::refresh()
{
    auto &scene = view_.objectScene();
    auto selected = scene.selectedObject();
    size_t shape = selected && selected->shape() ? selected->shape()->index() : size_t(-1);
    if (objects_ != scene.objects() || selected_ != selected || shapeIndex_ != shape)
        rebuild_ = true;
    if (rebuild_) {
        objects_ = scene.objects();
        selected_ = selected;
        shapeIndex_ = shape;
        rebuild();
        rebuild_ = false;
    }
    for (auto &b : bindings_)
        if (!b.control->isEditing())
            b.control->setValue(b.get(), false);
    for (auto &sync : sync_)
        sync();
}
void SceneEditorPanel::rebuild()
{
    bindings_.clear();
    sync_.clear();
    form_ = std::make_shared<FormLayout>();
    form_->setPadding(0);
    scroll_->setContent(form_);
    switch (pages_->currentIndex()) {
    case 0:
        buildObjects();
        break;
    case 1:
        buildProperties();
        break;
    default:
        buildRendering();
        break;
    }
}
void SceneEditorPanel::buildObjects()
{
    auto kind = std::make_shared<ComboBox>();
    kind->setItems(names);
    form_->addRow("New shape", kind);
    auto addButton = std::make_shared<PushButton>("Add object");
    addButton->setOnClickedFunction([this, kind](bool) {
        attempt([&] {
            if (addObject_)
                addObject_(kind->currentIndex());
            else {
                auto &scene = view_.objectScene();
                size_t n = scene.objects().size();
                auto object =
                    scene.addPrimitive(names[kind->currentIndex()], defaultShape(kind->currentIndex()),
                                       {double(n % 3) * 1.5, double(n / 3) * 1.5, .5});
                object->setColor(.3, .6, .85);
                scene.selectObject(object);
            }
            rebuild_ = true;
        });
    });
    form_->add(addButton);
    auto path = std::make_shared<TextEditWidget>();
    form_->addRow("STL / OBJ", path);
    auto load = std::make_shared<PushButton>("Load mesh path");
    load->setOnClickedFunction([this, path](bool) {
        attempt([&] {
            auto &scene = view_.objectScene();
            scene.selectObject(scene.loadMesh(path->text(), path->text()));
            rebuild_ = true;
        });
    });
    form_->add(load);
    for (auto object : objects_) {
        auto row = std::make_shared<Layout>(Layout::Direction::Horizontal);
        row->setPadding(0);
        auto visible = std::make_shared<CheckBox>();
        visible->setPreferredSize(20, 28);
        visible->setChecked(object->isVisible());
        visible->setOnToggledFunction([object](bool on) { object->setVisible(on); });
        row->add(visible);
        auto select = std::make_shared<PushButton>(object->name());
        select->setCheckable(true);
        select->setChecked(object == selected_);
        select->setPreferredSize(170, 28);
        select->setOnClickedFunction([this, object](bool) {
            attempt([&] {
                if (!object->isVisible())
                    object->setVisible(true);
                view_.objectScene().selectObject(object);
                setPage(1);
            });
        });
        row->add(select, 1);
        auto remove = std::make_shared<PushButton>("x");
        remove->setPreferredSize(26, 28);
        remove->setOnClickedFunction([this, object](bool) {
            view_.objectScene().removeObject(object);
            rebuild_ = true;
        });
        row->add(remove);
        form_->add(row);
        sync_.push_back([object, select, visible] {
            select->setText(object->name());
            visible->setCheckedSilently(object->isVisible());
        });
    }
}
void SceneEditorPanel::buildProperties()
{
    auto object = selected_;
    if (!object) {
        form_->add(std::make_shared<Label>("Select an object in the scene."));
        return;
    }
    auto name = std::make_shared<TextEditWidget>();
    name->setText(object->name());
    name->setOnTextChangedFunction(
        [object](const std::string &s, const std::string &) { object->setName(s); });
    form_->addRow("Name", name);
    sync_.push_back([object, name] { name->setText(object->name()); });
    for (int i = 0; i < 3; ++i)
        number(
            std::string("Position ") + "XYZ"[i], [object, i] { return object->transform()->position()[i]; },
            [object, i](double v) {
                auto p = object->transform()->position();
                p[i] = v;
                object->transform()->setPosition(p);
            });
    for (int i = 0; i < 3; ++i)
        number(
            std::string("Rotation ") + "XYZ"[i],
            [object, i] {
                return object->transform()->rotation().eulerAngles(0, 1, 2)[i] * 180 / 3.141592653589793;
            },
            [object, i](double value) {
                auto a = object->transform()->rotation().eulerAngles(0, 1, 2);
                a[i] = value * 3.141592653589793 / 180;
                object->transform()->setRotation((Eigen::AngleAxisd(a[0], Eigen::Vector3d::UnitX()) *
                                                  Eigen::AngleAxisd(a[1], Eigen::Vector3d::UnitY()) *
                                                  Eigen::AngleAxisd(a[2], Eigen::Vector3d::UnitZ()))
                                                     .toRotationMatrix());
            },
            -360, 360, 1, 1);
    auto color = std::make_shared<ColorPicker>();
    color->setColor(object->color(), false);
    color->setOnColorChangedFunction(
        [object](const std::array<double, 3> &c) { object->setColor(c[0], c[1], c[2]); });
    form_->addRow("Color", color);
    sync_.push_back([object, color] { color->setColor(object->color(), false); });
    number(
        "Opacity", [this, object] { return view_.objectScene().objectOpacity(object); },
        [this, object](double v) { view_.objectScene().setObjectOpacity(object, v); }, 0, 1, .05, 2);
    auto material = [this, object] {
        auto styled = dynamic_cast<StyledGraphicsView *>(&view_);
        return object->material().value_or(styled ? styled->defaultMaterial() : Material{});
    };
    flag(
        "Own material", [object] { return bool(object->material()); },
        [object, material](bool on) {
            if (on)
                object->setMaterial(material());
            else
                object->clearMaterial();
        });
    number(
        "Roughness", [material] { return material().roughness; },
        [object, material](double v) {
            auto m = material();
            m.roughness = v;
            object->setMaterial(m);
        },
        .12, 1, .02, 2);
    number(
        "Reflectance", [material] { return material().specularReflectance; },
        [object, material](double v) {
            auto m = material();
            m.specularReflectance = v;
            object->setMaterial(m);
        },
        0, 1, .01, 2);
    if (object->shape()) {
        auto kind = std::make_shared<ComboBox>();
        kind->setItems(names);
        kind->setCurrentIndex(int(object->shape()->index()), false);
        kind->setOnCurrentIndexChangedFunction([this, object](int index) {
            attempt([&] {
                object->setShape(defaultShape(index));
                rebuild_ = true;
            });
        });
        form_->addRow("Shape", kind);
        std::visit(
            [&](const auto &original) {
                using T = std::decay_t<decltype(original)>;
                auto parameter = [&](const std::string &caption, auto get, auto set, double min,
                                     double max = 1e4) {
                    number(
                        caption, [object, get] { return get(std::get<T>(*object->shape())); },
                        [object, set](double v) {
                            auto s = std::get<T>(*object->shape());
                            set(s, v);
                            object->setShape(s);
                        },
                        min, max, .05, 3);
                };
                if constexpr (std::is_same_v<T, BoxShape> || std::is_same_v<T, RoundedBoxShape>)
                    for (int i = 0; i < 3; ++i)
                        parameter(
                            std::string("Size ") + "XYZ"[i], [i](const T &s) { return s.sides[i]; },
                            [i](T &s, double v) { s.sides[i] = v; }, .001);
                if constexpr (!std::is_same_v<T, BoxShape>)
                    parameter(
                        "Radius", [](const T &s) { return s.radius; }, [](T &s, double v) { s.radius = v; },
                        std::is_same_v<T, RoundedBoxShape> ? 0 : .001);
                if constexpr (std::is_same_v<T, CylinderShape> || std::is_same_v<T, ConeShape> ||
                              std::is_same_v<T, RoundedCylinderShape> || std::is_same_v<T, RoundedConeShape>)
                    parameter(
                        "Length", [](const T &s) { return s.length; }, [](T &s, double v) { s.length = v; },
                        .001);
                if constexpr (std::is_same_v<T, RoundedCylinderShape> || std::is_same_v<T, RoundedConeShape>)
                    parameter(
                        "Fillet", [](const T &s) { return s.filletRadius; },
                        [](T &s, double v) { s.filletRadius = v; }, 0);
            },
            *object->shape());
    } else
        form_->add(std::make_shared<Label>("Mesh geometry (STL / OBJ / custom)"));
    auto status = std::make_shared<Label>();
    form_->add(status);
    sync_.push_back([object, status] { status->setText(sectionStatusMessage(object->sectionStatus())); });
    auto remove = std::make_shared<PushButton>("Delete object");
    remove->setOnClickedFunction([this, object](bool) {
        view_.objectScene().removeObject(object);
        rebuild_ = true;
    });
    form_->add(remove);
}
void SceneEditorPanel::buildRendering()
{
    auto view = dynamic_cast<StyledGraphicsView *>(&view_);
    if (!view) {
        form_->add(std::make_shared<Label>("Requires StyledGraphicsView"));
        return;
    }
    auto style = std::make_shared<ComboBox>();
    style->setItems({"Legacy", "Plastic", "CAD"});
    style->setCurrentIndex(int(view->renderStyle()), false);
    style->setOnCurrentIndexChangedFunction([view](int i) { view->setRenderStyle(RenderStyle(i)); });
    form_->addRow("Style", style);
    sync_.push_back([view, style] { style->setCurrentIndex(int(view->renderStyle()), false); });
    flag(
        "Edges", [view] { return view->edgeSettings().enabled; },
        [view](bool on) {
            auto s = view->edgeSettings();
            s.enabled = on;
            view->setEdgeSettings(s);
        });
    number(
        "Edge width", [view] { return view->edgeSettings().width; },
        [view](double v) {
            auto s = view->edgeSettings();
            s.width = v;
            view->setEdgeSettings(s);
        },
        .5, 4, .25, 2);
    flag(
        "Shadows", [view] { return view->shadowSettings().enabled; },
        [view](bool on) {
            auto s = view->shadowSettings();
            s.enabled = on;
            view->setShadowSettings(s);
        });
    number(
        "Softness", [view] { return view->shadowSettings().softness; },
        [view](double v) {
            auto s = view->shadowSettings();
            s.softness = v;
            view->setShadowSettings(s);
        },
        0, 4, .25, 2);
    number(
        "Brightness", [view] { return view->cadSettings().brightness; },
        [view](double v) {
            auto s = view->cadSettings();
            s.brightness = v;
            view->setCadSettings(s);
        },
        0, 4, .1, 2);
    flag(
        "Environment", [view] { return view->environmentSettings().enabled; },
        [view](bool on) {
            auto s = view->environmentSettings();
            s.enabled = on;
            view->setEnvironmentSettings(s);
        });
    flag(
        "Ambient occlusion", [view] { return view->ambientOcclusionSettings().enabled; },
        [view](bool on) {
            auto s = view->ambientOcclusionSettings();
            s.enabled = on;
            view->setAmbientOcclusionSettings(s);
        });
    flag(
        "Section plane", [view] { return view->sectionSettings().enabled; },
        [view](bool on) {
            auto s = view->sectionSettings();
            s.enabled = on;
            view->setSectionSettings(s);
        });
    for (int i = 0; i < 3; ++i)
        number(
            std::string("Plane ") + "XYZ"[i], [view, i] { return view->sectionSettings().point[i]; },
            [view, i](double v) {
                auto s = view->sectionSettings();
                s.point[i] = v;
                view->setSectionSettings(s);
            });
    for (int i = 0; i < 3; ++i)
        number(
            std::string("Normal ") + "XYZ"[i], [view, i] { return view->sectionSettings().normal[i]; },
            [view, i](double v) {
                auto s = view->sectionSettings();
                s.normal[i] = v;
                view->setSectionSettings(s);
            },
            -1, 1, .1, 3);
    flag(
        "Fill section", [view] { return view->sectionSettings().capEnabled; },
        [view](bool on) {
            auto s = view->sectionSettings();
            s.capEnabled = on;
            view->setSectionSettings(s);
        });
    flag(
        "Keep positive side", [view] { return view->sectionSettings().keepPositiveSide; },
        [view](bool on) {
            auto s = view->sectionSettings();
            s.keepPositiveSide = on;
            view->setSectionSettings(s);
        });
    auto cap = std::make_shared<ColorPicker>();
    cap->setColor(view->sectionSettings().capColor, false);
    cap->setOnColorChangedFunction([view](const std::array<double, 3> &c) {
        auto s = view->sectionSettings();
        s.capColor = c;
        view->setSectionSettings(s);
    });
    form_->addRow("Cap color", cap);
    sync_.push_back([view, cap] { cap->setColor(view->sectionSettings().capColor, false); });
    auto themePicker = std::make_shared<ComboBox>();
    themePicker->setItems({"Classic", "Dark", "Light"});
    themePicker->setCurrentIndex(int(theme().preset), false);
    themePicker->setOnCurrentIndexChangedFunction([this](int i) {
        auto t = i == 1 ? Theme::dark() : i == 2 ? Theme::light() : Theme{};
        t.scale = theme().scale;
        setTheme(t);
    });
    form_->addRow("GUI theme", themePicker);
    sync_.push_back(
        [this, themePicker] { themePicker->setCurrentIndex(int(theme().preset), false); });
    number(
        "GUI scale", [this] { return theme().scale; },
        [this](double v) {
            auto t = theme();
            t.scale = v;
            setTheme(t);
        },
        .75, 2, .25, 2);
}
} // namespace ptgl::gui
