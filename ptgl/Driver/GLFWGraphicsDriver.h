#ifndef PTGL_DRIVER_GLFWGRAPHICSDRIVER_H_
#define PTGL_DRIVER_GLFWGRAPHICSDRIVER_H_

#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <iostream>
#include "ptgl/Core/GraphicsDriver.h"
#include "ptgl/Core/GraphicsView.h"
#include "ptgl/Core/Event.h"
#include "ptgl/Core/GLPath.h"
#include <GLFW/glfw3.h>

namespace ptgl {

class GLFWGraphicsDriver : public GraphicsDriver {
public:
    GLFWGraphicsDriver();
    virtual ~GLFWGraphicsDriver();

    static void setEnableInitializeGLFW(bool enable);   // default is enable
    static void setEnableTerminateGLFW(bool enable);    // default is enable

    static Key keymap(int key, int scancode);
    static int modKeyMap(int mods);

protected:
    virtual void initialize(GraphicsView* view) override;
    virtual void execute() override;
    virtual void terminate() override;

    virtual bool terminated() override;

    virtual void setWindowSize(int width, int height) override;
    virtual void setWindowTitle(const std::string& title) override;
    virtual void setFrameRate(int fps) override;

    virtual const std::string& windowTitle() const override { return windowTitle_; }
    virtual int width() const override { return width_; }
    virtual int height() const override { return height_; }
    virtual int frameRate() const override { return frameRate_; }

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

    GLFWwindow* glfwWindow_ = nullptr;

    bool isMousePressed_ = false;
    ptgl::MouseEvent::MouseButton pressedMouseButton_ = ptgl::MouseEvent::MouseButton::NoButton;
    double mouseCursorX_ = 0;
    double mouseCursorY_ = 0;

    std::atomic<bool> requireResizeEvent_;
    std::atomic<bool> requireMousePressEvent_;
    std::atomic<bool> requireMouseMoveEvent_;
    std::atomic<bool> requireMouseReleaseEvent_;
    std::atomic<bool> requireMouseWheelEvent_;
    std::atomic<bool> requireMouseKeyPressEvent_;
    std::atomic<bool> requireDropEvent_;

    std::string windowTitle_;
    int width_;
    int height_;

    int frameRate_;

    static inline std::atomic<bool> enableInitializeGLFW_ = true;
    static inline std::atomic<bool> enableTerminateGLFW_ = true;
    static inline std::once_flag init_once_flag_;
    static inline std::atomic<int> terminateCount_ = 0;
    static inline std::mutex staticMutex_;
    static inline std::atomic<bool> terminated_ = false;

