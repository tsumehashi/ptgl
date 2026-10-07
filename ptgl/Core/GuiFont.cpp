#include "Renderer2D.h"
#include "thirdparty/Core/nanovg/src/nanovg.h"
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iterator>
namespace ptgl
{
float Renderer2D::measureText(const std::string &text) const
{
    if (!renderNvgContext_)
        return textWidth(int(text.size()));
    nvgFontFace(renderNvgContext_, fontName_.c_str());
    nvgFontSize(renderNvgContext_, textSize_);
    return nvgTextBounds(renderNvgContext_, 0, 0, text.c_str(), nullptr, nullptr);
}
bool Renderer2D::loadFont(const std::string &path)
{
    if (!renderNvgContext_)
        return false;
    std::ifstream f(std::filesystem::u8path(path), std::ios::binary);
    if (!f)
        return false;
    std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
    if (bytes.empty())
        return false;
    auto memory = std::make_shared<std::vector<unsigned char>>(std::move(bytes));
    std::string name = "gui-font-" + std::to_string(fontData_.size());
    int font = nvgCreateFontMem(renderNvgContext_, name.c_str(), memory->data(), int(memory->size()), 0);
    if (font < 0)
        return false;
    fontData_.push_back(std::move(memory));
    fontName_ = name;
    textOffsetCacheMap_.clear();
    return true;
}
} // namespace ptgl
