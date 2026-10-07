#pragma once
#include "PlotGraphicsView.h"

namespace ptgl::plot {
struct ViewerOptions {
    std::string title = "ptgl Plot";
    int width = 1280, height = 960;
    int max_vertices = 1024 * 1024;
    unsigned smoke_frames = 0; // Exit after N frames; 0 = interactive.
};
// Called on the display thread before each frame, except while isSourcePaused().
// elapsed is seconds since the previous frame; clamp catch-up work after stalls.
// Takes ownership of the figure and callback. Creates a GLFW window + ptgl
// renderer. Native: blocks until closed, returns 0 on success, 1 on error.
// Web: installs the browser loop and does not return. Capture callback state
// by value; never capture references to local variables in main().
// One viewer at a time; call on the main thread.
int show(Figure figure, Update update = {}, ViewerOptions options = {});
} // namespace ptgl::plot
