#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <thread>
#include <vector>
#include "ptgl/Core/QuickGraphicsItem.h"
#include "ptgl/Core/QuickGraphicsView.h"
#include "ptgl/Core/PrimitiveShapeVertex.h"
#include "ptgl/Core/SphericalCamera.h"
#include "ptgl/Driver/GLFWGraphicsDriver.h"
#include "ptgl/Handle/TransformHandle.h"

int main()
{
    ptgl::QuickPlasticGraphicsView view(std::make_unique<ptgl::GLFWGraphicsDriver>());
    view.setWindowTitle("PlasticDemo - Space: style, S: shadows, L: moving light");
    view.setWindowSize(1100, 720);
    view.setFrameRate(60);
    view.setBackgroundColor(0.15, 0.18, 0.23);
    view.setDrawWorldGrid(false);
    auto lighting = view.plasticLighting();
    lighting.keyDirection = {-1, -1, 2};
    view.setPlasticLighting(lighting);
    auto shadows = view.shadowSettings();
    shadows.center = {0, 0, 0.7};
    shadows.halfExtent = 6.0;
    shadows.autoFit = true;
    shadows.softness = 2.5;
    view.setShadowSettings(shadows);

    bool movingLight = false;
    double lightAngle = -2.35619449019;
    view.setPrevProcessFunction([&] {
        if (movingLight) {
            lightAngle += 0.012;
            auto lighting = view.plasticLighting();
            lighting.keyDirection = {1.4 * std::cos(lightAngle), 1.4 * std::sin(lightAngle), 2.0};
            view.setPlasticLighting(lighting);
        }
    });

    auto camera = std::make_shared<ptgl::SphericalCamera>();
    view.setCamera(camera);
    view.setInitProcessFunction([&] {
        camera->setCenter(Eigen::Vector3d(0, 0, 0.7));
        camera->setDistance(10);
        camera->setHeading(65);
        camera->setElevation(25);
    });

    // Generated once; drawVertex reuses the renderer's upload buffer.
    auto roundedBox = ptgl::PrimitiveShapeVertex::generateRoundedBox({1.5, 1.5, 1.5}, 0.18);
    view.setRenderSceneFunction([&](ptgl::Renderer3D* r) {
        const auto material = r->material();
        r->setMaterial({0.85, 0.04});
        r->setColor(0.68, 0.71, 0.76);
        r->drawBox({0, 0, -0.1}, r->R0(), {7.0, 8.0, 0.2});
        r->setMaterial(material);
    });

    struct DemoObject {
        ptgl::GraphicsItemPtr item;
        ptgl::TransformPtr transform;
    };
    std::vector<DemoObject> objects;
    ptgl::GraphicsItemPtr selectedObject;
    constexpr double selectedOpacity = 0.5;
    auto addObject = [&](const std::string& name, const Eigen::Vector3d& position,
                         std::function<void(ptgl::Renderer3D*)> draw) {
        auto transform = std::make_shared<ptgl::Transform>(position, Eigen::Matrix3d::Identity());
        auto item = std::make_shared<ptgl::QuickGraphicsItem>();
        item->setName(name);
        // The same transform is used for drawing, picking and casting shadows.
        item->setRenderSceneFunction([transform, draw](ptgl::Renderer3D* r) {
            r->pushMatrix();
            r->transform(transform->position(), transform->rotation());
            draw(r);
            r->popMatrix();
        });
        objects.push_back({item, transform});
        view.addGraphicsItem(item);
    };
    addObject("Sphere", {0, -2.1, 0.8}, [](ptgl::Renderer3D* r) {
        r->setColor(0.86, 0.24, 0.13);
        r->drawSphere(r->p0(), r->R0(), 0.8);
    });
    addObject("Rounded box", {0, 0, 0.75}, [&](ptgl::Renderer3D* r) {
        r->setColor(0.16, 0.52, 0.83);
        r->drawVertex(r->p0(), r->R0(), roundedBox.vertices, roundedBox.indices);
    });
    addObject("Cylinder", {0, 2.1, 0.8}, [](ptgl::Renderer3D* r) {
        r->setColor(0.95, 0.64, 0.12);
        r->drawCylinder(r->p0(), r->R0(), 1.6, 0.72);
    });

    auto handle = std::make_shared<ptgl::handle::TransformHandle>();
    handle->setEnabled(false);
    view.addGraphicsItem(handle);
    view.setPickingUpEventFunction([&](ptgl::PickingEvent* e) {
        auto picked = e->pickedGraphicsItem();
        // Handle parts are children: clicking one must preserve the selection.
        for (auto item = picked.get(); item; item = item->parentItem()) {
            if (item == handle.get()) return;
        }
        if (selectedObject) selectedObject->setOpacity(1.0);
        for (const auto& object : objects) {
            if (object.item == picked) {
                selectedObject = object.item;
                selectedObject->setOpacity(selectedOpacity);
                handle->setTransform(object.transform);
                handle->setEnabled(true);
                return;
            }
        }
        selectedObject.reset();
        handle->setEnabled(false);
    });

    view.setKeyPressEventFunction([&](ptgl::KeyEvent* e) {
        if (e->keyAction() == ptgl::KeyEvent::KeyAction::KeyRelease) return;
        using Style = ptgl::PlasticGraphicsView::RenderStyle;
        if (e->key() == ptgl::Key::Key_Space && e->keyAction() == ptgl::KeyEvent::KeyAction::KeyPress) {
            view.setRenderStyle(view.renderStyle() == Style::Plastic ? Style::Legacy : Style::Plastic);
        } else if (e->key() == ptgl::Key::Key_S && e->keyAction() == ptgl::KeyEvent::KeyAction::KeyPress) {
            auto settings = view.shadowSettings();
            settings.enabled = !settings.enabled;
            view.setShadowSettings(settings);
        } else if (e->key() == ptgl::Key::Key_L && e->keyAction() == ptgl::KeyEvent::KeyAction::KeyPress) {
            movingLight = !movingLight;
        } else if (e->key() == ptgl::Key::Key_E && e->keyAction() == ptgl::KeyEvent::KeyAction::KeyPress) {
            auto settings = view.environmentSettings();
            settings.enabled = !settings.enabled;
            view.setEnvironmentSettings(settings);
        } else if (e->key() == ptgl::Key::Key_A && e->keyAction() == ptgl::KeyEvent::KeyAction::KeyPress) {
            auto settings = view.ambientOcclusionSettings();
            settings.enabled = !settings.enabled;
            view.setAmbientOcclusionSettings(settings);
        } else if (e->key() == ptgl::Key::Key_LeftBracket || e->key() == ptgl::Key::Key_RightBracket) {
            auto settings = view.shadowSettings();
            settings.softness = std::clamp(settings.softness + (e->key() == ptgl::Key::Key_RightBracket ? 0.5 : -0.5), 0.0, 4.0);
            view.setShadowSettings(settings);
        } else if (e->key() == ptgl::Key::Key_Up || e->key() == ptgl::Key::Key_Down) {
            auto material = view.defaultMaterial();
            material.roughness = std::clamp(material.roughness + (e->key() == ptgl::Key::Key_Up ? 0.05 : -0.05), 0.12, 1.0);
            view.setDefaultMaterial(material);
        } else if (e->key() == ptgl::Key::Key_Escape) {
            view.terminate();
        }
    });
    view.setRenderTextSceneFunction([&](ptgl::TextRenderer* r) {
        using Style = ptgl::PlasticGraphicsView::RenderStyle;
        bool plastic = view.renderStyle() == Style::Plastic && view.plasticRenderingAvailable();
        r->setTextColor(1, 1, 1);
        r->drawText(20, 30, std::string(plastic ? "Plastic" : "Legacy")
            + " | roughness: " + std::to_string(view.defaultMaterial().roughness).substr(0, 4)
            + " | shadows: " + (view.shadowsActive() ? "on" : "off")
            + " | softness: " + std::to_string(view.shadowSettings().softness).substr(0, 3));
        r->drawText(20, 55, "Space: switch style   Up/Down: roughness   Right drag: orbit   Wheel: zoom");
        r->drawText(20, 80, "S: shadows   [ / ]: softness   L: moving light   E: environment reflections   A: ambient occlusion");
        r->drawText(20, 105, "Left click: select   Drag arrows/planes: move   Drag rings: rotate   Click floor/background: deselect");
        r->drawText(20, 130, "Selected: " + (selectedObject ? selectedObject->name() + " (50% opacity)" : std::string("none")));
    });

    view.initialize();
    view.execute();
    while (!view.terminated()) std::this_thread::sleep_for(std::chrono::milliseconds(20));
    return 0;
}
