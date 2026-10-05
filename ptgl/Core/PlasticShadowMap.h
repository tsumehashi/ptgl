#ifndef PTGL_CORE_PLASTICSHADOWMAP_H_
#define PTGL_CORE_PLASTICSHADOWMAP_H_

#include <functional>
#include "PlasticGraphicsView.h"

namespace ptgl {
namespace detail {

// GL resources stay on the rendering thread. release() must run while the
// owning context is current (GraphicsDriver's finalize event handles this).
class PlasticShadowMap {
public:
    PlasticShadowMap() = default;
    PlasticShadowMap(const PlasticShadowMap&) = delete;
    PlasticShadowMap& operator=(const PlasticShadowMap&) = delete;
    void initialize();
    void release();
    bool prepare(const PlasticGraphicsView::ShadowSettings& settings, const PlasticLighting& lighting);
    void render(const std::function<void()>& draw);
    void bind(const ShaderProgramPtr& program, bool enabled);
    void unbind();

    ShaderProgramPtr program() const { return program_; }
    const Eigen::Matrix4d& view() const { return view_; }
    const Eigen::Matrix4d& projection() const { return projection_; }

private:
    bool allocate(int size);
    void releaseMap();

    ShaderProgramPtr program_;
    GLuint framebuffer_ = 0, texture_ = 0, depth_ = 0, whiteTexture_ = 0;
    GLenum framebufferTarget_ = GL_FRAMEBUFFER;
    GLenum framebufferBinding_ = GL_FRAMEBUFFER_BINDING;
    bool es_ = false;
    bool samplerObjects_ = false;
    int size_ = 0, failedSize_ = 0;
    GLint savedTexture_ = 0;
    GLint savedSampler_ = 0;
    bool bound_ = false;
    PlasticGraphicsView::ShadowSettings settings_;
    Eigen::Matrix4d view_ = Eigen::Matrix4d::Identity();
    Eigen::Matrix4d projection_ = Eigen::Matrix4d::Identity();
};

} // namespace detail
} // namespace ptgl

#endif
