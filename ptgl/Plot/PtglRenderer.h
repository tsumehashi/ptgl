#pragma once
#include "ptgl/Plot/Renderer.h"
#include <memory>

namespace ptgl::plot {
// Uses ptgl Shader/ShaderProgram, bundled NanoVG and default font.
// Requires a current OpenGL 2.1 compatibility or GLES/WebGL context.
// Does not clear, swap, read pixels, or run GraphicsView picking passes.
// Modifies GL state; the host must rebind its state after draw().
class PtglRenderer final : public Renderer {
public:
    PtglRenderer();
    ~PtglRenderer() override;
    void initialize(int max_vertices = 1024 * 1024) override;
    void shutdown() override;
    void draw() override;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace ptgl::plot
