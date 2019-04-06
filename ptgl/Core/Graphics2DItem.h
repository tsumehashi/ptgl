#ifndef PTGL_CORE_GRAPHICS2DITEM_H_
#define PTGL_CORE_GRAPHICS2DITEM_H_

#include "GraphicsItem.h"

namespace ptgl {

class Graphics2DItem : public GraphicsItem {
public:
    Graphics2DItem();
    virtual ~Graphics2DItem();

    Graphics2DItem* parent2DItem() const;

    virtual void setWidth(int width);
    virtual void setHeight(int height);
    virtual void setSize(int width, int height);

    virtual void setPos(int x, int y);
    virtual void setX(int x);
    virtual void setY(int y);

    virtual void setLocalPos(int x, int y);
    virtual void setLocalX(int x);
    virtual void setLocalY(int y);

    int width() const { return width_; }
    int height() const { return height_; }

    int x() const { return x_; }
    int y() const { return y_; }

    int localX() const { return localX_; }
    int localY() const { return localY_; }

    virtual void updatePos();

protected:

    int width_ = 0;
    int height_ = 0;

    int x_ = 0;        // global
    int y_ = 0;        // global

    int localX_ = 0;
    int localY_ = 0;
};

} /* namespace ptgl */

#endif /* PTGL_CORE_GRAPHICS2DITEM_H_ */
