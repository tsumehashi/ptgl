#include "ptgl/Plot/PtglRenderer.h"
#include "ptgl/Core/ShaderProgram.h"
#include "ptgl/Core/Font.h"
#include "thirdparty/Core/nanovg/src/nanovg.h"
#define NANOVG_GLES2
#include "thirdparty/Core/nanovg/src/nanovg_gl.h"
#include <stdexcept>

namespace ptgl::plot {
struct PtglRenderer::Impl {
    ptgl::ShaderProgram program;
    GLuint vbo = 0;
    GLint position = -1, coverage = -1, viewport = -1, color = -1;
    NVGcontext* nvg = nullptr;
    ~Impl() {
        if (nvg) nvgDeleteGLES2(nvg);
        if (vbo) glDeleteBuffers(1, &vbo);
    }
};
PtglRenderer::PtglRenderer() = default;
PtglRenderer::~PtglRenderer() = default;
void PtglRenderer::initialize(int max_vertices) {
    if (impl_) throw std::logic_error("PtglRenderer is already initialized");
    configure(max_vertices);
    auto p = std::make_unique<Impl>();
    auto vs = ptgl::Shader::loadFromSource(ptgl::Shader::VertexShader, R"(
        attribute vec2 position;
        attribute float coverage;
        varying float edgeCoverage;
        uniform vec2 viewport;
        void main() {
            gl_Position = vec4(position.x / viewport.x * 2.0 - 1.0,
                               1.0 - position.y / viewport.y * 2.0, 0.0, 1.0);
            gl_PointSize = 1.0;
            edgeCoverage = coverage;
        })");
    auto fs = ptgl::Shader::loadFromSource(ptgl::Shader::FragmentShader, R"(
        #ifdef GL_ES
        precision mediump float;
        #endif
        uniform vec4 color;
        varying float edgeCoverage;
        void main() { gl_FragColor = vec4(color.rgb, color.a * edgeCoverage); })");
    if (!vs || !fs || !p->program.linkShaders({vs, fs}))
        throw std::runtime_error("ptgl plot shader compilation failed");
    p->position = p->program.attribute("position");
    p->coverage = p->program.attribute("coverage");
    p->viewport = p->program.uniform("viewport");
    p->color = p->program.uniform("color");
    glGenBuffers(1, &p->vbo);
    p->nvg = nvgCreateGLES2(NVG_ANTIALIAS);
    if (!p->vbo || !p->nvg) throw std::runtime_error("ptgl plot resource creation failed");
    if (nvgCreateFontMem(p->nvg, "ptglPlot", ptgl::Font::defaultFontData(), ptgl::Font::defaultFontDataSize(), 0) < 0)
        throw std::runtime_error("ptgl plot font creation failed");
    impl_ = std::move(p);
}
void PtglRenderer::shutdown() { impl_.reset(); }
void PtglRenderer::draw() {
    if (!impl_) throw std::logic_error("PtglRenderer is not initialized");
    auto& p = *impl_;
    const auto& f = drawList();
    glViewport(0, 0, int(f.width), int(f.height));
    glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glDisable(GL_STENCIL_TEST);
    glEnable(GL_BLEND); glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); glLineWidth(1);
    p.program.bind(); p.program.setParameter(p.viewport, f.width, f.height);
    glBindBuffer(GL_ARRAY_BUFFER, p.vbo);
    // Orphan once per frame; transfer only valid bytes, with no per-series uploads.
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(maxVertices()) * sizeof(Vertex2D), nullptr, GL_STREAM_DRAW);
    if (!f.vertices.empty())
        glBufferSubData(GL_ARRAY_BUFFER, 0, GLsizeiptr(f.vertices.size() * sizeof(Vertex2D)), f.vertices.data());
    glEnableVertexAttribArray(GLuint(p.position));
    glVertexAttribPointer(GLuint(p.position), 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), nullptr);
    glEnableVertexAttribArray(GLuint(p.coverage));
    glVertexAttribPointer(GLuint(p.coverage), 1, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), reinterpret_cast<const void*>(offsetof(Vertex2D, coverage)));
    glEnable(GL_SCISSOR_TEST);
    for (const auto& cmd : f.commands) {
        glScissor(int(cmd.clip.x), int(f.height) - int(cmd.clip.y) - int(cmd.clip.h), int(cmd.clip.w), int(cmd.clip.h));
        p.program.setParameter(p.color, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a);
        GLenum mode = GL_LINES;
        switch (cmd.primitive) {
            case Primitive::points: mode = GL_POINTS; break;
            case Primitive::lines: mode = GL_LINES; break;
            case Primitive::line_strip: mode = GL_LINE_STRIP; break;
            case Primitive::triangles: mode = GL_TRIANGLES; break;
            case Primitive::triangle_strip: mode = GL_TRIANGLE_STRIP; break;
        }
        glDrawArrays(mode, GLint(cmd.first), GLsizei(cmd.count));
    }
    glDisableVertexAttribArray(GLuint(p.position));
    glDisableVertexAttribArray(GLuint(p.coverage));
    glBindBuffer(GL_ARRAY_BUFFER, 0); p.program.unbind(); glDisable(GL_SCISSOR_TEST);
    nvgBeginFrame(p.nvg, f.width, f.height, 1);
    nvgFontFace(p.nvg, "ptglPlot"); nvgFontSize(p.nvg, 12 * f.text_scale);
    nvgTextAlign(p.nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    for (const auto& t : f.texts) {
        nvgFillColor(p.nvg, nvgRGBAf(t.color.r, t.color.g, t.color.b, t.color.a));
        nvgText(p.nvg, t.x, t.y, t.value.c_str(), nullptr);
    }
    nvgEndFrame(p.nvg);
}
} // namespace ptgl::plot
