#include "Image.h"
#include <iostream>
#include "thirdparty/Core/nanovg/src/stb_image.h"

namespace ptgl {

Image::Image() {


}

Image::~Image() {

}

// --- ImageLoader ---
ImageLoader::ImageLoader()
{

}

ImageLoader::~ImageLoader()
{

}

ImagePtr ImageLoader::loadImage(const std::string& filename)
{
    std::ifstream ifs(filename, std::ios::in | std::ios::binary);
    return loadImage(ifs);
}

ImagePtr ImageLoader::loadImage(std::istream& is)
{
    if (!is) {
        std::cerr << "error ImageLoader::loadImage stream is null" << std::endl;
        return nullptr;
    }

    is.seekg(0 ,std::ios::end);
    size_t size = is.tellg();
    is.seekg(0, std::ios_base::beg);

    std::vector<uint8_t> rawData;
    rawData.resize(size);
    is.read((char*)rawData.data(), size);

    //     N=#comp     components
    //       1           grey
    //       2           grey, alpha
    //       3           red, green, blue
    //       4           red, green, blue, alpha

    int width = 0;
    int height = 0;
    int comp = 0;
    int req_comp = 0;
    uint8_t* pixels = stbi_load_from_memory(rawData.data(), rawData.size(), &width, &height, &comp, req_comp);
    if (!pixels) {
        std::cerr << "error ImageLoader::loadImage load pixels is null" << std::endl;
        return nullptr;
    }

    std::cout << "loadImage width = " << width << ", height = " << height << ", comp = " << comp << std::endl;

    ImagePtr image = std::make_shared<Image>();
    image->width_ = width;
    image->height_ = height;
    switch (comp) {
    case 1:
        image->pixelType_ = Image::PixelType::Gray;
        image->pixelDataStrideSize_ = 1;
        break;
    case 2:
        image->pixelType_ = Image::PixelType::GrayAlpha;
        image->pixelDataStrideSize_ = 2;
        break;
    case 3:
        image->pixelType_ = Image::PixelType::RGB;
        image->pixelDataStrideSize_ = 3;
        break;
    case 4:
        image->pixelType_ = Image::PixelType::RGBA;
        image->pixelDataStrideSize_ = 4;
        break;
    default:
        // fail
        image->width_ = 0;
        image->height_ = 0;
        image->data_.clear();
    }

    size_t pixelSize = image->pixelDataStrideSize_ * width * height;
    image->data_.assign(pixels, pixels + pixelSize);

    stbi_image_free(pixels);

    return image;
}

ImagePtr ImageLoader::loadImageAs(const std::string& filename, Image::PixelType pixelType)
{
    std::ifstream ifs(filename, std::ios::in | std::ios::binary);
    return loadImageAs(ifs, pixelType);
}

ImagePtr ImageLoader::loadImageAs(std::istream& is, Image::PixelType pixelType)
{
    if (!is) {
        std::cerr << "error ImageLoader::loadImage stream is null" << std::endl;
        return nullptr;
    }

    is.seekg(0 ,std::ios::end);
    size_t size = is.tellg();
    is.seekg(0, std::ios_base::beg);

    std::vector<uint8_t> rawData;
    rawData.resize(size);
    is.read((char*)rawData.data(), size);

    //     N=#comp     components
    //       1           grey
    //       2           grey, alpha
    //       3           red, green, blue
    //       4           red, green, blue, alpha

    int width = 0;
    int height = 0;
    int comp = 0;
    int req_comp = 0;
    switch (pixelType) {
    case Image::PixelType::Gray:      req_comp = 1; break;
    case Image::PixelType::GrayAlpha: req_comp = 2; break;
    case Image::PixelType::RGB:       req_comp = 3; break;
    case Image::PixelType::RGBA:      req_comp = 4; break;
    default:
        req_comp = 0;
    }

    uint8_t* pixels = stbi_load_from_memory(rawData.data(), rawData.size(), &width, &height, &comp, req_comp);
    if (!pixels) {
        std::cerr << "error ImageLoader::loadImage load pixels is null" << std::endl;
        return nullptr;
    }

    std::cout << "loadImage width = " << width << ", height = " << height << ", comp = " << comp << std::endl;

    ImagePtr image = std::make_shared<Image>();
    image->width_ = width;
    image->height_ = height;
    switch (comp) {
    case 1:
        image->pixelType_ = Image::PixelType::Gray;
        image->pixelDataStrideSize_ = 1;
        break;
    case 2:
        image->pixelType_ = Image::PixelType::GrayAlpha;
        image->pixelDataStrideSize_ = 2;
        break;
    case 3:
        image->pixelType_ = Image::PixelType::RGB;
        image->pixelDataStrideSize_ = 3;
        break;
    case 4:
        image->pixelType_ = Image::PixelType::RGBA;
        image->pixelDataStrideSize_ = 4;
        break;
    default:
        // fail
        image->width_ = 0;
        image->height_ = 0;
        image->data_.clear();
    }

    size_t pixelSize = image->pixelDataStrideSize_ * width * height;
    image->data_.assign(pixels, pixels + pixelSize);

    stbi_image_free(pixels);

    return image;
}

} /* namespace ptgl */
