#include "RenderItem.h"
#include <iostream>
#include "Renderer3D.h"

namespace {
template <typename T>
void setItem(std::vector<uint8_t>& data, const T& item) {
    uint8_t* ptr = (uint8_t*)&item;
    for (size_t i = 0; i < sizeof(T); ++i) {
        data.push_back(*ptr);
        ++ptr;
    }
}

template <typename T>
void setPos(T& item, const double pos[3], const double R[9]) {
    for (size_t i = 0; i < 3; ++i) {
        item.pos[i] = pos[i];
    }

    for (size_t i = 0; i < 9; ++i) {
        item.R[i] = R[i];
    }
}

template <int N>
void setVal(double* a, const double* b) {
    for (int i = 0; i < N; ++i) {
        a[i] = b[i];
    }
}

}

namespace ptgl {

Render3DItem::Render3DItem()
{

}

Render3DItem::~Render3DItem()
{

}

// Light
void Render3DItem::setEffectLight(double e)
{
    EffectLightItem item;
    item.effect = e;
    setItem(data_, item);
}

void Render3DItem::setPointSize(double size)
{
    PointSizeItem item;
    item.size = size;
    setItem(data_, item);
}

void Render3DItem::setLineWidth(double width)
{
    LineWidthItem item;
    item.width = width;
    setItem(data_, item);
}

void Render3DItem::setColor(double r, double g, double b, double a)
{
    ColorItem item;
    item.rgba[0] = r;
    item.rgba[1] = g;
    item.rgba[2] = b;
    item.rgba[3] = a;
    setItem(data_, item);
}

// draw functions
void Render3DItem::drawPoint(const double pos[3])
{
    DrawPointItem item;
    item.pos[0] = pos[0];
    item.pos[1] = pos[1];
    item.pos[2] = pos[2];
    setItem(data_, item);
}

void Render3DItem::drawLine(const double pos1[3], const double pos2[3])
{
    DrawLineItem item;
    item.pos1[0] = pos1[0];
    item.pos1[1] = pos1[1];
    item.pos1[2] = pos1[2];
    item.pos2[0] = pos2[0];
    item.pos2[1] = pos2[1];
    item.pos2[2] = pos2[2];
    setItem(data_, item);
}

void Render3DItem::drawBox(const double pos[3], const double R[9], const double sides[3])
{
    DrawBoxItem item;
    setPos(item, pos, R);
    setVal<3>(item.sides, sides);
    setItem(data_, item);
}

void Render3DItem::drawSphere(const double pos[3], const double R[9], double r)
{
    DrawSphereItem item;
    setPos(item, pos, R);
    item.r = r;
    setItem(data_, item);
}

void Render3DItem::drawCylinder(const double pos[3], const double R[9], double length, double radius, bool cap)
{
    DrawCylinderItem item;
    setPos(item, pos, R);
    item.length = length;
    item.radius = radius;
    item.drawCap = (int)cap;
    setItem(data_, item);
}

void Render3DItem::drawCapsule(const double pos[3], const double R[9], double length, double radius)
{
    DrawCapsuleItem item;
    setPos(item, pos, R);
    item.length = length;
    item.radius = radius;
    setItem(data_, item);
}

void Render3DItem::drawCone(const double pos[3], const double R[9], double length, double radius)
{
    DrawConeItem item;
    setPos(item, pos, R);
    item.length = length;
    item.radius = radius;
    setItem(data_, item);
}

void Render3DItem::drawRing(const double pos[3], const double R[9], double length, double outer_radius, double inner_radius)
{
    DrawRingItem item;
    setPos(item, pos, R);
    item.length = length;
    item.outer_radius = outer_radius;
    item.inner_radius = inner_radius;
    setItem(data_, item);
}

void Render3DItem::drawCircle(const double pos[3], const double R[9], double r)
{
    DrawCircleItem item;
    setPos(item, pos, R);
    item.r = r;
    setItem(data_, item);
}

void Render3DItem::drawRingCircle(const double pos[3], const double R[9], double outer_radius, double inner_radius)
{
    DrawRingCircleItem item;
    setPos(item, pos, R);
    item.outer_radius = outer_radius;
    item.inner_radius = inner_radius;
    setItem(data_, item);
}

void Render3DItem::drawRect(const double pos[3], const double R[9], double xw, double yh)
{
    DrawRectItem item;
    setPos(item, pos, R);
    item.w = xw;
    item.h = yh;
    setItem(data_, item);
}

void Render3DItem::drawArrow(const double pos1[3], const double pos2[3], double r)
{
    DrawArrowItem item;
    setVal<3>(item.pos1, pos1);
    setVal<3>(item.pos2, pos2);
    item.r = r;
    setItem(data_, item);
}

void Render3DItem::drawArrowCone(const double pos1[3], const double pos2[3], double r)
{
    DrawArrowConeItem item;
    setVal<3>(item.pos1, pos1);
    setVal<3>(item.pos2, pos2);
    item.r = r;
    setItem(data_, item);
}

void Render3DItem::drawAxis(const double pos[3], const double R[9], double length)
{
    DrawAxisItem item;
    setPos(item, pos, R);
    item.length = length;
    setItem(data_, item);
}

void Render3DItem::drawPoints(const double* p, int numpoints)
{
    double* ptr = (double*)p;
    for (int i = 0; i < numpoints; ++i) {
        drawPoint(ptr[0], ptr[1], ptr[2]);
        ptr += 3;
    }
}

void Render3DItem::drawLines(const double* lines, int numlines)
{
    if (numlines < 2) return;

    double* ptr = (double*)lines;
    for (int i = 0; i < numlines-1; ++i) {
        drawLine(ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
        ptr += 3;
    }
}

//void RenderItem::drawLineStrip(const double* lines, int numlines)
//{
//
//}

void Render3DItem::drawLineLoop(const double* lines, int numlines)
{
    if (numlines < 2) return;

    double* ptr = (double*)lines;
    for (int i = 0; i < numlines-1; ++i) {
        drawLine(ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
        ptr += 3;
    }

    int idx = 3*(numlines-1);
    drawLine(lines[0], lines[1], lines[2], lines[idx], lines[idx+1], lines[idx+2]);
}

void Render3DItem::drawPoint(double x, double y, double z)
{
    DrawPointItem item;
    item.pos[0] = x;
    item.pos[1] = y;
    item.pos[2] = z;
    setItem(data_, item);
}

void Render3DItem::drawLine(double x1, double y1, double z1, double x2, double y2, double z2)
{
    DrawLineItem item;
    item.pos1[0] = x1;
    item.pos1[1] = y1;
    item.pos1[2] = z1;
    item.pos2[0] = x2;
    item.pos2[1] = y2;
    item.pos2[2] = z2;
    setItem(data_, item);
}

// transform stack
void Render3DItem::pushMatrix()
{
    PushMatrixItem item;
    setItem(data_, item);
}

void Render3DItem::popMatrix()
{
    PopMatrixItem item;
    setItem(data_, item);
}

void Render3DItem::identiry()
{
    IdentityMatrixItem item;
    setItem(data_, item);
}

void Render3DItem::scale(double s)
{
    scale(s, s, s);
}

void Render3DItem::scale(double sx, double sy, double sz)
{
    ScaleItem item;
    item.scale[0] = sx;
    item.scale[1] = sy;
    item.scale[2] = sz;
    setItem(data_, item);
}

void Render3DItem::scale(const Eigen::Vector3d s)
{
    scale(s(0), s(1), s(2));
}

void Render3DItem::translate(double x, double y, double z)
{
    TranslateItem item;
    item.t[0] = x;
    item.t[1] = y;
    item.t[2] = z;
    setItem(data_, item);
}

void Render3DItem::translate(const Eigen::Vector3d t)
{
    translate(t(0), t(1), t(2));
}

void Render3DItem::rotate(const Eigen::Matrix3d& R)
{
    RotateItem item;
    setVal<9>(item.R, R.data());
    setItem(data_, item);
}

void Render3DItem::rotate(double angle, const Eigen::Vector3d& axis)
{
    Eigen::Matrix3d R = ptgl::rodrigues(axis, angle);
    rotate(R);
}

void Render3DItem::rotateRpy(double r, double p, double y)
{
    RpyRotateItem item;
    item.rpy[0] = r;
    item.rpy[1] = p;
    item.rpy[2] = y;
    setItem(data_, item);
}

void Render3DItem::rotateX(double deg)
{
    rotateRpy(deg, 0, 0);
}

void Render3DItem::rotateY(double deg)
{
    rotateRpy(0, deg, 0);
}

void Render3DItem::rotateZ(double deg)
{
    rotateRpy(0, 0, deg);
}

void Render3DItem::transform(const Eigen::Affine3d& t)
{
    TransformItem item;
    setPos(item, t.translation().data(), t.linear().data());
    setItem(data_, item);
}

void Render3DItem::transform(const Eigen::Matrix4d& m)
{
    Eigen::Affine3d t;
    t = m;
    transform(t);
}

void Render3DItem::transform(const Eigen::Vector3d& p, const Eigen::Matrix3d& R)
{
    TransformItem item;
    setPos(item, p.data(), R.data());
    setItem(data_, item);
}

std::vector<uint8_t> Render3DItem::serialize() const
{
    return data_;
}

void Render3DItem::deserialize(const std::vector<uint8_t>& data)
{
    data_ = data;
}

void Render3DItem::clear()
{
    data_.clear();
}

// render
void Render3DItem::render(Renderer3D* r)
{
    if (data_.empty()) return;
    if (!r) return;

    uint8_t*  ptr = data_.data();
    int index = 0;
    while (index < (int)data_.size()) {
        int itemType = getItemType(&ptr[index]);
        int itemSize = getItemSize(itemType);
//        std::cout << "type = " << itemType << ", itemSize = " << itemSize << std::endl;
        if (itemSize == 0) {
            // parse error
            std::cout << "parse error" << std::endl;
            break;
        }

        if ((index + itemSize) <= (int)data_.size()) {
            renderSub(r, &ptr[index], itemType);
        }
        index += itemSize;
    }
}

int Render3DItem::getItemType(uint8_t* ptr) const
{
    return *((int*)ptr);
}

int Render3DItem::getItemSize(int itemType) const
{
    switch (itemType) {
    case EffectLightItemType: return sizeof(EffectLightItem); break;
    case PointSizeItemType: return sizeof(PointSizeItem); break;
    case LineWidthItemType: return sizeof(LineWidthItem); break;
    case ColorItemType: return sizeof(ColorItem); break;
    case DrawPointItemType: return sizeof(DrawPointItem); break;
    case DrawLineItemType: return sizeof(DrawLineItem); break;
    case DrawBoxItemType: return sizeof(DrawBoxItem); break;
    case DrawSphereItemType: return sizeof(DrawSphereItem); break;
    case DrawCylinderItemType: return sizeof(DrawCylinderItem); break;
    case DrawCapsuleItemType: return sizeof(DrawCapsuleItem); break;
    case DrawConeItemType: return sizeof(DrawConeItem); break;
    case DrawRingItemType: return sizeof(DrawRingItem); break;
    case DrawCircleItemType: return sizeof(DrawCircleItem); break;
    case DrawRingCircleItemType: return sizeof(DrawRingCircleItem); break;
    case DrawRectItemType: return sizeof(DrawRectItem); break;
    case DrawArrowItemType: return sizeof(DrawArrowItem); break;
    case DrawArrowConeItemType: return sizeof(DrawArrowConeItem); break;
    case DrawAxisItemType: return sizeof(DrawAxisItem); break;
    case DrawGridItemType: return sizeof(DrawGridItem); break;
//        case DrawPointsItemType: return sizeof(DrawPointsItem); break;
//        case DrawLinesItemType: return sizeof(DrawLinesItem); break;
    case PushMatrixItemType: return sizeof(PushMatrixItem); break;
    case PopMatrixItemType: return sizeof(PopMatrixItem); break;
    case IdentityMatrixItemType: return sizeof(IdentityMatrixItem); break;
    case TranslateItemType: return sizeof(TranslateItem); break;
    case RotateItemType: return sizeof(RotateItem); break;
    case RpyRotateItemType: return sizeof(RpyRotateItem); break;
    case TransformItemType: return sizeof(TransformItem); break;
    case ScaleItemType: return sizeof(ScaleItem); break;

    default: return 0; break;
    }

    return 0;
}


void Render3DItem::renderSub(Renderer3D* r, uint8_t* ptr, int itemType)
{
    switch (itemType) {
    case EffectLightItemType:
    {
        EffectLightItem* item = (EffectLightItem*)ptr;
        r->setEffectLight(item->effect);
        break;
    }
    case PointSizeItemType:
    {
        PointSizeItem* item = (PointSizeItem*)ptr;
        r->setPointSize(item->size);
        break;
    }
    case LineWidthItemType:
    {
        LineWidthItem* item = (LineWidthItem*)ptr;
        r->setLineWidth(item->width);
        break;
    }
    case ColorItemType:
    {
        ColorItem* item = (ColorItem*)ptr;
        r->setColor(item->rgba[0], item->rgba[1], item->rgba[2], item->rgba[3]);
        break;
    }
    case DrawPointItemType:
    {
        DrawPointItem* item = (DrawPointItem*)ptr;
        r->drawPoint(item->pos);
        break;
    }
    case DrawLineItemType:
    {
        DrawLineItem* item = (DrawLineItem*)ptr;
        r->drawLine(item->pos1, item->pos2);
        break;
    }
    case DrawBoxItemType:
    {
        DrawBoxItem* item = (DrawBoxItem*)ptr;
        r->drawBox(item->pos, item->R, item->sides);
        break;
    }
    case DrawSphereItemType:
    {
        DrawSphereItem* item = (DrawSphereItem*)ptr;
        r->drawSphere(item->pos, item->R, item->r);
        break;
    }
    case DrawCylinderItemType:
    {
        DrawCylinderItem* item = (DrawCylinderItem*)ptr;
        r->drawCylinder(item->pos, item->R, item->length, item->radius, (bool)item->drawCap);
        break;
    }
    case DrawCapsuleItemType:
    {
        DrawCapsuleItem* item = (DrawCapsuleItem*)ptr;
        r->drawCapsule(item->pos, item->R, item->length, item->radius);
        break;
    }
    case DrawConeItemType:
    {
        DrawConeItem* item = (DrawConeItem*)ptr;
        r->drawCone(item->pos, item->R, item->length, item->radius);
        break;
    }
    case DrawRingItemType:
    {
        DrawRingItem* item = (DrawRingItem*)ptr;
        r->drawRing(item->pos, item->R, item->length, item->outer_radius, item->inner_radius);
        break;
    }
    case DrawCircleItemType:
    {
        DrawCircleItem* item = (DrawCircleItem*)ptr;
        r->drawCircle(item->pos, item->R, item->r);
        break;
    }
    case DrawRingCircleItemType:
    {
        DrawRingCircleItem* item = (DrawRingCircleItem*)ptr;
        r->drawRingCircle(item->pos, item->R, item->outer_radius, item->inner_radius);
        break;
    }
    case DrawRectItemType:
    {
        DrawRectItem* item = (DrawRectItem*)ptr;
        r->drawRect(item->pos, item->R, item->w, item->h);
        break;
    }
    case DrawArrowItemType:
    {
        DrawArrowItem* item = (DrawArrowItem*)ptr;
        r->drawArrow(item->pos1, item->pos2, item->r);
        break;
    }
    case DrawArrowConeItemType:
    {
        DrawArrowConeItem* item = (DrawArrowConeItem*)ptr;
        r->drawArrowCone(item->pos1, item->pos2, item->r);
        break;
    }
    case DrawAxisItemType:
    {
        DrawAxisItem* item = (DrawAxisItem*)ptr;
        r->drawAxis(item->pos, item->R, item->length);
        break;
    }
//    case DrawGridItemType:
//    {
//        DrawGridItem* item = (DrawGridItem*)ptr;
//        r->drawGrid(item->w, item->div);
//        break;
//    }
//        case DrawPointsItemType:
//        break;
//        case DrawLinesItemType:
//        break;
    case PushMatrixItemType:
    {
        r->pushMatrix();
        break;
    }
    case PopMatrixItemType:
    {
        r->popMatrix();
        break;
    }
    case IdentityMatrixItemType:
    {
        r->identiry();
        break;
    }
    case TranslateItemType:
    {
        TranslateItem* item = (TranslateItem*)ptr;
        r->translate(item->t[0], item->t[1], item->t[2]);
        break;
    }
    case RotateItemType:
    {
        RotateItem* item = (RotateItem*)ptr;
        const double* m = item->R;
        Eigen::Matrix3d R;
        R << m[0], m[1], m[2],
             m[3], m[4], m[5],
             m[6], m[7], m[8];
        r->rotate(R);
        break;
    }
    case RpyRotateItemType:
    {
        RpyRotateItem* item = (RpyRotateItem*)ptr;
        r->rotateRpy(item->rpy[0], item->rpy[1], item->rpy[2]);
        break;
    }
    case TransformItemType:
    {
        TransformItem* item = (TransformItem*)ptr;
        const double* m = item->R;
        Eigen::Vector3d p(item->pos[0], item->pos[1], item->pos[2]);
        Eigen::Matrix3d R;
        R << m[0], m[1], m[2],
             m[3], m[4], m[5],
             m[6], m[7], m[8];
        r->transform(p, R);
        break;
    }
    case ScaleItemType:
    {
        ScaleItem* item = (ScaleItem*)ptr;
        r->scale(item->scale[0], item->scale[1], item->scale[2]);
        break;
    }
    default:
        break;
    }
}

} /* namespace ptgl */
