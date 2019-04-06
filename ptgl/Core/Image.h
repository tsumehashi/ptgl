#ifndef PTGL_CORE_IMAGE_H_
#define PTGL_CORE_IMAGE_H_

#include <memory>
#include <vector>
#include <string>
#include <fstream>

namespace ptgl {

class Image;
using ImagePtr = std::shared_ptr<Image>;

class ImageLoader;

class Image {
public:
    friend class ImageLoader;

    enum class PixelType {
        Gray,
        GrayAlpha,
        RGB,
        RGBA,
    };

    Image();
    virtual ~Image();

    PixelType pixelType() const { return pixelType_; }
    size_t pixelDataStrideSize() const { return pixelDataStrideSize_; }

    size_t width() const { return width_; }
    size_t height() const { return height_; }

    size_t size1() const { return height_; }
    size_t size2() const { return width_; }

    size_t dataSize() const { return data_.size(); }
    const uint8_t* data() const { return data_.data(); }
    uint8_t* data() { return data_.data(); }

protected:

    PixelType pixelType_ = PixelType::RGB;
    size_t pixelDataStrideSize_ = 0;
    size_t width_ = 0;
    size_t height_ = 0;
    std::vector<uint8_t> data_;
};


class ImageLoader
{
public:
    ImageLoader();
    virtual ~ImageLoader();

    static ImagePtr loadImage(const std::string& filename);
    static ImagePtr loadImage(std::istream& is);

    static ImagePtr loadImageAs(const std::string& filename, Image::PixelType pixelType);
    static ImagePtr loadImageAs(std::istream& is, Image::PixelType pixelType);
};

} /* namespace ptgl */

#endif /* PTGL_CORE_IMAGE_H_ */
