#include "GLFWGraphicsDriver.h"

namespace ptgl {

GLFWGraphicsDriver::GLFWGraphicsDriver(ExecutionMode mode) : executionMode_(mode)
{
    width_ = 640;
    height_ = 480;

    windowTitle_ = "GraphicsView";

    requireResizeEvent_ = false;

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

    waitUntilStopped();

    if (glfwWindow_) {
        glfwDestroyWindow(glfwWindow_);
        glfwWindow_ = nullptr;
        terminated_ = true;
    }

    std::lock_guard<std::mutex> lock(staticMutex_);
    --terminateCount_;
    if (terminateCount_ == 0 && enableTerminateGLFW_.load() && glfwInitialized_) {
        glfwTerminate();
        glfwInitialized_ = false;
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
    windowWidth_ = width;
    windowHeight_ = height;
    if (glfwWindow_) {
        glfwSetWindowSize(glfwWindow_, width, height);
        int w, h; glfwGetFramebufferSize(glfwWindow_, &w, &h);
        resizeGL(w, h);
    } else { width_ = width; height_ = height; }
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

    if (enableInitializeGLFW_.load() && !glfwInitialized_) {
        if (!glfwInit()) throw std::runtime_error("Could not initialize GLFW");
        glfwInitialized_ = true;
    }

    GraphicsDriver::initialize(view);
}

bool GLFWGraphicsDriver::initializeWindow()
{
#ifdef __EMSCRIPTEN__
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#elif !defined(PTGL_DISABLE_GLES)
    // set OpenGL ES 2.0
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
#endif    // PTGL_DISABLE_GLES

#ifdef __EMSCRIPTEN__
    // WebGL cannot disable MSAA for the ID/depth passes on the default buffer.
    glfwWindowHint(GLFW_SAMPLES, 0);
#else
    glfwWindowHint(GLFW_SAMPLES, 4);
#endif

    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindow_ = glfwCreateWindow(windowWidth_, windowHeight_, windowTitle_.c_str(), NULL, NULL);

    if (!glfwWindow_) {
        std::cerr << "Could not create the graphics window/context." << std::endl;
        terminated_ = true;
        throw std::runtime_error("Could not create the graphics window/context");
    }
    terminated_ = false;
    glfwMakeContextCurrent(glfwWindow_);
    glfwSwapInterval(swapInterval_);

    // init GLEW
    if (glewInit() != GLEW_OK) throw std::runtime_error("Could not initialize GLEW");
    // GLEW can probe unsupported extensions on a valid context.
    while (glGetError() != GL_NO_ERROR) {}

    // print out some info about the graphics drivers
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    // set this pointer
    glfwSetWindowUserPointer(glfwWindow_, this);
#ifdef _WIN32
    ime_ = std::make_unique<detail::WindowsIme>(
        glfwGetWin32Window(glfwWindow_),
        [this](std::string text) {
            events_.push_back([this, text] { graphicsView()->textInput(text); });
        },
        [this](std::string text, int cursor) {
            events_.push_back([this, text, cursor] { graphicsView()->textComposition(text, cursor); });
        });
#endif

    // set callback
    glfwSetMouseButtonCallback(glfwWindow_, mouseButtonEvent);
    glfwSetCursorEnterCallback(glfwWindow_, cursorEnterEvent);
    glfwSetCursorPosCallback(glfwWindow_, cursorPosEvent);
    glfwSetScrollCallback(glfwWindow_, scrollEvent);
    glfwSetKeyCallback(glfwWindow_, keyEvent);
    glfwSetCharCallback(glfwWindow_, characterEvent);
    glfwSetWindowFocusCallback(glfwWindow_, focusEvent);
    glfwSetDropCallback(glfwWindow_, dropEvent);

    glfwSetFramebufferSizeCallback(glfwWindow_, resizeEvent);

    glfwGetFramebufferSize(glfwWindow_, &width_, &height_);
    resizeGL(width_, height_);

    return true;
}

void GLFWGraphicsDriver::renderFrame()
{
#ifdef __EMSCRIPTEN__
    double cssWidth = 0, cssHeight = 0;
    if (emscripten_get_element_css_size("#canvas", &cssWidth, &cssHeight) == EMSCRIPTEN_RESULT_SUCCESS
        && cssWidth > 0 && cssHeight > 0) {
        double scale = emscripten_get_device_pixel_ratio();
        int w = std::max(1, int(cssWidth * scale)), h = std::max(1, int(cssHeight * scale));
        if (w != width_ || h != height_) {
            glfwSetWindowSize(glfwWindow_, w, h);
            resizeGL(w, h);
        }
    }
#endif
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

}

void GLFWGraphicsDriver::finalizeWindow()
{
    executeGraphicsViewFinalizeEvent();
#ifdef _WIN32
    ime_.reset();
#endif
    glfwDestroyWindow(glfwWindow_);
    glfwWindow_ = nullptr;
    terminated_ = true;
}

void GLFWGraphicsDriver::execute()
{
    if (!graphicsView()) return;
    if (!terminated_.load()) throw std::logic_error("The view is already executing");
    waitUntilStopped();
    closeRequested_ = false;
    terminated_ = false;
    executionError_ = nullptr;
#ifdef __EMSCRIPTEN__
    try {
        if (!initializeWindow()) return;
        executeGraphicsViewInitializeEvent();
    } catch (...) {
        executionError_ = std::current_exception();
        if (glfwWindow_) finalizeWindow();
        else terminated_ = true;
        throw;
    }
    // The browser owns the event loop. Keep main's stack/captures alive (JS EH).
    emscripten_set_main_loop_arg([](void* arg) {
        auto driver = static_cast<GLFWGraphicsDriver*>(arg);
        if (driver->closeRequested_.load() || glfwWindowShouldClose(driver->glfwWindow_)) {
            emscripten_cancel_main_loop();
            driver->finalizeWindow();
            return;
        }
        try {
            driver->renderFrame();
        } catch (const std::exception& error) {
            driver->executionError_ = std::current_exception();
            std::cerr << "Graphics loop failed: " << error.what() << '\n';
            emscripten_cancel_main_loop();
            driver->finalizeWindow();
        } catch (...) {
            driver->executionError_ = std::current_exception();
            emscripten_cancel_main_loop();
            driver->finalizeWindow();
        }
    }, this, frameRate() == 60 ? 0 : std::max(0, frameRate()), true);
#else
    auto run = [this](std::atomic<bool>* ready) {
        bool initialized = false;
        try {
            initialized = initializeWindow();
            if (ready) *ready = true;
            if (!initialized) return;
            executeGraphicsViewInitializeEvent();
            while (!closeRequested_.load() && !glfwWindowShouldClose(glfwWindow_)) {
                auto currentTime = std::chrono::steady_clock::now();
                renderFrame();
                if (frameRate() > 0) {
                    auto step = std::chrono::duration<double>(1.0 / frameRate());
                    std::this_thread::sleep_until(currentTime + step);
                }
            }
        } catch (const std::exception& e) {
            executionError_ = std::current_exception();
            std::cerr << "Graphics loop failed: " << e.what() << '\n';
        } catch (...) {
            executionError_ = std::current_exception();
            std::cerr << "Graphics loop failed\n";
        }
        if (glfwWindow_) finalizeWindow();
        else terminated_ = true;
        // Do not touch ready again after publishing initialization completion.
    };
    if (executionMode_ == ExecutionMode::CallingThread) {
        run(nullptr);
    } else {
        std::atomic<bool> ready{false};
        try {
            thread_ = std::make_unique<std::thread>([run, &ready] { run(&ready); });
        } catch (...) { terminated_ = true; throw; }
        while (!ready.load() && !terminated_.load()) std::this_thread::yield();
    }
#endif
}

void GLFWGraphicsDriver::terminate()
{
    // Never touch GLFWwindow from a producer/control thread.
    closeRequested_ = true;
}

void GLFWGraphicsDriver::waitUntilStopped()
{
    if (thread_ && thread_->joinable()) {
        if (thread_->get_id() == std::this_thread::get_id())
            throw std::logic_error("Cannot wait for the rendering thread from itself");
        thread_->join();
    } else if (!terminated_.load()) {
        throw std::logic_error("Cannot wait while the calling-thread/browser loop is running");
    }
}

bool GLFWGraphicsDriver::terminated() { return terminated_.load(); }

void GLFWGraphicsDriver::resizeGL(int x, int y) {
    width_ = x;
    height_ = y;
    requireResizeEvent_ = true;
}

void GLFWGraphicsDriver::mouseButtonEvent(GLFWwindow* window, int button, int action, int mods)
{
    GLFWGraphicsDriver* driver = static_cast<GLFWGraphicsDriver*>(glfwGetWindowUserPointer(window));
    if (!driver) return;

    double x, y; int w, h, fw, fh;
    glfwGetCursorPos(window, &x, &y);
    glfwGetWindowSize(window, &w, &h);
    glfwGetFramebufferSize(window, &fw, &fh);
    driver->mouseCursorX_ = w > 0 ? x * fw / w : 0;
    driver->mouseCursorY_ = h > 0 ? y * fh / h : 0;

    ptgl::MouseEvent::MouseButton btn = ptgl::MouseEvent::MouseButton::NoButton;
    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT: btn = ptgl::MouseEvent::MouseButton::LeftButton; break;
    case GLFW_MOUSE_BUTTON_MIDDLE: btn = ptgl::MouseEvent::MouseButton::MiddleButton; break;
    case GLFW_MOUSE_BUTTON_RIGHT: btn = ptgl::MouseEvent::MouseButton::RightButton; break;
    default:
        btn = ptgl::MouseEvent::MouseButton::NoButton;
        break;
    }

    const int modifyKey = modKeyMap(mods);
    const int px = int(driver->mouseCursorX_), py = int(driver->mouseCursorY_);
    if (action == GLFW_PRESS) {
        driver->isMousePressed_ = true;
        driver->pressedMouseButton_ = btn;
    } else if (action == GLFW_RELEASE) {
        driver->isMousePressed_ = false;
        driver->pressedMouseButton_ = MouseEvent::MouseButton::NoButton;
    }
    driver->events_.push_back([driver, px, py, btn, modifyKey, action] {
        auto event = driver->getGraphicsViewMouseEvent();
        event->setAccepted(false);
        if (action == GLFW_PRESS) {
            event->setPressEvent(px, py, btn, modifyKey);
            driver->executeGraphicsViewMousePressEvent(event);
        } else if (action == GLFW_RELEASE) {
            event->setReleaseEvent(px, py, btn, modifyKey);
            driver->executeGraphicsViewMouseReleaseEvent(event);
        }
    });
}

void GLFWGraphicsDriver::cursorPosEvent(GLFWwindow* window, double x, double y)
{
    GLFWGraphicsDriver* driver = static_cast<GLFWGraphicsDriver*>(glfwGetWindowUserPointer(window));
    if (!driver) return;

    int w, h, fw, fh;
    glfwGetWindowSize(window, &w, &h);
    glfwGetFramebufferSize(window, &fw, &fh);
    driver->mouseCursorX_ = w > 0 ? x * fw / w : 0;
    driver->mouseCursorY_ = h > 0 ? y * fh / h : 0;

    int px = int(driver->mouseCursorX_), py = int(driver->mouseCursorY_);
    auto button = driver->pressedMouseButton_;
    driver->events_.push_back([driver, px, py, button] {
        auto e = driver->getGraphicsViewMouseEvent();
        e->setMoveEvent(px, py, button);
        e->setAccepted(false);
        driver->executeGraphicsViewMouseMoveEvent(e);
    });
}

void GLFWGraphicsDriver::cursorEnterEvent(GLFWwindow* window, int enter)
{
    GLFWGraphicsDriver* driver = static_cast<GLFWGraphicsDriver*>(glfwGetWindowUserPointer(window));
    if (!driver) return;

    if (!enter)
        driver->events_.push_back([driver] { driver->graphicsView()->mouseLeave(); });
}

void GLFWGraphicsDriver::scrollEvent(GLFWwindow* window, double x, double y)
{
    GLFWGraphicsDriver* driver = static_cast<GLFWGraphicsDriver*>(glfwGetWindowUserPointer(window));
    if (!driver) return;

    int px = int(driver->mouseCursorX_), py = int(driver->mouseCursorY_);
    double steps = y != 0 ? y : x;
    int delta = int(80 * steps);
    int modifiers = ModifierKey_None;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
        modifiers |= ModifierKey_Shift;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
        modifiers |= ModifierKey_Control;
    auto orientation = y != 0 ? WheelEvent::Vertical : WheelEvent::Horizontal;
    driver->events_.push_back([driver, px, py, delta, orientation, steps, modifiers] {
        auto e = driver->getGraphicsViewWheelEvent();
        e->setWheelEvent(px, py, delta, orientation);
        e->setScrollSteps(steps);
        e->setModifierKey(modifiers);
        e->setAccepted(false);
        driver->executeGraphicsViewWheelEvent(e);
    });
}

void GLFWGraphicsDriver::keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    GLFWGraphicsDriver* driver = static_cast<GLFWGraphicsDriver*>(glfwGetWindowUserPointer(window));
    if (!driver) return;
#ifdef _WIN32
    if (driver->ime_ && driver->ime_->composing())
        return;
#endif

