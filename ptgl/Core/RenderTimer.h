#ifndef PTGL_CORE_RENDERTIMER_H_
#define PTGL_CORE_RENDERTIMER_H_
#include "SceneRenderSupport.h"
#include "RenderSettings.h"
namespace ptgl
{
namespace detail
{
#ifdef __EMSCRIPTEN__
// WebGL has no desktop timer-query entry points. CPU statistics remain available.
class RenderTimer
{
  public:
    void begin(RenderStatistics &) {}
    void end() {}
    void release() {}
};
#else
// Desktop asynchronous timers. Never wait for GPU completion, and do not nest
// a timer query owned by the host application. ES reports CPU timings only.
class RenderTimer
{
    struct Sample {
        GLuint query = 0;
        bool pending = false;
        std::uint64_t frame = 0;
    };
    Sample samples_[3];
    Sample *active_ = nullptr;

  public:
    void begin(RenderStatistics &stats)
    {
        GLFeatures features;
        if (features.es || !(GLEW_VERSION_3_3 || GLEW_ARB_timer_query) || !glGetQueryObjectui64v)
            return;
        for (auto &s : samples_)
            if (s.pending) {
                GLint ready = 0;
                glGetQueryObjectiv(s.query, GL_QUERY_RESULT_AVAILABLE, &ready);
                if (ready) {
                    GLuint64 ns = 0;
                    glGetQueryObjectui64v(s.query, GL_QUERY_RESULT, &ns);
                    if (s.frame >= stats.edgeGpuSampleFrame) {
                        stats.edgeGpuMilliseconds = double(ns) / 1e6;
                        stats.edgeGpuSampleFrame = s.frame;
                    }
                    s.pending = false;
                }
            }
        GLint current = 0;
        glGetQueryiv(GL_TIME_ELAPSED, GL_CURRENT_QUERY, &current);
        if (current)
            return;
        for (auto &s : samples_)
            if (!s.pending) {
                if (!s.query)
                    glGenQueries(1, &s.query);
                s.frame = stats.frame;
                active_ = &s;
                glBeginQuery(GL_TIME_ELAPSED, s.query);
                break;
            }
    }
    void end()
    {
        if (active_) {
            glEndQuery(GL_TIME_ELAPSED);
            active_->pending = true;
            active_ = nullptr;
        }
    }
    void release()
    {
        end();
        for (auto &s : samples_) {
            if (s.query)
                glDeleteQueries(1, &s.query);
            s = {};
        }
    }
};
#endif
} // namespace detail
} // namespace ptgl
#endif
