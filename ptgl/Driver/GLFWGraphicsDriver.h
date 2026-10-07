#ifndef PTGL_DRIVER_GLFWGRAPHICSDRIVER_H_
#define PTGL_DRIVER_GLFWGRAPHICSDRIVER_H_

#include <thread>
#include <deque>
#include <functional>
#include "ptgl/GUI/Utf8.h"
#include <mutex>
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include "ptgl/Core/GraphicsDriver.h"
#include "ptgl/Core/GraphicsView.h"
#include "ptgl/Core/Event.h"
#include "ptgl/Core/GLPath.h"
#include <GLFW/glfw3.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include "WindowsIme.h"
#endif

namespace ptgl {

class GLFWGraphicsDriver : public GraphicsDriver {
public:
    enum class ExecutionMode { BackgroundThread, CallingThread };
    // CallingThread keeps GLFW window/event operations on the application's main thread.
    explicit GLFWGraphicsDriver(ExecutionMode mode = ExecutionMode::BackgroundThread);
    virtual ~GLFWGraphicsDriver();

    static void setEnableInitializeGLFW(bool enable);   // default is enable
    static void setEnableTerminateGLFW(bool enable);    // default is enable

    // Read after the loop has stopped (and waitUntilStopped for background mode).
    std::exception_ptr executionError() const { return executionError_; }

    static Key keymap(int key, int scancode);
    static int modKeyMap(int mods);

protected:
    virtual void initialize(GraphicsView* view) override;
    virtual void execute() override;
    virtual void terminate() override;
    void waitUntilStopped() override;

    virtual bool terminated() override;

    virtual void setWindowSize(int width, int height) override;
    virtual void setWindowTitle(const std::string& title) override;
    virtual void setFrameRate(int fps) override;
    void setSwapInterval(int interval) override {
        if (interval < 0) throw std::invalid_argument("Swap interval must be nonnegative");
        swapInterval_ = interval;
        if (glfwWindow_) glfwSwapInterval(interval);
    }

    virtual const std::string& windowTitle() const override { return windowTitle_; }
    virtual int width() const override { return width_; }
    virtual int height() const override { return height_; }
    virtual int frameRate() const override { return frameRate_; }
    float pixelRatio() const override
    {
#ifdef __EMSCRIPTEN__
        return float(emscripten_get_device_pixel_ratio());
#else
        int w = windowWidth_, h = windowHeight_;
        if (glfwWindow_) glfwGetWindowSize(glfwWindow_, &w, &h);
        return w > 0 ? float(width_) / w : 1.0f;
#endif
    }

    std::string clipboardText() const override
    {
        const char *s = glfwWindow_ ? glfwGetClipboardString(glfwWindow_) : nullptr;
        return s ? s : "";
    }
    void setClipboardText(const std::string &s) override
    {
        if (glfwWindow_)
            glfwSetClipboardString(glfwWindow_, s.c_str());
    }
    bool hasTextInputEvents() const override { return true; }
    void setTextInputRect(int x, int y, int w, int h) override
    {
        (void)w;
#ifdef _WIN32
        if (ime_ && glfwWindow_) {
            int sw, sh;
            glfwGetWindowSize(glfwWindow_, &sw, &sh);
            ime_->setCaret(width_ ? x * sw / width_ : x, height_ ? y * sh / height_ : y,
                           height_ ? h * sh / height_ : h);
        }
#else
        (void)x;
        (void)y;
        (void)h;
#endif
    }
    void cancelTextComposition() override
    {
#ifdef _WIN32
        if (ime_)
            ime_->cancel();
#endif
    }
#ifdef _WIN32
    std::unique_ptr<detail::WindowsIme> ime_;
#endif
    static void characterEvent(GLFWwindow *window, unsigned int codepoint);
    static void focusEvent(GLFWwindow *window, int focused);
    std::deque<std::function<void()>> events_;

    // GLFW event
    static void mouseButtonEvent(GLFWwindow *window, int button, int action, int mods);
    static void cursorPosEvent(GLFWwindow *window, double x, double y);
    static void cursorEnterEvent(GLFWwindow *window, int enter);
    static void scrollEvent(GLFWwindow *window, double x, double y);
    static void keyEvent(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void resizeEvent(GLFWwindow *window, int width, int height);
    static void dropEvent(GLFWwindow *window, int count, const char** paths);

    void resizeGL(int x, int y);

    void handleEvents();
    bool initializeWindow();
    void renderFrame();
    void finalizeWindow();

    GLFWwindow* glfwWindow_ = nullptr;

    bool isMousePressed_ = false;
    ptgl::MouseEvent::MouseButton pressedMouseButton_ = ptgl::MouseEvent::MouseButton::NoButton;
    double mouseCursorX_ = 0;
    double mouseCursorY_ = 0;

    std::atomic<bool> requireResizeEvent_;

    std::string windowTitle_;
    int windowWidth_ = 640, windowHeight_ = 480; // GLFW screen coordinates.
    int width_; // Framebuffer pixels, also used for picking and pointer events.
    int height_;

    int frameRate_;
    int swapInterval_ = 0;

    static inline std::atomic<bool> enableInitializeGLFW_ = true;
    static inline std::atomic<bool> enableTerminateGLFW_ = true;
    static inline bool glfwInitialized_ = false;
    static inline std::atomic<int> terminateCount_ = 0;
    static inline std::mutex staticMutex_;
    std::atomic<bool> terminated_{true};
    std::atomic<bool> closeRequested_{false};
    ExecutionMode executionMode_;
    std::exception_ptr executionError_;

    std::unique_ptr<std::thread> thread_;
};

} /* namespace ptgl */

#endif /* PTGL_DRIVER_GLFWGRAPHICSDRIVER_H_ */