    const int modifyKey = modKeyMap(mods);
    const ptgl::Key ptglKey = GLFWGraphicsDriver::keymap(key, scancode);

    const auto type = action == GLFW_RELEASE  ? KeyEvent::KeyAction::KeyRelease
                      : action == GLFW_REPEAT ? KeyEvent::KeyAction::KeyRepeat
                                              : KeyEvent::KeyAction::KeyPress;
    driver->events_.push_back([driver, ptglKey, type, modifyKey] {
        auto e = driver->getGraphicsViewKeyEvent();
        e->setKeyPressEvent(ptglKey, type, modifyKey);
        e->setAccepted(false);
        driver->executeGraphicsViewKeyPressEvent(e);
    });
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

    std::vector<std::string> dropPaths;
    for (int i = 0; i < count; ++i)
        dropPaths.emplace_back(paths[i]);
    driver->events_.push_back([driver, dropPaths] {
        auto e = driver->getGraphicsViewDropEvent();
        e->setDropEvent(dropPaths);
        driver->executeGraphicsViewDropEvent(e);
    });
}

void GLFWGraphicsDriver::handleEvents()
{
    // handle resize event
    if (requireResizeEvent_) {
        executeGraphicsViewResizeEvent(width_, height_);
        requireResizeEvent_ = false;
    }

    auto pending = std::move(events_);
    events_.clear();
    for (auto &event : pending)
        event();
}

