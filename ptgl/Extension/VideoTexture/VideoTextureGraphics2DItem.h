#ifndef PTGL_EXTENSION_VIDEOTEXTURE_VIDEOTEXTUREGRAPHICS2DITEM_H_
#define PTGL_EXTENSION_VIDEOTEXTURE_VIDEOTEXTUREGRAPHICS2DITEM_H_

#include "ptgl/Core/Graphics2DItem.h"
#include "ptgl/Core/QuickGraphicsItem.h"
#include "ptgl/Core/Renderer3D.h"
#include "ptgl/Core/Renderer2D.h"
#include "ptgl/Core/TextRenderer.h"
#include "VideoTextureItem.h"

namespace ptgl {
namespace ext {

class VideoTextureGraphics2DItem;
using VideoTextureGraphics2DItemPtr = std::shared_ptr<VideoTextureGraphics2DItem>;

class VideoTextureGraphics2DItem : public ptgl::Graphics2DItem {
public:
    VideoTextureGraphics2DItem() {
        videoTextureItem_ = std::make_shared<VideoTextureItem>();

        moveHandle_ = std::make_shared<ptgl::QuickGraphics2DItem>();
        resizeHandle_ = std::make_shared<ptgl::QuickGraphics2DItem>();

        moveHandle_->setRender2DSceneFunction([&](ptgl::Renderer2D* r){
            if (!(videoTextureItem_ && videoTextureItem_->isValid())) {
                return;
            }
            moveHandle_->updatePos();

            r->setEllipseMode(ptgl::Renderer2D::Mode::Center);
            if (moveHandle_->isHoverd() || moveHandle_->isPressed()) {
                r->setFillColor(0, 0.5, 1, 0.8);
            } else {
                r->setFillColor(1, 0.5, 0, 0.4);
            }
            r->setNoStroke();
            r->drawCircle(moveHandle_->x(), moveHandle_->y(), 5);
        });

        moveHandle_->setMousePressEventFunction([&](ptgl::GraphicsItemMouseEvent* e){
            movePressX_ = e->pressX();
            movePressY_ = e->pressY();
        });

        moveHandle_->setMouseMoveEventFunction([&](ptgl::GraphicsItemMouseEvent* e){
            int x = e->moveX() + movePressX_;
            int y = e->moveY() + movePressY_;

            if ((this->x() != x) || (this->y() != y)) {
                reqUpdateVideoTexture_ = true;
            }

            this->setPos(x, y);
        });

        resizeHandle_->setRender2DSceneFunction([&](ptgl::Renderer2D* r){
            if (!(videoTextureItem_ && videoTextureItem_->isValid())) {
                return;
            }
            resizeHandle_->setLocalX(this->width());
            resizeHandle_->setLocalY(this->height());

            resizeHandle_->updatePos();

            r->setRectMode(ptgl::Renderer2D::Mode::Center);
            if (resizeHandle_->isHoverd() || resizeHandle_->isPressed()) {
                r->setFillColor(0, 0.5, 1, 0.8);
            } else {
                r->setFillColor(1, 0.5, 0, 0.4);
            }
            r->setNoStroke();
            r->drawRect(resizeHandle_->x(), resizeHandle_->y(), 8, 8);
        });

        resizeHandle_->setMousePressEventFunction([&](ptgl::GraphicsItemMouseEvent* e){
            resizePressWidth_ = this->width();
            resizePressHeight_ = this->height();
        });

        resizeHandle_->setMouseMoveEventFunction([&](ptgl::GraphicsItemMouseEvent* e){
            int w = e->moveX() + resizePressWidth_;
            int h = e->moveY() + resizePressHeight_;
            if (w < 10) w = 10;
            if (h < 10) h = 10;

            if (enableVideoResize_) {
                int vw = w;
                int vh = h;
                if (enableVideoKeepAspect_) {
                    vw = w;
                    vh = (vw / (double)videoTextureItem_->videoWidth()) * (double)videoTextureItem_->videoHeight();
                } else {
                    vw = w;
                    vh = h;
                }

                if ((this->width() != vw) || (this->height() != vh)) {
                    reqUpdateVideoTexture_ = true;
                }
                setSize(vw, vh);
            }
        });

        this->addChild(moveHandle_);
        this->addChild(resizeHandle_);
    }
    virtual ~VideoTextureGraphics2DItem() {}