    std::unique_ptr<std::thread> thread_;
};

GLFWGraphicsDriver::GLFWGraphicsDriver()
{
    width_ = 640;
    height_ = 480;

    windowTitle_ = "GraphicsView";

    requireResizeEvent_ = false;
    requireMousePressEvent_ = false;
    requireMouseMoveEvent_ = false;
    requireMouseReleaseEvent_ = false;
    requireMouseWheelEvent_ = false;
    requireMouseKeyPressEvent_ = false;
    requireDropEvent_ = false;

    isMousePressed_ = false;
    pressedMouseButton_ = ptgl::MouseEvent::MouseButton::NoButton;
    mouseCursorX_ = 0;
    mouseCursorY_ = 0;

    frameRate_ = 60;

    ++terminateCount_;
}

GLFWGraphicsDriver::~GLFWGraphicsDriver()
{
    terminate();

    try {
        if (thread_ && thread_->joinable()) {
            thread_->join();
        }
    } catch (...) {

    }

    if (glfwWindow_) {
        glfwDestroyWindow(glfwWindow_);
        glfwWindow_ = nullptr;
        terminated_ = true;
    }

    --terminateCount_;
    if (terminateCount_ == 0) {
        if (enableTerminateGLFW_.load()) {
            glfwTerminate();
        }
    }
}

void GLFWGraphicsDriver::setEnableInitializeGLFW(bool enable)
{
    enableInitializeGLFW_ = enable;
}

void GLFWGraphicsDriver::setEnableTerminateGLFW(bool enable)
{
    enableTerminateGLFW_ = enable;
}

void GLFWGraphicsDriver::setWindowSize(int width, int height)
{
    width_ = width;
    height_ = height;

    if (glfwWindow_) {
        glfwSetWindowSize(glfwWindow_, width_, height_);
    }
}

void GLFWGraphicsDriver::setWindowTitle(const std::string& title)
{
    windowTitle_ = title;

    if (glfwWindow_) {
        glfwSetWindowTitle(glfwWindow_, windowTitle_.c_str());
    }
}

void GLFWGraphicsDriver::setFrameRate(int fps)
{
    frameRate_ = fps;
}

void GLFWGraphicsDriver::initialize(ptgl::GraphicsView* view)
{
    if (!view) return;

    std::unique_lock<std::mutex> lock(staticMutex_);

    std::call_once(init_once_flag_, [&](){
        if (enableInitializeGLFW_.load()) {
            std::cerr << "glfwInit" << std::endl;
            glfwInit();
        }
    });

    GraphicsDriver::initialize(view);
}

void GLFWGraphicsDriver::execute()
{
    if (!graphicsView()) return;

    std::unique_lock<std::mutex> lock(staticMutex_);

    // if not exec initializ glfwInit
    std::call_once(init_once_flag_, [&](){
        if (enableInitializeGLFW_.load()) {
            std::cerr << "glfwInit" << std::endl;
            glfwInit();
        }
    });

    std::atomic<bool> waitInitEnd(true);

    thread_ = std::move(std::unique_ptr<std::thread>(new std::thread([&](){

#ifndef PTGL_DISABLE_GLES
        // set OpenGL ES 2.0
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif    // PTGL_DISABLE_GLES

        glfwWindowHint(GLFW_SAMPLES, 4);

        glfwWindow_ = glfwCreateWindow(width_, height_, windowTitle_.c_str(), NULL, NULL);

        glfwMakeContextCurrent(glfwWindow_);
        glfwSwapInterval(0);

        // init GLEW
        if(glewInit() != GLEW_OK) {
            std::cerr << "Could not init glew." << std::endl;
//            return;
        }

        // print out some info about the graphics drivers
        std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
        std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
        std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
        std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

        // set this pointer
        glfwSetWindowUserPointer(glfwWindow_, this);

        // set callback
        glfwSetMouseButtonCallback(glfwWindow_, mouseButtonEvent);
        glfwSetCursorEnterCallback(glfwWindow_, cursorEnterEvent);
        glfwSetCursorPosCallback(glfwWindow_, cursorPosEvent);
        glfwSetScrollCallback(glfwWindow_, scrollEvent);
        glfwSetKeyCallback(glfwWindow_, keyEvent);
        glfwSetDropCallback(glfwWindow_, dropEvent);

        glfwSetWindowSizeCallback(glfwWindow_, resizeEvent);

        // finish initialize
        waitInitEnd = false;

        resizeGL(width_, height_);

        executeGraphicsViewInitializeEvent();

        while (!glfwWindowShouldClose(glfwWindow_)) {

            auto current_time = std::chrono::system_clock::now();

            // execute prev process
            executeGraphicsViewPrevProcessEvent();

            // execute prev event process
            executeGraphicsViewPrevEventProcessEvent();

            // handle events
            handleEvents();

            // execute prev event process
            executeGraphicsViewPostEventProcessEvent();

            // render
            executeGraphicsViewRenderEvent();

            // execute post process
            executeGraphicsViewPostProcessEvent();

            // swap front and back buffers
            glfwSwapBuffers(glfwWindow_);

            // poll for and process events
            glfwPollEvents();

            // fps
            if (frameRate() > 0) {
                int64_t msTimeStep = std::round(1000.0 / frameRate());
                auto sleep_time = current_time + std::chrono::milliseconds(msTimeStep);
                std::this_thread::sleep_until(sleep_time);
            }
        }

        glfwDestroyWindow(glfwWindow_);
        glfwWindow_ = nullptr;
        terminated_ = true;
    })));

    // wait for GLFW initialized
    while (waitInitEnd.load()) {
        ;    // wait
    }
}

void GLFWGraphicsDriver::terminate() {
    std::unique_lock<std::mutex> lock(staticMutex_);
    if (glfwWindow_) {
        glfwSetWindowShouldClose(glfwWindow_, GL_TRUE);
    }
}

bool GLFWGraphicsDriver::terminated() {
    return !glfwWindow_ || terminated_.load();
}

void GLFWGraphicsDriver::resizeGL(int x, int y) {
    width_ = x;
    height_ = y;
    requireResizeEvent_ = true;
}

void GLFWGraphicsDriver::mouseButtonEvent(GLFWwindow* window, int button, int action, int mods)
{
    GLFWGraphicsDriver* driver = static_cast<GLFWGraphicsDriver*>(glfwGetWindowUserPointer(window));
    if (!driver) return;

    ptgl::MouseEvent::MouseButton btn = ptgl::MouseEvent::MouseButton::NoButton;
    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT: btn = ptgl::MouseEvent::MouseButton::LeftButton; break;
    case GLFW_MOUSE_BUTTON_MIDDLE: btn = ptgl::MouseEvent::MouseButton::MiddleButton; break;
    case GLFW_MOUSE_BUTTON_RIGHT: btn = ptgl::MouseEvent::MouseButton::RightButton; break;
    default:
        btn = ptgl::MouseEvent::MouseButton::NoButton;
        break;
    }