void GLFWGraphicsDriver::characterEvent(GLFWwindow *window, unsigned int c)
{
    auto driver = static_cast<GLFWGraphicsDriver *>(glfwGetWindowUserPointer(window));
    if (driver && c >= 32 && c != 127)
        driver->events_.push_back([driver, c] { driver->graphicsView()->textInput(gui::utf8::encode(c)); });
}
void GLFWGraphicsDriver::focusEvent(GLFWwindow *window, int focused)
{
    auto driver = static_cast<GLFWGraphicsDriver *>(glfwGetWindowUserPointer(window));
    if (driver && !focused) {
        driver->isMousePressed_ = false;
        driver->pressedMouseButton_ = MouseEvent::MouseButton::NoButton;
        driver->events_.push_back([driver] { driver->graphicsView()->cancelInput(); });
    }
}

Key GLFWGraphicsDriver::keymap(int key, int scancode)
{
    // Native scancodes differ across platforms (48 is B on Windows). Do not
    // override GLFW key tokens with a fixed X11/JIS scancode table. Resolve
    // punctuation using the active layout so the keys labelled [ and ] also
    // work on JIS keyboards, where their physical positions differ from US.
    // Emscripten's built-in GLFW aborts on glfwGetKeyName; use its key tokens.
#if !defined(__EMSCRIPTEN__) && (GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 2))
    if ((key >= GLFW_KEY_SPACE && key <= GLFW_KEY_WORLD_2) || key == GLFW_KEY_UNKNOWN) {
        const char* name = glfwGetKeyName(key, scancode);
        if (name && name[0] && name[1] == '\0') {
            switch (name[0]) {
            case '-': return Key::Key_Minus;
            case '^': return Key::Key_Caret;
            case '@': return Key::Key_At;
            case '[': return Key::Key_LeftBracket;
            case ']': return Key::Key_RightBracket;
            case ';': return Key::Key_Semicolon;
            case ':': return Key::Key_Colon;
            case ',': return Key::Key_Comma;
            case '.': return Key::Key_Period;
            case '/': return Key::Key_Slash;
            case '\\': return Key::Key_BackSlash;
            case '=': return Key::Key_Equal;
            default: break;
            }
        }
    }
#else
    (void)scancode;
#endif

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
    case GLFW_KEY_KP_ADD: return Key::Key_Equal;
    case GLFW_KEY_KP_SUBTRACT: return Key::Key_Minus;
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

} // namespace ptgl
