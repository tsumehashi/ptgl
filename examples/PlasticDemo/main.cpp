#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <functional>
#include <filesystem>
#include <thread>
#include <vector>
#include "ptgl/Core/ObjectScene.h"
#include "ptgl/Core/QuickGraphicsView.h"
#include "ptgl/Core/SphericalCamera.h"
#include "ptgl/Driver/GLFWGraphicsDriver.h"

int main()
{
    ptgl::QuickStyledGraphicsView view(std::make_unique<ptgl::GLFWGraphicsDriver>());
    view.setWindowTitle("PlasticDemo - Space: Plastic / CAD / Legacy");
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

    auto quality = view.renderQualitySettings();
    quality.cacheStaticEdges = true;
    quality.collectTimings = true;
    view.setRenderQualitySettings(quality);
    bool showStatistics = false;

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

    view.setRenderSceneFunction([&](ptgl::Renderer3D* r) {
        const auto material = r->material();
        r->setMaterial({0.85, 0.04});
        r->setColor(0.68, 0.71, 0.76);
        r->drawBox({0, 0, -0.1}, r->R0(), {7.0, 8.0, 0.2});
        r->setMaterial(material);
    });

    auto& scene = view.objectScene();
    auto section = view.sectionSettings();
    section.point = {0, 0, 0.8};
    view.setSectionSettings(section);
    std::string meshMessage;
    const auto& objects = scene.objects();
    const auto& selectedObject = scene.selectedObject();
    const std::array<std::string, 6> shapeNames{{"Sphere", "Rounded box", "Cylinder", "Cube", "Rounded cylinder", "Rounded cone"}};
    constexpr std::array<double, 6> shapeHeights{{0.8, 0.75, 0.8, 0.75, 0.8, 0.9}};
    const std::array<ptgl::PrimitiveShape, 6> shapes{{
        ptgl::SphereShape{0.8}, ptgl::RoundedBoxShape{{1.5, 1.5, 1.5}, 0.18},
        ptgl::CylinderShape{1.6, 0.72}, ptgl::BoxShape{{1.5, 1.5, 1.5}},
        ptgl::RoundedCylinderShape{1.6, 0.72, 0.18}, ptgl::RoundedConeShape{1.8, 0.82, 0.14}
    }};
    const std::array<std::array<double, 3>, 6> colors{{
        {{0.86, 0.24, 0.13}}, {{0.16, 0.52, 0.83}}, {{0.95, 0.64, 0.12}},
        {{0.30, 0.68, 0.37}}, {{0.62, 0.38, 0.84}}, {{0.25, 0.72, 0.72}}
    }};
    auto createShape = [&](size_t shape, const std::string& name, const Eigen::Vector3d& position) {
        auto object = scene.addPrimitive(name, shapes[shape], position);
        object->setColor(colors[shape][0], colors[shape][1], colors[shape][2]);
        return object;
    };
    for (size_t shape = 0; shape < 3; ++shape)
        createShape(shape, shapeNames[shape], {0, (double(shape) - 1) * 2.1, shapeHeights[shape]});
    std::array<size_t, 6> shapeSerials{{1, 1, 1, 0, 0, 0}};

    auto addShape = [&](size_t shape) {
        // Search outward for a free grid position, reusing gaps left by deletions.
        // Existing transforms are checked because objects may have been dragged.
        for (int ring = 0; ; ++ring) {
            for (int x = ring; x >= -ring; --x) for (int y = -ring; y <= ring; ++y) {
                if (std::max(std::abs(x), std::abs(y)) != ring) continue;
                Eigen::Vector3d position(x * 2.1, y * 2.1, shapeHeights[shape]);
                bool occupied = std::any_of(objects.begin(), objects.end(), [&](const ptgl::SceneObjectPtr& object) {
                    return (object->transform()->position() - position).head<2>().norm() < 2.0;
                });
                if (occupied) continue;
                auto object = createShape(shape, shapeNames[shape] + " " + std::to_string(++shapeSerials[shape]), position);
                scene.selectObject(object);
                return;
            }
        }
    };
    view.setDropEventFunction([&](ptgl::DropEvent* e) {
        for (const auto& path : e->dropPaths()) {
            try {
                auto object = scene.loadMesh(std::filesystem::u8path(path).filename().u8string(), path);
                object->setColor(0.68, 0.73, 0.80);
                scene.selectObject(object);
                const auto bounds = object->worldBounds();
                camera->setCenter(bounds.center());
                camera->setDistance(std::max(3.0, bounds.sizes().norm() * 1.8));
                auto settings = view.sectionSettings();
                settings.point = {{bounds.center().x(), bounds.center().y(), bounds.center().z()}};
                view.setSectionSettings(settings);
                meshMessage = "Loaded: " + object->name();
            } catch (const std::exception& error) { meshMessage = error.what(); }
        }
    });
    view.setKeyPressEventFunction([&](ptgl::KeyEvent* e) {
        if (e->keyAction() == ptgl::KeyEvent::KeyAction::KeyRelease) return;
        using Style = ptgl::StyledGraphicsView::RenderStyle;
        const std::array<ptgl::Key, 6> addKeys{{ptgl::Key::Key_1, ptgl::Key::Key_2, ptgl::Key::Key_3,
                                              ptgl::Key::Key_4, ptgl::Key::Key_5, ptgl::Key::Key_6}};
        auto addKey = std::find(addKeys.begin(), addKeys.end(), e->key());
        if (addKey != addKeys.end()) {
            if (e->keyAction() == ptgl::KeyEvent::KeyAction::KeyPress)
                addShape(static_cast<size_t>(std::distance(addKeys.begin(), addKey)));
        } else if (e->key() == ptgl::Key::Key_Delete) {
            if (e->keyAction() == ptgl::KeyEvent::KeyAction::KeyPress) scene.removeSelectedObject();
        } else if (e->key() == ptgl::Key::Key_X || e->key() == ptgl::Key::Key_F || e->key() == ptgl::Key::Key_K) {
            if (e->keyAction() == ptgl::KeyEvent::KeyAction::KeyPress) {
                auto settings = view.sectionSettings();
                if (e->key() == ptgl::Key::Key_X) settings.enabled = !settings.enabled;
                if (e->key() == ptgl::Key::Key_F) settings.capEnabled = !settings.capEnabled;
                if (e->key() == ptgl::Key::Key_K) settings.keepPositiveSide = !settings.keepPositiveSide;
                view.setSectionSettings(settings);
            }
        } else if (e->key() == ptgl::Key::Key_PageUp || e->key() == ptgl::Key::Key_PageDown) {
            auto settings = view.sectionSettings();
            settings.point[2] += e->key() == ptgl::Key::Key_PageUp ? 0.1 : -0.1;
            view.setSectionSettings(settings);
        } else if (e->key() == ptgl::Key::Key_Space && e->keyAction() == ptgl::KeyEvent::KeyAction::KeyPress) {
            view.setRenderStyle(view.renderStyle() == Style::Plastic ? Style::CAD
                              : view.renderStyle() == Style::CAD ? Style::Legacy : Style::Plastic);
        } else if (e->key() == ptgl::Key::Key_Q && e->keyAction() == ptgl::KeyEvent::KeyAction::KeyPress) {
            auto settings=view.renderQualitySettings();
            settings.edges=settings.edges==ptgl::EdgeQuality::Fast ? ptgl::EdgeQuality::Balanced
                : settings.edges==ptgl::EdgeQuality::Balanced ? ptgl::EdgeQuality::High : ptgl::EdgeQuality::Fast;
            view.setRenderQualitySettings(settings);
        } else if (e->key() == ptgl::Key::Key_P && e->keyAction() == ptgl::KeyEvent::KeyAction::KeyPress) {
            showStatistics=!showStatistics;
        } else if (e->key() == ptgl::Key::Key_C && e->keyAction() == ptgl::KeyEvent::KeyAction::KeyPress) {
            auto settings=view.renderQualitySettings(); settings.cacheStaticEdges=!settings.cacheStaticEdges;
            view.setRenderQualitySettings(settings); view.invalidateEdgeCache();
        } else if (e->key() == ptgl::Key::Key_Comma || e->key() == ptgl::Key::Key_Period) {
            auto settings=view.cadSettings();
            settings.brightness=std::clamp(settings.brightness+(e->key()==ptgl::Key::Key_Period ? 0.1 : -0.1),0.0,4.0);
            view.setCadSettings(settings);
        } else if (e->key() == ptgl::Key::Key_B && e->keyAction() == ptgl::KeyEvent::KeyAction::KeyPress) {
            auto settings = view.edgeSettings();
            settings.enabled = !settings.enabled;
            view.setEdgeSettings(settings);
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
            if (view.renderStyle() == Style::CAD) {
                auto settings = view.edgeSettings();
                settings.width = std::clamp(settings.width + (e->key() == ptgl::Key::Key_RightBracket ? 0.25 : -0.25), 0.5, 4.0);
                view.setEdgeSettings(settings);
            } else {
                auto settings = view.shadowSettings();
                settings.softness = std::clamp(settings.softness + (e->key() == ptgl::Key::Key_RightBracket ? 0.5 : -0.5), 0.0, 4.0);
                view.setShadowSettings(settings);
            }
        } else if (e->key() == ptgl::Key::Key_Up || e->key() == ptgl::Key::Key_Down) {
            auto material = view.defaultMaterial();
            material.roughness = std::clamp(material.roughness + (e->key() == ptgl::Key::Key_Up ? 0.05 : -0.05), 0.12, 1.0);
            view.setDefaultMaterial(material);
        } else if (e->key() == ptgl::Key::Key_Escape) {
            view.terminate();
        }
    });
    view.setRenderTextSceneFunction([&](ptgl::TextRenderer* r) {
        using Style = ptgl::StyledGraphicsView::RenderStyle;
        bool plastic = view.renderStyle() == Style::Plastic && view.plasticRenderingAvailable();
        bool cad = view.renderStyle() == Style::CAD && view.cadRenderingAvailable();
        r->setTextColor(1, 1, 1);
        if (cad) {
            r->drawText(20, 30, std::string("CAD | edges: ") + (view.edgesActive() ? "on" : "off")
                + " | width: " + std::to_string(view.edgeSettings().width).substr(0, 4) + " px"
                + " | shadows: " + (view.shadowsActive() ? "on" : "off"));
        } else {
            r->drawText(20, 30, std::string(plastic ? "Plastic" : "Legacy")
                + " | roughness: " + std::to_string(view.defaultMaterial().roughness).substr(0, 4)
                + " | shadows: " + (view.shadowsActive() ? "on" : "off")
                + " | softness: " + std::to_string(view.shadowSettings().softness).substr(0, 3));
        }
        r->drawText(20, 55, cad ? "Space: Plastic / CAD / Legacy   Right drag: orbit   Wheel: zoom"
            : "Space: Plastic / CAD / Legacy   Up/Down: roughness   Right drag: orbit   Wheel: zoom");
        r->drawText(20, 80, cad ? "B: toggle edges   [ / ]: edge width   S: shadows   L: moving light"
            : "S: shadows   [ / ]: softness   L: moving light   E: environment reflections   A: ambient occlusion");
        r->drawText(20, 105, "Left click: select   Drag arrows/planes: move   Drag rings: rotate   Click floor/background: deselect");
        if(cad && showStatistics) {
            const auto stats=view.renderStatistics();
            r->drawText(20,130,"Edges " + std::to_string(stats.edgeSupersampling) + "x | CPU "
                + std::to_string(stats.edgeCpuMilliseconds).substr(0,5) + " ms | GPU "
                + (stats.edgeGpuMilliseconds<0 ? "n/a" : std::to_string(stats.edgeGpuMilliseconds).substr(0,5)+" ms")
                + " | captures " + (stats.edgeCaptureReused ? "reused" : std::to_string(stats.edgeDrawCalls)+" draws"));
        } else r->drawText(20, 130, "Selected: " + (selectedObject ? selectedObject->name() + " (50% opacity)" : std::string("none"))
            + (cad ? " | Q: quality  P: timings  C: cache  , / .: brightness" : ""));
        r->drawText(20, 155, "Add: 1 sphere   2 rounded box   3 cylinder   4 cube   5 rounded cylinder   6 rounded cone");
        r->drawText(20, 180, "Delete: remove selected   Objects: " + std::to_string(objects.size()));
        const auto settings = view.sectionSettings();
        r->drawText(20, 205, std::string("X: section ") + (settings.enabled ? "on" : "off")
            + "   PgUp/PgDn: height " + std::to_string(settings.point[2]).substr(0, 5)
            + "   F: cap " + (settings.capEnabled ? "on" : "off") + "   K: reverse   Drop STL/OBJ to load");
        if (settings.enabled && selectedObject && selectedObject->mesh())
            r->drawText(20, 230, ptgl::sectionStatusMessage(selectedObject->sectionStatus()));
        if (!meshMessage.empty()) r->drawText(20, 255, meshMessage);
    });

    view.initialize();
    view.execute();
    while (!view.terminated()) std::this_thread::sleep_for(std::chrono::milliseconds(20));
    return 0;
}