    auto event = driver->getGraphicsViewMouseEvent();

    const int modifyKey = modKeyMap(mods);

    if (action == GLFW_PRESS) {
        driver->isMousePressed_ = true;
        driver->pressedMouseButton_ = btn;
        event->setPressEvent((int)driver->mouseCursorX_, (int)driver->mouseCursorY_, btn, modifyKey);

        driver->requireMousePressEvent_ = true;
    } else if (action == GLFW_RELEASE) {
        driver->isMousePressed_ = false;
        driver->pressedMouseButton_ = ptgl::MouseEvent::MouseButton::NoButton;
        event->setReleaseEvent((int)driver->mouseCursorX_, (int)driver->mouseCursorY_, btn, modifyKey);

        driver->requireMouseReleaseEvent_ = true;
    }
}

void GLFWGraphicsDriver::cursorPosEvent(GLFWwindow* window, double x, double y)
{
    GLFWGraphicsDriver* driver = static_cast<GLFWGraphicsDriver*>(glfwGetWindowUserPointer(window));
    if (!driver) return;

    driver->mouseCursorX_ = x;
    driver->mouseCursorY_ = y;

    auto event = driver->getGraphicsViewMouseEvent();

    event->setMoveEvent((int)driver->mouseCursorX_, (int)driver->mouseCursorY_, driver->pressedMouseButton_);

    driver->requireMouseMoveEvent_ = true;
}

void GLFWGraphicsDriver::cursorEnterEvent(GLFWwindow* window, int enter)
{
    GLFWGraphicsDriver* driver = static_cast<GLFWGraphicsDriver*>(glfwGetWindowUserPointer(window));
    if (!driver) return;

    (void)window;
    (void)enter;
}

void GLFWGraphicsDriver::scrollEvent(GLFWwindow* window, double x, double y)
{
    GLFWGraphicsDriver* driver = static_cast<GLFWGraphicsDriver*>(glfwGetWindowUserPointer(window));
    if (!driver) return;

    auto event = driver->getGraphicsViewWheelEvent();

    int delta = 80*y;
    event->setWheelEvent(x, y, delta);

    driver->requireMouseWheelEvent_ = true;
}

void GLFWGraphicsDriver::keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    GLFWGraphicsDriver* driver = static_cast<GLFWGraphicsDriver*>(glfwGetWindowUserPointer(window));
    if (!driver) return;

    (void)scancode;

    const int modifyKey = modKeyMap(mods);
    const ptgl::Key ptglKey = GLFWGraphicsDriver::keymap(key, scancode);

    if (action == GLFW_PRESS) {
        auto event = driver->getGraphicsViewKeyEvent();
        event->setKeyPressEvent(ptglKey, ptgl::KeyEvent::KeyAction::KeyPress, modifyKey);
        driver->requireMouseKeyPressEvent_ = true;
    } else if (action == GLFW_RELEASE) {
#if 0
        auto event = driver->getGraphicsViewKeyEvent();
        event->setKeyPressEvent(ptglKey, ptgl::KeyEvent::KeyAction::KeyRelease, modifyKey);
        driver->requireMouseKeyPressEvent_ = true;
#endif
    } else if (action == GLFW_REPEAT) {
        auto event = driver->getGraphicsViewKeyEvent();
        event->setKeyPressEvent(ptglKey, ptgl::KeyEvent::KeyAction::KeyRepeat, modifyKey);
        driver->requireMouseKeyPressEvent_ = true;
    }

}

void GLFWGraphicsDriver::resizeEvent(GLFWwindow* window, int width, int height) {
    GLFWGraphicsDriver* driver = static_cast<GLFWGraphicsDriver*>(glfwGetWindowUserPointer(window));
    if (!driver) return;

    driver->resizeGL(width, height);
}

