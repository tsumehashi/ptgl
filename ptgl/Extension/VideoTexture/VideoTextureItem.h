#ifndef PTGL_EXTENSION_VIDEOTEXTURE_VIDEOTEXTUREITEM_H_
#define PTGL_EXTENSION_VIDEOTEXTURE_VIDEOTEXTUREITEM_H_

#include <memory>
#include <opencv2/opencv.hpp>

namespace ptgl {
namespace ext {

class VideoTextureItem;
using VideoTextureItemPtr = std::shared_ptr<VideoTextureItem>;

class VideoTextureItem {
public:
    VideoTextureItem() {}
    virtual ~VideoTextureItem() { cap_.release(); }

    bool isValid() const { return cap_.isOpened(); }

    int videoWidth() const { return videoWidth_; }
    int videoHeight() const { return videoHeight_; }
    int videoFrameCount() const { return videoFrameCount_; }
    double videoFps() const { return videoFps_; }

    bool loadVideo(const std::string& filename) {
        bool isOpend = cap_.open(filename);
        if (isOpend) {
            videoWidth_ = cap_.get(CV_CAP_PROP_FRAME_WIDTH);
            videoHeight_ = cap_.get(CV_CAP_PROP_FRAME_HEIGHT);
            videoFrameCount_ = cap_.get(CV_CAP_PROP_FRAME_COUNT);
            videoFps_ = cap_.get(CV_CAP_PROP_FPS);
        }
        return isOpend;
    }

    cv::VideoCapture& videoCapture() { return cap_; }

    static bool getFrame(cv::Mat& cvImage, cv::VideoCapture& cap, int64_t frame) {
        // get current video frame
        if (frame > 1) {
            cap.set(CV_CAP_PROP_POS_FRAMES, frame-1);
        } else {
            cap.set(CV_CAP_PROP_POS_FRAMES, frame);
        }
        cap >> cvImage;

        // get current frame position
        int64_t currentFrame = cap.get(CV_CAP_PROP_POS_FRAMES);

        if (frame == 0) {
            frame = 1;
        }
        const int64_t rdf = std::abs(currentFrame - frame);
        int64_t count = 1;
        while (currentFrame > frame) {
            int64_t df = rdf * count++;
            int64_t nframe = std::max<int64_t>(0, currentFrame - df);
            cap.set(CV_CAP_PROP_POS_FRAMES, nframe);
            cap >> cvImage;
            currentFrame = cap.get(CV_CAP_PROP_POS_FRAMES);
        }

        while (currentFrame < frame) {
            cap >> cvImage;
            currentFrame = cap.get(CV_CAP_PROP_POS_FRAMES);
        }

        // convert bgr -> rgba
        cv::cvtColor(cvImage, cvImage, CV_BGR2RGBA);

        return true;
    }

    static cv::Mat getFrame(cv::VideoCapture& cap, int64_t frame) {
        cv::Mat cvImage;
        getFrame(cvImage, cap, frame);
        return cvImage;
    }

    static bool getFrameFromTime(cv::Mat& cvImage, cv::VideoCapture& cap, double fps, double time) {
        int64_t frame = std::round(time / fps);
        return getFrame(cvImage, cap, frame);
    }

    static cv::Mat getFrameFromTime(cv::VideoCapture& cap, double fps, double time) {
        int64_t frame = std::round(time / fps);
        return getFrame(cap, frame);
    }

    const cv::Mat& getFrame(int64_t frame) {
        getFrame(cvImage_, cap_, frame);
        return cvImage_;
    }

    const cv::Mat& getFrameFromTime(double fps, double time) {
        getFrameFromTime(cvImage_, cap_, fps, time);
        return cvImage_;
    }

    const cv::Mat& image() const {
        return cvImage_;
    }

protected:
    cv::VideoCapture cap_;
    cv::Mat cvImage_;
    int videoWidth_ = 0;
    int videoHeight_ = 0;
    int videoFrameCount_ = 0;
    double videoFps_ = 60;
};

}
} /* namespace ptgl */

#endif /* PTGL_EXTENSION_VIDEOTEXTURE_VIDEOTEXTUREITEM_H_ */
