#include "Graphics2DItem.h"

namespace ptgl {

Graphics2DItem::Graphics2DItem()
{

}

Graphics2DItem::~Graphics2DItem()
{

}

Graphics2DItem* Graphics2DItem::parent2DItem() const
{
    return dynamic_cast<Graphics2DItem*>(parentItem());
}

void Graphics2DItem::setWidth(int width)
{
    setSize(width, height_);
}

void Graphics2DItem::setHeight(int height)
{
    setSize(width_, height);
}

void Graphics2DItem::setSize(int width, int height)
{
    width_ = width;
    height_ = height;
}

void Graphics2DItem::setPos(int x, int y)
{
    x_ = x;
    y_ = y;
}

void Graphics2DItem::setX(int x)
{
    x_ = x;
}

void Graphics2DItem::setY(int y)
{
    y_ = y;
}

void Graphics2DItem::setLocalPos(int x, int y)
{
    localX_ = x;
    localY_ = y;
}

void Graphics2DItem::setLocalX(int x)
{
    localX_ = x;
}

void Graphics2DItem::setLocalY(int y)
{
    localY_ = y;
}

void Graphics2DItem::updatePos()
{
    if (auto parent = parent2DItem()) {
        x_ = parent->x() + localX();
        y_ = parent->y() + localY();
    }
}

} /* namespace ptgl */
