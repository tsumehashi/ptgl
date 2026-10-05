#include <algorithm>
#include <chrono>
#include <thread>
#include "ptgl/Core/QuickGraphicsView.h"
#include "ptgl/Core/PrimitiveShapeVertex.h"
#include "ptgl/Core/SphericalCamera.h"
#include "ptgl/Driver/GLFWGraphicsDriver.h"

int main()
{
    ptgl::QuickPlasticGraphicsView view(std::make_unique<ptgl::GLFWGraphicsDriver>());
    view.setWindowTitle("PlasticDemo - Space: legacy/plastic, Up/Down: roughness");
    view.setWindowSize(1100, 720);
    view.setFrameRate(60);
    view.setBackgroundColor(0.15, 0.18, 0.23);
    view.setDrawWorldGrid(false);

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
        r->drawBox({0, 0, -0.12}, r->R0(), {7.0, 8.0, 0.2});
        r->setMaterial(material);
        r->setColor(0.86, 0.24, 0.13);
        r->drawSphere({0, -2.1, 0.8}, r->R0(), 0.8);
        r->setColor(0.16, 0.52, 0.83);
        r->drawVertex({0, 0, 0.75}, r->R0(), roundedBox.vertices, roundedBox.indices);
        r->setColor(0.95, 0.64, 0.12);
        r->drawCylinder({0, 2.1, 0.8}, r->R0(), 1.6, 0.72);
    });

    view.setKeyPressEventFunction([&](ptgl::KeyEvent* e) {
        if (e->keyAction() == ptgl::KeyEvent::KeyAction::KeyRelease) return;
        using Style = ptgl::PlasticGraphicsView::RenderStyle;
        if (e->key() == ptgl::Key::Key_Space && e->keyAction() == ptgl::KeyEvent::KeyAction::KeyPress) {
            view.setRenderStyle(view.renderStyle() == Style::Plastic ? Style::Legacy : Style::Plastic);
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
            + " | roughness: " + std::to_string(view.defaultMaterial().roughness).substr(0, 4));
        r->drawText(20, 55, "Space: switch style   Up/Down: roughness   Right drag: orbit   Wheel: zoom");
    });

    view.initialize();
    view.execute();
    while (!view.terminated()) std::this_thread::sleep_for(std::chrono::milliseconds(20));
    return 0;
}