void GLFWGraphicsDriver::dropEvent(GLFWwindow *window, int count, const char** paths)
{
    GLFWGraphicsDriver* driver = static_cast<GLFWGraphicsDriver*>(glfwGetWindowUserPointer(window));
    if (!driver) return;

    auto event = driver->getGraphicsViewDropEvent();
    std::vector<std::string> dropPaths;
    for (int i = 0; i < count; ++i) {
        dropPaths.push_back(paths[i]);
    }
    event->setDropEvent(dropPaths);
    driver->requireDropEvent_ = true;
}

void GLFWGraphicsDriver::handleEvents()
{
    // handle resize event
    if (requireResizeEvent_) {
        executeGraphicsViewResizeEvent(width_, height_);
        requireResizeEvent_ = false;
    }

    if (requireMousePressEvent_) {
        executeGraphicsViewMousePressEvent(getGraphicsViewMouseEvent());
        requireMousePressEvent_ = false;
    }

    if (requireMouseMoveEvent_) {
        executeGraphicsViewMouseMoveEvent(getGraphicsViewMouseEvent());
        requireMouseMoveEvent_ = false;
    }

    if (requireMouseReleaseEvent_) {
        executeGraphicsViewMouseReleaseEvent(getGraphicsViewMouseEvent());
        requireMouseReleaseEvent_ = false;
    }

    if (requireMouseWheelEvent_) {
        executeGraphicsViewWheelEvent(getGraphicsViewWheelEvent());
        requireMouseWheelEvent_ = false;
    }

    if (requireMouseKeyPressEvent_) {
        executeGraphicsViewKeyPressEvent(getGraphicsViewKeyEvent());
        requireMouseKeyPressEvent_ = false;
    }

    if (requireDropEvent_) {
        executeGraphicsViewDropEvent(getGraphicsViewDropEvent());
        requireDropEvent_ = false;
    }
}