    bool isValid() const { return videoTextureItem_ && videoTextureItem_->isValid(); }

    void setEnableVideoResizeHandle(bool on) { enableVideoResizeHandle_ = on; }
    void setEnableVideoResize(bool on) { enableVideoResize_ = on; }
    void setEnableVideoKeepAspect(bool on) { enableVideoKeepAspect_ = on; }

    bool loadVideo(const std::string& filename) {
        VideoTextureItemPtr videoTextureItem = std::make_shared<VideoTextureItem>();
        videoTextureItem->loadVideo(filename);

        if (videoTextureItem->isValid()) {
            setVideoItem(videoTextureItem);
            return true;
        } else {
            return false;
        }
    }

    bool loadVideo(VideoTextureItemPtr videoTextureItem) {
        if (videoTextureItem && videoTextureItem->isValid()) {
            setVideoItem(videoTextureItem);
            return true;
        } else {
            return false;
        }
    }

    VideoTextureItemPtr videoTextureItem() const { return videoTextureItem_; }

    void setVideoFrame(int64_t frame) {
        if (videoFrame_ != frame) {
            reqUpdateVideoTexture_ = true;
        }
        videoFrame_ = frame;
    }

    void setVideoTime(double time) {
        if (videoTextureItem_ && videoTextureItem_->isValid()) {
            setVideoFrame(time * videoTextureItem_->videoFps());
        }
    }

    void setVideoSize(int w, int h) {
        if (enableVideoResize_) {
            int vw = this->width();
            int vh = this->height();
            if (enableVideoKeepAspect_) {
                vw = w;
                double s = (vw / (double)videoTextureItem_->videoWidth());
                vh = (vw / (double)videoTextureItem_->videoWidth()) * vh;
            } else {
                vw = w;
                vh = h;
            }

            if ((this->width() != vw) || (this->height() != vh)) {
                reqUpdateVideoTexture_ = true;
            }
            setSize(vw, vh);
        }
    }

protected:

    void setVideoItem(VideoTextureItemPtr videoItem) {
        videoTextureItem_ = videoItem;
        renderVideoImageName_ = "VideoTexture" + std::to_string((int64_t)videoTextureItem_.get());

        setSize(videoTextureItem_->videoWidth(), videoTextureItem_->videoHeight());
        reqUpdateVideoTexture_ = true;
        videoFrame_ = 1;
        prevGetVideoFrame_ = 0;
    }

    virtual void renderBackground2DScene(ptgl::Renderer2D* r) {
        if (!isEnabled() || !isVisible()) return;
        if (!(videoTextureItem_ && videoTextureItem_->isValid())) {
            return;
        }

        this->updatePos();

        const cv::Mat& image = (prevGetVideoFrame_ != videoFrame_) ? videoTextureItem_->getFrame(videoFrame_) : videoTextureItem_->image();
        prevGetVideoFrame_ = videoFrame_;
        if ((image.cols > 0) && (image.rows > 0) && image.data) {
            int vw = this->width();
            int vh = this->height();

            if (reqUpdateVideoTexture_) {
                r->registerImage(renderVideoImageName_, image.cols, image.rows, image.data, true);
                reqUpdateVideoTexture_ = false;
            }
            r->drawRegisteredImage(renderVideoImageName_, x(), y(), vw, vh);
        }
    }

    bool reqUpdateVideoTexture_ = true;
    bool enableVideoResizeHandle_ = true;
    bool enableVideoResize_ = true;
    bool enableVideoKeepAspect_ = true;
    int64_t videoFrame_ = 1;
    int64_t prevGetVideoFrame_ = 1;

    VideoTextureItemPtr videoTextureItem_;
    std::string renderVideoImageName_;

    ptgl::QuickGraphics2DItemPtr moveHandle_;
    ptgl::QuickGraphics2DItemPtr resizeHandle_;

    int movePressX_ = 0;
    int movePressY_ = 0;
    int resizePressWidth_ = 0;
    int resizePressHeight_ = 0;
};

}
} /* namespace ptgl */

#endif /* PTGL_EXTENSION_VIDEOTEXTURE_VIDEOTEXTUREGRAPHICS2DITEM_H_ */
