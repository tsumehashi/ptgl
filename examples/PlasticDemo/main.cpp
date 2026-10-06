#include <algorithm>
#include <array>
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
#include "ptgl/Util/MeshProcessing.h"

class DemoView : public ptgl::QuickPlasticGraphicsView {
public:
    using ptgl::QuickPlasticGraphicsView::QuickPlasticGraphicsView;

    // End the current handle gesture before deleting or selecting another object.
    // Subsequent mouse moves while the button is held must not move the new selection.
    void finishObjectDrag() {
        ptgl::MouseEvent release(this);
        release.setReleaseEvent(getMouseEvent().x(), getMouseEvent().y(),
                                ptgl::MouseEvent::MouseButton::LeftButton);
        executeGraphicsItemMouseReleaseEvent(&release);
    }
};

int main()
{
    DemoView view(std::make_unique<ptgl::GLFWGraphicsDriver>());
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

    // Preprocess once, then keep the geometry in a registered GPU buffer.
    auto roundedBox = ptgl::PrimitiveShapeVertex::generateRoundedBox({1.5, 1.5, 1.5}, 0.18);
    // Explicit tangent boundaries between flat faces and the rounded fillets.
    // Imported CAD meshes can supply the same pairs from their face metadata.
    for (int axis=0; axis<3; ++axis) for (int sign : {-1,1}) {
        GLuint base=static_cast<GLuint>(roundedBox.vertices.size());
        for (const auto& corner : {std::pair<double,double>{-1,-1}, {1,-1}, {1,1}, {-1,1}}) {
            Eigen::Vector3d p=Eigen::Vector3d::Zero(), n=Eigen::Vector3d::Zero();
            p[axis]=sign*0.75; p[(axis+1)%3]=corner.first*0.57; p[(axis+2)%3]=corner.second*0.57;
            n[axis]=sign;
            roundedBox.vertices.emplace_back(float(p.x()),float(p.y()),float(p.z()),float(n.x()),float(n.y()),float(n.z()));
        }
        for(GLuint i=0;i<4;++i) roundedBox.edges.insert(roundedBox.edges.end(),{base+i,base+(i+1)%4});
    }
    roundedBox = ptgl::prepareCadMesh(roundedBox);
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
        Eigen::Vector3d lastPosition;
        Eigen::Matrix3d lastRotation;
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
        objects.push_back({item, transform, position, transform->rotation()});
        view.addGraphicsItem(item);
        view.invalidateEdgeCache();
    };
    const std::array<std::string, 4> shapeNames{{"Sphere", "Rounded box", "Cylinder", "Cube"}};
    constexpr std::array<double, 4> shapeHeights{{0.8, 0.75, 0.8, 0.75}};
    const std::array<std::function<void(ptgl::Renderer3D*)>, 4> drawShapes{{[](ptgl::Renderer3D* r) {
        r->setColor(0.86, 0.24, 0.13);
        r->drawSphere(r->p0(), r->R0(), 0.8);
    }, [&](ptgl::Renderer3D* r) {
        r->setColor(0.16, 0.52, 0.83);
        if (!r->isRegisteredVertices("roundedBox")) r->registerMesh("roundedBox", roundedBox);
        r->drawRegisteredVertices("roundedBox");
    }, [](ptgl::Renderer3D* r) {
        r->setColor(0.95, 0.64, 0.12);
        r->drawCylinder(r->p0(), r->R0(), 1.6, 0.72);
    }, [](ptgl::Renderer3D* r) {
        r->setColor(0.30, 0.68, 0.37);
        r->drawBox(r->p0(), r->R0(), {1.5, 1.5, 1.5});
    }}};
    for (size_t shape = 0; shape < 3; ++shape)
        addObject(shapeNames[shape], {0, (double(shape) - 1) * 2.1, shapeHeights[shape]}, drawShapes[shape]);
    std::array<size_t, 4> shapeSerials{{1, 1, 1, 0}};

    // Handle changes occur during event processing; invalidate before this frame
    // renders. Selection fading keeps the same nearest surfaces and needs no reset.
    view.setPostEventProcessFunction([&] {
        bool changed=false;
        for (auto& object : objects) {
            const auto& t = object.transform;
            if (object.lastPosition != t->position() || object.lastRotation != t->rotation()) changed = true;
            object.lastPosition = t->position();
            object.lastRotation = t->rotation();
        }
        if(changed) view.invalidateEdgeCache();
    });

    auto handle = std::make_shared<ptgl::handle::TransformHandle>();
    handle->setEnabled(false);
    view.addGraphicsItem(handle);
    const auto idleTransform = handle->transform();
    auto selectObject = [&](const DemoObject* object) {
        view.finishObjectDrag();
        if (selectedObject) selectedObject->setOpacity(1.0);
        selectedObject = object ? object->item : nullptr;
        handle->setEnabled(bool(object));
        handle->setTransform(object ? object->transform : idleTransform);
        if (selectedObject) selectedObject->setOpacity(selectedOpacity);
    };
    auto addShape = [&](size_t shape) {
        // Search outward for a free grid position, reusing gaps left by deletions.
        // Existing transforms are checked because objects may have been dragged.
        for (int ring = 0; ; ++ring) {
            for (int x = ring; x >= -ring; --x) for (int y = -ring; y <= ring; ++y) {
                if (std::max(std::abs(x), std::abs(y)) != ring) continue;
                Eigen::Vector3d position(x * 2.1, y * 2.1, shapeHeights[shape]);
                bool occupied = std::any_of(objects.begin(), objects.end(), [&](const DemoObject& object) {
                    return (object.transform->position() - position).head<2>().norm() < 2.0;
                });
                if (occupied) continue;
                addObject(shapeNames[shape] + " " + std::to_string(++shapeSerials[shape]), position, drawShapes[shape]);
                selectObject(&objects.back());
                return;
            }
        }
    };
    auto deleteSelectedObject = [&] {
        auto found = std::find_if(objects.begin(), objects.end(), [&](const DemoObject& object) {
            return object.item == selectedObject;
        });
        if (found == objects.end()) return;
        selectObject(nullptr);
        view.removeGraphicsItem(found->item);
        objects.erase(found);
        view.invalidateEdgeCache();
    };
    view.setPickingUpEventFunction([&](ptgl::PickingEvent* e) {
        auto picked = e->pickedGraphicsItem();
        // Handle parts are children: clicking one must preserve the selection.
        for (auto item = picked.get(); item; item = item->parentItem()) {
            if (item == handle.get()) return;
        }
        for (const auto& object : objects) {
            if (object.item == picked) {
                selectObject(&object);
                return;
            }
        }
        selectObject(nullptr);
    });

    view.setKeyPressEventFunction([&](ptgl::KeyEvent* e) {
        if (e->keyAction() == ptgl::KeyEvent::KeyAction::KeyRelease) return;
        using Style = ptgl::PlasticGraphicsView::RenderStyle;
        if (e->key() == ptgl::Key::Key_1 || e->key() == ptgl::Key::Key_2 ||
            e->key() == ptgl::Key::Key_3 || e->key() == ptgl::Key::Key_4) {
            if (e->keyAction() == ptgl::KeyEvent::KeyAction::KeyPress)
                addShape(e->key() == ptgl::Key::Key_1 ? 0 : e->key() == ptgl::Key::Key_2 ? 1 :
                         e->key() == ptgl::Key::Key_3 ? 2 : 3);
        } else if (e->key() == ptgl::Key::Key_Delete) {
            if (e->keyAction() == ptgl::KeyEvent::KeyAction::KeyPress) deleteSelectedObject();
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
        using Style = ptgl::PlasticGraphicsView::RenderStyle;
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
        r->drawText(20, 155, "1: add sphere   2: add rounded box   3: add cylinder   4: add cube   Delete: remove selected   Objects: "
            + std::to_string(objects.size()));
    });

    view.initialize();
    view.execute();
    while (!view.terminated()) std::this_thread::sleep_for(std::chrono::milliseconds(20));
    return 0;
}