Key GLFWGraphicsDriver::keymap(int key, int scancode)
{
    // todo
    // jis key
    switch (scancode) {
    case 20: return Key::Key_Minus; break;
    case 21: return Key::Key_Caret; break;
    case 132: return Key::Key_BackSlash_VerticalBar; break;
    case 34: return Key::Key_At; break;
    case 35: return Key::Key_LeftBracket; break;
    case 47: return Key::Key_Semicolon; break;
    case 48: return Key::Key_Colon; break;
    case 51: return Key::Key_RightBracket; break;
    case 59: return Key::Key_Comma; break;
    case 60: return Key::Key_Period; break;
    case 61: return Key::Key_Slash; break;
    case 97: return Key::Key_BackSlash; break;
    default:
        break;
    }

    switch (key) {
    case GLFW_KEY_SPACE: return Key::Key_Space; break;
    case GLFW_KEY_APOSTROPHE: return Key::Key_Unknown; break;
    case GLFW_KEY_COMMA: return Key::Key_Comma; break;
    case GLFW_KEY_MINUS: return Key::Key_Minus; break;
    case GLFW_KEY_PERIOD: return Key::Key_Period; break;
    case GLFW_KEY_SLASH: return Key::Key_Slash; break;
    case GLFW_KEY_0: return Key::Key_0; break;
    case GLFW_KEY_1: return Key::Key_1; break;
    case GLFW_KEY_2: return Key::Key_2; break;
    case GLFW_KEY_3: return Key::Key_3; break;
    case GLFW_KEY_4: return Key::Key_4; break;
    case GLFW_KEY_5: return Key::Key_5; break;
    case GLFW_KEY_6: return Key::Key_6; break;
    case GLFW_KEY_7: return Key::Key_7; break;
    case GLFW_KEY_8: return Key::Key_8; break;
    case GLFW_KEY_9: return Key::Key_9; break;
    case GLFW_KEY_SEMICOLON: return Key::Key_Semicolon; break;
    case GLFW_KEY_EQUAL: return Key::Key_Equal; break;
    case GLFW_KEY_A: return Key::Key_A; break;
    case GLFW_KEY_B: return Key::Key_B; break;
    case GLFW_KEY_C: return Key::Key_C; break;
    case GLFW_KEY_D: return Key::Key_D; break;
    case GLFW_KEY_E: return Key::Key_E; break;
    case GLFW_KEY_F: return Key::Key_F; break;
    case GLFW_KEY_G: return Key::Key_G; break;
    case GLFW_KEY_H: return Key::Key_H; break;
    case GLFW_KEY_I: return Key::Key_I; break;
    case GLFW_KEY_J: return Key::Key_J; break;
    case GLFW_KEY_K: return Key::Key_K; break;
    case GLFW_KEY_L: return Key::Key_L; break;
    case GLFW_KEY_M: return Key::Key_M; break;
    case GLFW_KEY_N: return Key::Key_N; break;
    case GLFW_KEY_O: return Key::Key_O; break;
    case GLFW_KEY_P: return Key::Key_P; break;
    case GLFW_KEY_Q: return Key::Key_Q; break;
    case GLFW_KEY_R: return Key::Key_R; break;
    case GLFW_KEY_S: return Key::Key_S; break;
    case GLFW_KEY_T: return Key::Key_T; break;
    case GLFW_KEY_U: return Key::Key_U; break;
    case GLFW_KEY_V: return Key::Key_V; break;
    case GLFW_KEY_W: return Key::Key_W; break;
    case GLFW_KEY_X: return Key::Key_X; break;
    case GLFW_KEY_Y: return Key::Key_Y; break;
    case GLFW_KEY_Z: return Key::Key_Z; break;
    case GLFW_KEY_LEFT_BRACKET:  return Key::Key_LeftBracket; break;    // "["
    case GLFW_KEY_BACKSLASH:     return Key::Key_BackSlash; break;        // "\"
    case GLFW_KEY_RIGHT_BRACKET: return Key::Key_RightBracket; break;    // "]"
    case GLFW_KEY_ESCAPE: return Key::Key_Escape; break;
    case GLFW_KEY_ENTER: return Key::Key_Enter; break;
    case GLFW_KEY_TAB: return Key::Key_Tab; break;
    case GLFW_KEY_INSERT: return Key::Key_Insert; break;
    case GLFW_KEY_DELETE: return Key::Key_Delete; break;
    case GLFW_KEY_BACKSPACE: return Key::Key_BackSpace; break;
    case GLFW_KEY_RIGHT: return Key::Key_Right; break;
    case GLFW_KEY_LEFT: return Key::Key_Left; break;
    case GLFW_KEY_DOWN: return Key::Key_Down; break;
    case GLFW_KEY_UP: return Key::Key_Up; break;
    case GLFW_KEY_PAGE_DOWN: return Key::Key_PageDown; break;
    case GLFW_KEY_PAGE_UP: return Key::Key_PageUp; break;
    case GLFW_KEY_HOME: return Key::Key_Home; break;
    case GLFW_KEY_END: return Key::Key_End; break;

    case GLFW_KEY_F1: return Key::Key_F1; break;
    case GLFW_KEY_F2: return Key::Key_F2; break;
    case GLFW_KEY_F3: return Key::Key_F3; break;
    case GLFW_KEY_F4: return Key::Key_F4; break;
    case GLFW_KEY_F5: return Key::Key_F5; break;
    case GLFW_KEY_F6: return Key::Key_F6; break;
    case GLFW_KEY_F7: return Key::Key_F7; break;
    case GLFW_KEY_F8: return Key::Key_F8; break;
    case GLFW_KEY_F9: return Key::Key_F9; break;
    case GLFW_KEY_F10: return Key::Key_F10; break;
    case GLFW_KEY_F11: return Key::Key_F11; break;
    case GLFW_KEY_F12: return Key::Key_F12; break;
    case GLFW_KEY_LEFT_SHIFT:
    case GLFW_KEY_RIGHT_SHIFT: return Key::Key_Shift; break;
    case GLFW_KEY_LEFT_CONTROL:
    case GLFW_KEY_RIGHT_CONTROL: return Key::Key_Control; break;
    case GLFW_KEY_LEFT_ALT:
    case GLFW_KEY_RIGHT_ALT: return Key::Key_Alt; break;
    default:
        return Key::Key_Unknown;
        break;
    }

    return Key::Key_Unknown;
}

int GLFWGraphicsDriver::modKeyMap(int mods)
{
    int modifyKey = ModifierKey_None;
    if ((mods & GLFW_MOD_SHIFT )   != 0) { modifyKey |= ModifierKey_Shift;   }
    if ((mods & GLFW_MOD_CONTROL ) != 0) { modifyKey |= ModifierKey_Control; }
    if ((mods & GLFW_MOD_ALT )     != 0) { modifyKey |= ModifierKey_Alt;     }
    if ((mods & GLFW_MOD_SUPER )   != 0) { modifyKey |= ModifierKey_Super;   }
    return modifyKey;
}

} /* namespace ptgl */

#endif /* PTGL_DRIVER_GLFWGRAPHICSDRIVER_H_ */
