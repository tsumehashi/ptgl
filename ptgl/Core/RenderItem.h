#ifndef PTGL_CORE_RENDERITEM_H_
#define PTGL_CORE_RENDERITEM_H_

#include <vector>
#include "ptgl/Util/MathUtil.h"

namespace ptgl {

class Renderer3D;

class Render3DItem {
public:
    Render3DItem();
    virtual ~Render3DItem();

    // Light
    void setEffectLight(double e);

    void setPointSize(double size);
    void setLineWidth(double width);

    void setColor(double r, double g, double b, double a = 1.0);

    // draw functions
    void drawPoint(const double pos[3]);
    void drawLine(const double pos1[3], const double pos2[3]);
    void drawBox(const double pos[3], const double R[9], const double sides[3]);
    void drawSphere(const double pos[3], const double R[9], double r);
    void drawCylinder(const double pos[3], const double R[9], double length, double radius, bool cap = true);
    void drawCapsule(const double pos[3], const double R[9], double length, double radius);
    void drawCone(const double pos[3], const double R[9], double length, double radius);
    void drawRing(const double pos[3], const double R[9], double length, double outer_radius, double inner_radius);
    void drawCircle(const double pos[3], const double R[9], double r);
    void drawRingCircle(const double pos[3], const double R[9], double outer_radius, double inner_radius);
    void drawRect(const double pos[3], const double R[9], double xw, double yh);
    void drawArrow(const double pos1[3], const double pos2[3], double r);
    void drawArrowCone(const double pos1[3], const double pos2[3], double r);
    void drawAxis(const double pos[3], const double R[9], double length);
//    void drawGrid(double w, int div);

    void drawPoints(const double* p, int numpoints);
    void drawLines(const double* lines, int numlines);
//    void drawLineStrip(const double* lines, int numlines);
    void drawLineLoop(const double* lines, int numlines);

    void drawPoint(double x, double y, double z);
    void drawLine(double x1, double y1, double z1, double x2, double y2, double z2);

    // transform stack
    void pushMatrix();
    void popMatrix();

    void identiry();

    void scale(double s);
    void scale(double sx, double sy, double sz);
    void scale(const Eigen::Vector3d s);

    void translate(double x, double y, double z);
    void translate(const Eigen::Vector3d t);

    void rotate(const Eigen::Matrix3d& R);
    void rotate(double angle, const Eigen::Vector3d& axis);
    void rotateRpy(double r, double p, double y);
    void rotateX(double deg);
    void rotateY(double deg);
    void rotateZ(double deg);

    void transform(const Eigen::Affine3d& t);
    void transform(const Eigen::Matrix4d& m);
    void transform(const Eigen::Vector3d& p, const Eigen::Matrix3d& R);

    // serialize/desirialize
    std::vector<uint8_t> serialize() const;
    void deserialize(const std::vector<uint8_t>& data);

    void clear();

    // render
    void render(Renderer3D* r);

private:
    int getItemType(uint8_t* ptr) const;
    int getItemSize(int itemType) const;
    void renderSub(Renderer3D* r, uint8_t* ptr, int itemType);

    enum ItemType {
        EffectLightItemType = 0,
        PointSizeItemType,
        LineWidthItemType,
        ColorItemType,
        DrawPointItemType,
        DrawLineItemType,
        DrawBoxItemType,
        DrawSphereItemType,
        DrawCylinderItemType,
        DrawCapsuleItemType,
        DrawConeItemType,
        DrawRingItemType,
        DrawCircleItemType,
        DrawRingCircleItemType,
        DrawRectItemType,
        DrawArrowItemType,
        DrawArrowConeItemType,
        DrawAxisItemType,
        DrawGridItemType,
        PushMatrixItemType,
        PopMatrixItemType,
        IdentityMatrixItemType,
        TranslateItemType,
        RotateItemType,
        RpyRotateItemType,
        TransformItemType,
        ScaleItemType,
        NumItemType,
    };

#pragma pack(1)
    struct EffectLightItem { const int itemType = EffectLightItemType; double effect;    };
    struct PointSizeItem { const int itemType = PointSizeItemType; double size;    };
    struct LineWidthItem { const int itemType = LineWidthItemType; double width;    };
    struct ColorItem { const int itemType = ColorItemType ;double rgba[4]; };

    struct DrawPointItem { const int itemType = DrawPointItemType;double pos[3]; };
    struct DrawLineItem { const int itemType = DrawLineItemType;double pos1[3]; double pos2[3]; };
    struct DrawBoxItem { const int itemType = DrawBoxItemType;double pos[3]; double R[9]; double sides[3]; };
    struct DrawSphereItem { const int itemType = DrawSphereItemType;double pos[3]; double R[9]; double r; };
    struct DrawCylinderItem { const int itemType = DrawCylinderItemType; double pos[3]; double R[9]; double length; double radius; int drawCap; };
    struct DrawCapsuleItem { const int itemType = DrawCapsuleItemType; double pos[3]; double R[9]; double length; double radius; };
    struct DrawConeItem { const int itemType = DrawConeItemType; double pos[3]; double R[9]; double length; double radius; };
    struct DrawRingItem { const int itemType = DrawRingItemType; double pos[3]; double R[9]; double length; double outer_radius; double inner_radius; };
    struct DrawCircleItem { const int itemType = DrawCircleItemType; double pos[3]; double R[9]; double r; };
    struct DrawRingCircleItem { const int itemType = DrawRingCircleItemType; double pos[3]; double R[9]; double outer_radius; double inner_radius; };
    struct DrawRectItem { const int itemType = DrawRectItemType; double pos[3]; double R[9]; double w; double h; };
    struct DrawArrowItem { const int itemType = DrawArrowItemType; double pos1[3]; double pos2[3]; double r; };
    struct DrawArrowConeItem { const int itemType = DrawArrowConeItemType; double pos1[3]; double pos2[3]; double r; };
    struct DrawAxisItem { const int itemType = DrawAxisItemType; double pos[3]; double R[9]; double length; };
    struct DrawGridItem { const int itemType = DrawGridItemType; double w; int div; };

    struct PushMatrixItem { const int itemType = PushMatrixItemType; };
    struct PopMatrixItem { const int itemType = PopMatrixItemType; };
    struct IdentityMatrixItem { const int itemType = IdentityMatrixItemType; };
    struct TranslateItem { const int itemType = TranslateItemType; double t[3]; };
    struct RotateItem { const int itemType = RotateItemType; double R[9]; };
    struct RpyRotateItem { const int itemType = RpyRotateItemType; double rpy[3]; };
    struct TransformItem { const int itemType = TransformItemType; double pos[3]; double R[9]; };
    struct ScaleItem { const int itemType = ScaleItemType; double scale[3]; };
#pragma pack()

    std::vector<uint8_t> data_;
};

} /* namespace ptgl */

#endif /* PTGL_CORE_RENDER3DITEM_H_ */
