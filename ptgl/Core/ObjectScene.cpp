#include "ObjectScene.h"
#include "GraphicsView.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <filesystem>
#include <cctype>
#include "ptgl/Loader/STL/STLLoader.h"
#include "ptgl/Loader/OBJ/OBJLoader.h"

namespace ptgl {
SceneObject::SceneObject(const std::string& name, const Eigen::Vector3d& position, DrawFunction draw)
    : transform_(std::make_shared<Transform>(position, Eigen::Matrix3d::Identity())),
      draw_(std::move(draw)), lastPosition_(position), lastRotation_(Eigen::Matrix3d::Identity())
{
    if (!position.allFinite()) throw std::invalid_argument("SceneObject position must be finite");
    setName(name);
}

void SceneObject::setDrawFunction(DrawFunction draw)
{
    mesh_.reset(); shape_.reset(); bounds_.setEmpty();
    sectionStatus_ = SectionStatus::Disabled;
    draw_ = std::move(draw);
    notifyGeometryChanged();
}

void SceneObject::replaceMesh(std::shared_ptr<const VertexSet> mesh)
{
    bounds_.setEmpty();
    for (const auto& v : mesh->vertices) bounds_.extend(Eigen::Vector3d(v.x, v.y, v.z));
    mesh_ = std::move(mesh); draw_ = {}; applyColor_ = true;
    sectionStatus_ = SectionStatus::Disabled;
    notifyGeometryChanged();
}

void SceneObject::setShape(const PrimitiveShape& shape)
{
    auto mesh = std::make_shared<const VertexSet>(generatePrimitiveMesh(shape));
    shape_ = shape;
    replaceMesh(std::move(mesh));
}

void SceneObject::setMesh(const VertexSet& mesh, const MeshProcessingSettings& settings)
{
    for (const auto& v : mesh.vertices) for (float f : v.data)
        if (!std::isfinite(f)) throw std::invalid_argument("Mesh attributes must be finite");
    auto prepared = std::make_shared<const VertexSet>(prepareCadMesh(mesh, settings));
    if (prepared->indices.empty()) throw std::invalid_argument("Mesh must contain nondegenerate triangles");
    shape_.reset();
    replaceMesh(std::move(prepared));
}

void SceneObject::setColor(double red, double green, double blue)
{
    for (double c : {red, green, blue}) if (!std::isfinite(c) || c < 0 || c > 1)
        throw std::invalid_argument("Object RGB must be in [0,1]");
    color_ = {{red, green, blue}}; applyColor_ = true;
}

void SceneObject::setMaterial(const Material& material)
{
    if (!std::isfinite(material.roughness) || material.roughness < 0.12 || material.roughness > 1
        || !std::isfinite(material.specularReflectance) || material.specularReflectance < 0 || material.specularReflectance > 1)
        throw std::invalid_argument("Object material needs roughness [0.12,1] and reflectance [0,1]");
    material_ = material;
}

Eigen::AlignedBox3d SceneObject::worldBounds() const
{
    Eigen::AlignedBox3d result;
    if (!bounds_.isEmpty()) for (int i = 0; i < 8; ++i)
        result.extend(transform_->position() + transform_->rotation() * bounds_.corner(static_cast<Eigen::AlignedBox3d::CornerType>(i)));
    return result;
}

void SceneObject::notifyGeometryChanged()
{
    if (graphicsWindow()) graphicsWindow()->notifySceneChanged();
}

void SceneObject::renderScene(Renderer3D* r)
{
    std::array<double, 4> savedColor;
    r->getColor(savedColor[0], savedColor[1], savedColor[2], savedColor[3]);
    const auto savedMaterial = r->material();
    r->pushMatrix();
    try {
        r->transform(transform_->position(), transform_->rotation());
        if (applyColor_) r->setColor(color_);
        if (material_) r->setMaterial(*material_);
        if (mesh_) {
            r->drawSharedMesh(mesh_);
            sectionStatus_ = r->lastSectionStatus();
        } else if (draw_) draw_(r);
    } catch (...) {
        r->popMatrix();
        r->setColor(savedColor); r->setMaterial(savedMaterial);
        throw;
    }
    r->popMatrix();
    r->setColor(savedColor); r->setMaterial(savedMaterial);
}

ObjectScene::ObjectScene(GraphicsView& view) : view_(view), handle_(std::make_shared<handle::TransformHandle>())
{
    idleTransform_ = handle_->transform();
    handle_->setEnabled(false);
    view_.addGraphicsItem(handle_);
}

ObjectScene::~ObjectScene()
{
    if (selected_) selected_->setOpacity(originalOpacity_);
    handle_->setEnabled(false);
    handle_->setTransform(idleTransform_);
}

SceneObjectPtr ObjectScene::addObject(const std::string& name, const Eigen::Vector3d& position,
                                     SceneObject::DrawFunction draw)
{
    auto object = std::make_shared<SceneObject>(name, position, std::move(draw));
    objects_.push_back(object);
    view_.addGraphicsItem(object);
    return object;
}

SceneObjectPtr ObjectScene::addPrimitive(const std::string& name, const PrimitiveShape& shape,
                                        const Eigen::Vector3d& position)
{
    auto object = std::make_shared<SceneObject>(name, position, SceneObject::DrawFunction{});
    object->setShape(shape);
    objects_.push_back(object); view_.addGraphicsItem(object);
    return object;
}

SceneObjectPtr ObjectScene::addMesh(const std::string& name, const VertexSet& mesh,
                                   const MeshProcessingSettings& settings)
{
    auto object = std::make_shared<SceneObject>(name, Eigen::Vector3d::Zero(), SceneObject::DrawFunction{});
    object->setMesh(mesh, settings);
    objects_.push_back(object); view_.addGraphicsItem(object);
    return object;
}

SceneObjectPtr ObjectScene::loadMesh(const std::string& name, const std::string& path,
                                    const MeshProcessingSettings& settings)
{
    auto extension = std::filesystem::u8path(path).extension().u8string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    VertexListPtr vertices;
    if (extension == ".stl") vertices = loader::STLLoader().loadVertex(path);
    else if (extension == ".obj") vertices = loader::OBJLoader().loadVertex(path);
    else throw std::invalid_argument("Unsupported mesh format (use STL or OBJ): " + path);
    if (!vertices || vertices->empty()) throw std::runtime_error("Failed to load a nonempty mesh: " + path);
    VertexSet mesh; mesh.vertices = std::move(*vertices);
    return addMesh(name, mesh, settings);
}

bool ObjectScene::removeObject(const SceneObjectPtr& object)
{
    // Copy: callers may pass a reference into objects_ or selected_.
    auto target = object;
    if (!target || std::find(objects_.begin(), objects_.end(), target) == objects_.end()) return false;
    view_.removeGraphicsItem(target); // Also notifies itemRemoved().
    return true;
}

bool ObjectScene::removeSelectedObject() { return removeObject(selected_); }

double ObjectScene::objectOpacity(const SceneObjectPtr &object) const
{
    if (!object)
        throw std::invalid_argument("Null object");
    return object == selected_ ? originalOpacity_ : object->opacity();
}
void ObjectScene::setObjectOpacity(const SceneObjectPtr &object, double opacity)
{
    if (!object || !std::isfinite(opacity) || opacity < 0 || opacity > 1)
        throw std::invalid_argument("Invalid object opacity");
    if (object == selected_) {
        originalOpacity_ = opacity;
        object->setOpacity(opacity * selectionOpacity_);
    } else
        object->setOpacity(opacity);
}

void ObjectScene::clear()
{
    const auto objects = objects_;
    for (const auto& object : objects) removeObject(object);
}

void ObjectScene::selectObject(const SceneObjectPtr& object)
{
    auto target = object;
    if (target && (!editingEnabled_ || target->graphicsWindow() != &view_ || !target->isEnabled()
        || !target->isVisible() || !target->isPickable()
        || std::find(objects_.begin(), objects_.end(), target) == objects_.end()))
        throw std::invalid_argument("Object is not editable in this scene");
    if (target == selected_) return;
    view_.cancelGraphicsItemDrag();
    if (selected_) selected_->setOpacity(originalOpacity_);
    selected_ = target;
    handle_->setEnabled(bool(selected_));
    handle_->setTransform(selected_ ? selected_->transform() : idleTransform_);
    if (selected_) {
        originalOpacity_ = selected_->opacity();
        selected_->setOpacity(originalOpacity_ * selectionOpacity_);
    }
}

void ObjectScene::setSelectionOpacity(double opacity)
{
    if (!std::isfinite(opacity) || opacity < 0 || opacity > 1)
        throw std::invalid_argument("Selection opacity must be in [0,1]");
    selectionOpacity_ = opacity;
    if (selected_) selected_->setOpacity(originalOpacity_ * opacity);
}

void ObjectScene::setEditingEnabled(bool enabled)
{
    if (!enabled) selectObject(nullptr);
    if (enabled && handle_->graphicsWindow() != &view_) view_.addGraphicsItem(handle_);
    editingEnabled_ = enabled;
}

void ObjectScene::pick(const GraphicsItemPtr& item)
{
    if (!editingEnabled_) return;
    for (auto p = item.get(); p; p = p->parentItem()) {
        if (p == handle_.get()) return;
        auto found = std::find_if(objects_.begin(), objects_.end(),
                                 [&](const SceneObjectPtr& object) { return object.get() == p; });
        if (found != objects_.end()) {
            const auto& object = *found;
            selectObject(object->isEnabled() && object->isVisible() && object->isPickable()
                         && object->graphicsWindow() == &view_ ? object : nullptr);
            return;
        }
    }
    selectObject(nullptr);
}

void ObjectScene::update()
{
    if (selected_ && (!selected_->isEnabled() || !selected_->isVisible() || !selected_->isPickable()))
        selectObject(nullptr);
    bool changed = false;
    for (auto& object : objects_) {
        const auto& t = object->transform();
        if (object->lastPosition_ != t->position() || object->lastRotation_ != t->rotation()) changed = true;
        object->lastPosition_ = t->position();
        object->lastRotation_ = t->rotation();
    }
    if (changed) view_.notifySceneChanged();
}

void ObjectScene::itemRemoved(const GraphicsItemPtr& item)
{
    if (item == handle_) { selectObject(nullptr); editingEnabled_ = false; }
    if (item == selected_) selectObject(nullptr);
    objects_.erase(std::remove(objects_.begin(), objects_.end(), item), objects_.end());
}
} // namespace ptgl
