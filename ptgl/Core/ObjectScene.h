#ifndef PTGL_CORE_OBJECTSCENE_H_
#define PTGL_CORE_OBJECTSCENE_H_

#include "GraphicsItem.h"
#include "PrimitiveShape.h"
#include "Material.h"
#include "SectionSettings.h"
#include "ptgl/Util/MeshProcessing.h"
#include <optional>
#include <Eigen/Geometry>
#include "ptgl/Handle/TransformHandle.h"

namespace ptgl {

// Draw in local coordinates. The same transform is applied in every scene pass.
class SceneObject : public GraphicsItem {
public:
    using DrawFunction = std::function<void(Renderer3D*)>;
    SceneObject(const std::string& name, const Eigen::Vector3d& position, DrawFunction draw);
    const TransformPtr& transform() const { return transform_; }
    void setDrawFunction(DrawFunction draw);
    // Atomic geometry replacement: invalid input leaves the object unchanged.
    void setShape(const PrimitiveShape& shape);
    const std::optional<PrimitiveShape>& shape() const { return shape_; }
    void setMesh(const VertexSet& mesh, const MeshProcessingSettings& settings = {});
    const std::shared_ptr<const VertexSet>& mesh() const { return mesh_; }
    void setColor(double red, double green, double blue);
    const std::array<double, 3>& color() const { return color_; }
    void setMaterial(const Material& material);
    const std::optional<Material>& material() const { return material_; }
    void clearMaterial() { material_.reset(); }
    // Callback-only objects have empty bounds unless replaced by a shape/mesh.
    const Eigen::AlignedBox3d& localBounds() const { return bounds_; }
    Eigen::AlignedBox3d worldBounds() const;
    // Result from the most recent mesh draw, including cap failure reasons.
    SectionStatus sectionStatus() const { return sectionStatus_; }
    // Call when data captured by the draw callback changes its geometry.
    void notifyGeometryChanged();

protected:
    void renderScene(Renderer3D* r) override;

private:
    friend class ObjectScene;
    TransformPtr transform_;
    DrawFunction draw_;
    std::optional<PrimitiveShape> shape_;
    std::shared_ptr<const VertexSet> mesh_;
    Eigen::AlignedBox3d bounds_;
    std::array<double, 3> color_{{1, 1, 1}};
    bool applyColor_ = false;
    std::optional<Material> material_;
    SectionStatus sectionStatus_ = SectionStatus::Disabled;
    void replaceMesh(std::shared_ptr<const VertexSet> mesh);
    Eigen::Vector3d lastPosition_;
    Eigen::Matrix3d lastRotation_;
};
using SceneObjectPtr = std::shared_ptr<SceneObject>;

// Owned by GraphicsView; obtained through view.objectScene(). All operations
// require the view/event thread (or an idle view before execution).
class ObjectScene {
public:
    ~ObjectScene();
    ObjectScene(const ObjectScene&) = delete;
    ObjectScene& operator=(const ObjectScene&) = delete;

    SceneObjectPtr addObject(const std::string& name, const Eigen::Vector3d& position,
                             SceneObject::DrawFunction draw);
    SceneObjectPtr addPrimitive(const std::string& name, const PrimitiveShape& shape,
                                const Eigen::Vector3d& position = Eigen::Vector3d::Zero());
    SceneObjectPtr addMesh(const std::string& name, const VertexSet& mesh,
                           const MeshProcessingSettings& settings = {});
    // STL/OBJ, case-insensitive extension; throws on unsupported/empty/failed input.
    SceneObjectPtr loadMesh(const std::string& name, const std::string& path,
                            const MeshProcessingSettings& settings = {});
    bool removeObject(const SceneObjectPtr& object);
    bool removeSelectedObject();
    void clear();
    const std::vector<SceneObjectPtr>& objects() const { return objects_; }

    // nullptr clears selection. Foreign, hidden or disabled objects are rejected.
    void selectObject(const SceneObjectPtr& object);
    const SceneObjectPtr& selectedObject() const { return selected_; }
    // Multiplier of the object's original opacity, [0,1], default 0.5.
    // The original opacity is restored when selection ends.
    void setSelectionOpacity(double opacity);
    double selectionOpacity() const { return selectionOpacity_; }
    double objectOpacity(const SceneObjectPtr &object) const;
    void setObjectOpacity(const SceneObjectPtr &object, double opacity);
    void setEditingEnabled(bool enabled);
    bool editingEnabled() const { return editingEnabled_; }
    const handle::TransformHandlePtr& transformHandle() const { return handle_; }

private:
    friend class GraphicsView;
    explicit ObjectScene(GraphicsView& view);
    void pick(const GraphicsItemPtr& item);
    void update();
    void itemRemoved(const GraphicsItemPtr& item);
    GraphicsView& view_;
    std::vector<SceneObjectPtr> objects_;
    SceneObjectPtr selected_;
    handle::TransformHandlePtr handle_;
    TransformPtr idleTransform_;
    double originalOpacity_ = 1;
    double selectionOpacity_ = 0.5;
    bool editingEnabled_ = true;
};

} // namespace ptgl
#endif
