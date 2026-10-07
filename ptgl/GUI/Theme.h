#ifndef PTGL_GUI_THEME_H_
#define PTGL_GUI_THEME_H_
#include <array>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace ptgl::gui
{
using Color = std::array<double, 4>;
struct Theme {
    enum class Preset { Classic, Dark, Light };
    Preset preset = Preset::Classic;
    // Match the original DockWidget, PushButton and TextEditWidget palette.
    Color background{0.6, 0.6, 0.6, 0.5}, field{1, 1, 1, 0.9};
    Color titleBackground{0.2, 0.2, 0.2, 0.7}, titleText{1, 1, 1, 1};
    Color text{0, 0, 0, 1}, muted{0.4, 0.4, 0.4, 1};
    Color border{0.3, 0.3, 0.3, 0.8}, accent{0.3, 0.3, 0.3, 1};
    Color checked{1, 0.8, 0.8, 1}, editing{0.8, 0.8, 1, 1};
    Color error{0.85, 0.25, 0.25, 1};
    double scale = 1;
    int fontSize = 14, rowHeight = 28, spacing = 6, padding = 10;
    int pixels(double value) const { return int(std::lround(value * scale)); }
    void validate() const
    {
        if (!std::isfinite(scale) || scale < .5 || scale > 4 || fontSize < 1 || rowHeight < 1 ||
            spacing < 0 || padding < 0)
            throw std::invalid_argument("Invalid GUI theme metrics");
        for (auto color : {background, field, titleBackground, titleText, text, muted, border, accent,
                           checked, editing, error})
            for (double c : color)
                if (!std::isfinite(c) || c < 0 || c > 1)
                    throw std::invalid_argument("Invalid GUI color");
    }
    static Theme dark()
    {
        Theme t;
        t.preset = Preset::Dark;
        t.background = {.10, .12, .16, .98};
        t.field = {.18, .21, .27, 1};
        t.text = {.92, .94, .98, 1};
        t.muted = {.57, .63, .72, 1};
        t.border = {.32, .38, .48, 1};
        t.accent = {.23, .58, .94, 1};
        t.titleBackground = t.background;
        t.titleText = t.text;
        t.checked = t.accent;
        t.editing = t.field;
        return t;
    }
    static Theme light()
    {
        Theme t = dark();
        t.preset = Preset::Light;
        t.background = {.94, .95, .97, 1};
        t.field = {1, 1, 1, 1};
        t.text = {.12, .15, .2, 1};
        t.muted = {.38, .42, .48, 1};
        t.border = {.65, .69, .75, 1};
        t.titleBackground = t.background;
        t.titleText = t.text;
        t.editing = {.9, .94, 1, 1};
        return t;
    }
};
struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
    bool contains(int px, int py) const { return px >= x && py >= y && px < x + w && py < y + h; }
    Rect intersect(Rect b) const
    {
        int left = std::max(x, b.x), top = std::max(y, b.y);
        return {left, top, std::max(0, std::min(x + w, b.x + b.w) - left),
                std::max(0, std::min(y + h, b.y + b.h) - top)};
    }
};
} // namespace ptgl::gui
#endif
