#ifndef PTGL_DRIVER_EMGLUTGRAPHICSDRIVER_H_
#define PTGL_DRIVER_EMGLUTGRAPHICSDRIVER_H_

#include <iostream>
#include "ptgl/Core/GraphicsDriver.h"
#include "ptgl/Core/GraphicsView.h"
#include "ptgl/Core/Event.h"
#include "ptgl/Core/GLPath.h"
#include <GL/glut.h>

namespace ptgl {

class EMGLUTGraphicsDriver : public GraphicsDriver {
public:
    EMGLUTGraphicsDriver(int argc, char** argv);
    virtual ~EMGLUTGraphicsDriver();

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

    void handleEvents();
    static Key keymap(int key);
    static Key specialKeymap(int key);

    // GLUT event
    static void glutResizeEvent(int width, int height);
    static void glutRenderEvent();
    static void glutUpdateEvent();
    static void glutMousePressEvent(int button , int state, int x , int y);
    static void glutMouseMotionEvent(int x , int y);
    static void glutMousePassiveEvent(int x , int y);
    static void glutKeybordEvent(unsigned char key, int x, int y);
    static void glutSpecialKeybordEvent(int key, int x, int y);

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

    std::string windowTitle_;
    int width_;
    int height_;

    int frameRate_;

    bool terminated_ = false;

    static inline EMGLUTGraphicsDriver* instance_ = nullptr;
};

EMGLUTGraphicsDriver::EMGLUTGraphicsDriver(int argc, char** argv)
{
    width_ = 640;
    height_ = 480;

    windowTitle_ = "GraphicsView";

    frameRate_ = 60;

    instance_ = this;

    glutInit(&argc , argv);
}

EMGLUTGraphicsDriver::~EMGLUTGraphicsDriver()
{
    terminate();
}

void EMGLUTGraphicsDriver::setWindowSize(int width, int height)
{
    width_ = width;
    height_ = height;
}

void EMGLUTGraphicsDriver::setWindowTitle(const std::string& title)
{
    windowTitle_ = title;
}

void EMGLUTGraphicsDriver::setFrameRate(int fps)
{
    frameRate_ = fps;
}

void EMGLUTGraphicsDriver::initialize(GraphicsView* view)
{
    GraphicsDriver::initialize(view);
}

void EMGLUTGraphicsDriver::execute()
{
    // init glut
    glutInitWindowSize(width_ , height_);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutCreateWindow(windowTitle_.c_str());

    glutReshapeFunc(EMGLUTGraphicsDriver::glutResizeEvent);
    glutDisplayFunc(EMGLUTGraphicsDriver::glutRenderEvent);
    glutIdleFunc(EMGLUTGraphicsDriver::glutUpdateEvent);

    glutMouseFunc(EMGLUTGraphicsDriver::glutMousePressEvent);
    glutMotionFunc(EMGLUTGraphicsDriver::glutMouseMotionEvent);
    glutPassiveMotionFunc(EMGLUTGraphicsDriver::glutMousePassiveEvent);
    glutKeyboardFunc(EMGLUTGraphicsDriver::glutKeybordEvent);
    glutSpecialFunc(EMGLUTGraphicsDriver::glutSpecialKeybordEvent);

    // init glew
    if (glewInit() != GLEW_OK) {
        std::cerr << "Could not init glew." << std::endl;
        //return;
    }

    // print out some info about the graphics drivers
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    // initialize event
    executeGraphicsViewInitializeEvent();

    glutMainLoop();
}

void EMGLUTGraphicsDriver::terminate()
{
    terminated_ = true;
}

bool EMGLUTGraphicsDriver::terminated()
{
    return terminated_;
}

void EMGLUTGraphicsDriver::glutResizeEvent(int width, int height)
{
    if (EMGLUTGraphicsDriver* driver = EMGLUTGraphicsDriver::instance_) {
        driver->width_ = width;
        driver->height_ = height;
        driver->requireResizeEvent_ = true;
    }
}

void EMGLUTGraphicsDriver::glutRenderEvent()
{
    if (EMGLUTGraphicsDriver* driver = EMGLUTGraphicsDriver::instance_) {
        // execute prev process
        driver->executeGraphicsViewPrevProcessEvent();

        // execute prev event process
        driver->executeGraphicsViewPrevEventProcessEvent();

        // handle events
        driver->handleEvents();

        // execute post event process
        driver->executeGraphicsViewPostEventProcessEvent();

        // render
        driver->executeGraphicsViewRenderEvent();

        // execute post process
        driver->executeGraphicsViewPostProcessEvent();
    }

    glutSwapBuffers();
}

void EMGLUTGraphicsDriver::glutUpdateEvent()
{
    glutPostRedisplay();
}

void EMGLUTGraphicsDriver::glutMousePressEvent(int button , int state, int x , int y)
{
    if (EMGLUTGraphicsDriver* driver = EMGLUTGraphicsDriver::instance_) {
        if ((button == 3) || (button == 4)) {
            // wheel event
            auto event = driver->getGraphicsViewWheelEvent();
            int sgn = (button == 3) ? 1 : -1;
            int delta = 80;
            event->setWheelEvent(x, y, sgn * delta);

            driver->requireMouseWheelEvent_ = true;
        } else {
            // mouse event
            ptgl::MouseEvent::MouseButton btn = ptgl::MouseEvent::MouseButton::NoButton;
            switch (button) {
                case GLUT_LEFT_BUTTON: btn = ptgl::MouseEvent::MouseButton::LeftButton; break;
                case GLUT_MIDDLE_BUTTON: btn = ptgl::MouseEvent::MouseButton::MiddleButton; break;
                case GLUT_RIGHT_BUTTON: btn = ptgl::MouseEvent::MouseButton::RightButton; break;
                default:
                    btn = ptgl::MouseEvent::MouseButton::NoButton;
                    break;
             }

            auto event = driver->getGraphicsViewMouseEvent();

            const int modifyKey = ptgl::ModifierKey_None;

            driver->mouseCursorX_ = x;
            driver->mouseCursorY_ = y;

            if (state ==  GLUT_DOWN) {
                // press
                driver->isMousePressed_ = true;
                driver->pressedMouseButton_ = btn;
                event->setPressEvent((int)driver->mouseCursorX_, (int)driver->mouseCursorY_, btn, modifyKey);

                driver->requireMousePressEvent_ = true;
            } else if (state == GLUT_UP) {
               // release
               driver->isMousePressed_ = false;
               driver->pressedMouseButton_ = ptgl::MouseEvent::MouseButton::NoButton;
               event->setReleaseEvent((int)driver->mouseCursorX_, (int)driver->mouseCursorY_, btn, modifyKey);

               driver->requireMouseReleaseEvent_ = true;
            }
        }
    }
}

void EMGLUTGraphicsDriver::glutMouseMotionEvent(int x , int y)
{
    if (EMGLUTGraphicsDriver* driver = EMGLUTGraphicsDriver::instance_) {
        driver->mouseCursorX_ = x;
        driver->mouseCursorY_ = y;

        auto event = driver->getGraphicsViewMouseEvent();

        event->setMoveEvent((int)driver->mouseCursorX_, (int)driver->mouseCursorY_, driver->pressedMouseButton_);

        driver->requireMouseMoveEvent_ = true;
    }
}

void EMGLUTGraphicsDriver::glutMousePassiveEvent(int x , int y)
{
    if (EMGLUTGraphicsDriver* driver = EMGLUTGraphicsDriver::instance_) {
        driver->mouseCursorX_ = x;
        driver->mouseCursorY_ = y;

        auto event = driver->getGraphicsViewMouseEvent();

        event->setMoveEvent((int)driver->mouseCursorX_, (int)driver->mouseCursorY_, driver->pressedMouseButton_);

        driver->requireMouseMoveEvent_ = true;
    }
}

void EMGLUTGraphicsDriver::glutKeybordEvent(unsigned char key, int x, int y)
{
    if (EMGLUTGraphicsDriver* driver = EMGLUTGraphicsDriver::instance_) {
        const ptgl::Key ptglKey = EMGLUTGraphicsDriver::keymap(key);
        ptgl::KeyEvent::KeyAction keyAction = ptgl::KeyEvent::KeyAction::KeyPress;
        //cout << "glu key = " << key << ", ptglKey = " << (int)ptglKey << endl;

        int modifyKey = ptgl::ModifierKey_None;
        if (glutGetModifiers() & GLUT_ACTIVE_SHIFT) {
            modifyKey |= ptgl::ModifierKey_Shift;
        }
        if (glutGetModifiers() & GLUT_ACTIVE_CTRL) {
            modifyKey |= ptgl::ModifierKey_Control;
        }
        if (glutGetModifiers() & GLUT_ACTIVE_ALT) {
            modifyKey |= ptgl::ModifierKey_Alt;
        }

        auto event = driver->getGraphicsViewKeyEvent();
        event->setKeyPressEvent(ptglKey, keyAction, modifyKey);
        driver->requireMouseKeyPressEvent_ = true;
    }
}

void EMGLUTGraphicsDriver::glutSpecialKeybordEvent(int key, int x, int y)
{
    if (EMGLUTGraphicsDriver* driver = EMGLUTGraphicsDriver::instance_) {
        const ptgl::Key ptglKey = EMGLUTGraphicsDriver::specialKeymap(key);
        ptgl::KeyEvent::KeyAction keyAction = ptgl::KeyEvent::KeyAction::KeyPress;

        int modifyKey = ptgl::ModifierKey_None;
        if (glutGetModifiers() & GLUT_ACTIVE_SHIFT) {
            modifyKey |= ptgl::ModifierKey_Shift;
        }
        if (glutGetModifiers() & GLUT_ACTIVE_CTRL) {
            modifyKey |= ptgl::ModifierKey_Control;
        }
        if (glutGetModifiers() & GLUT_ACTIVE_ALT) {
            modifyKey |= ptgl::ModifierKey_Alt;
        }

        auto event = driver->getGraphicsViewKeyEvent();
        event->setKeyPressEvent(ptglKey, keyAction, modifyKey);
        driver->requireMouseKeyPressEvent_ = true;
    }
}

void EMGLUTGraphicsDriver::handleEvents()
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
}

Key EMGLUTGraphicsDriver::keymap(int key)
{
    switch (key) {
        case 0x20: return Key::Key_Space; break;
        case 0x2a: return Key::Key_Asterisk; break;
        case 0x2b: return Key::Key_Plus; break;
        case 0x2c: return Key::Key_Comma; break;
        case 0x2d: return Key::Key_Minus; break;
        case 0x2e: return Key::Key_Period; break;
        case 0x2f: return Key::Key_Slash; break;
        case 0x30: return Key::Key_0; break;
        case 0x31: return Key::Key_1; break;
        case 0x32: return Key::Key_2; break;
        case 0x33: return Key::Key_3; break;
        case 0x34: return Key::Key_4; break;
        case 0x35: return Key::Key_5; break;
        case 0x36: return Key::Key_6; break;
        case 0x37: return Key::Key_7; break;
        case 0x38: return Key::Key_8; break;
        case 0x39: return Key::Key_9; break;
        case 0x3a: return Key::Key_Colon; break;
        case 0x3b: return Key::Key_Semicolon; break;
        case 0x3c: return Key::Key_Less; break;
        case 0x3d: return Key::Key_Equal; break;
        case 0x3e: return Key::Key_Greater; break;
        case 0x3f: return Key::Key_Question; break;
        // a
        case 0x40: return Key::Key_At; break;
        case 0x41: return Key::Key_A; break;
        case 0x42: return Key::Key_B; break;
        case 0x43: return Key::Key_C; break;
        case 0x44: return Key::Key_D; break;
        case 0x45: return Key::Key_E; break;
        case 0x46: return Key::Key_F; break;
        case 0x47: return Key::Key_G; break;
        case 0x48: return Key::Key_H; break;
        case 0x49: return Key::Key_I; break;
        case 0x4a: return Key::Key_J; break;
        case 0x4b: return Key::Key_K; break;
        case 0x4c: return Key::Key_L; break;
        case 0x4d: return Key::Key_M; break;
        case 0x4e: return Key::Key_N; break;
        case 0x4f: return Key::Key_O; break;
        case 0x50: return Key::Key_P; break;
        case 0x51: return Key::Key_Q; break;
        case 0x52: return Key::Key_R; break;
        case 0x53: return Key::Key_S; break;
        case 0x54: return Key::Key_T; break;
        case 0x55: return Key::Key_U; break;
        case 0x56: return Key::Key_V; break;
        case 0x57: return Key::Key_W; break;
        case 0x58: return Key::Key_X; break;
        case 0x59: return Key::Key_Y; break;
        case 0x5a: return Key::Key_Z; break;
        case 0x5B: return Key::Key_LeftBracket; break;
        case 0x5C: return Key::Key_BackSlash; break;
        case 0x5D: return Key::Key_RightBracket; break;
        case 0x5E: return Key::Key_Caret; break;
        // A
        case 97: return Key::Key_A; break;
        case 98: return Key::Key_B; break;
        case 99: return Key::Key_C; break;
        case 100: return Key::Key_D; break;
        case 101: return Key::Key_E; break;
        case 102: return Key::Key_F; break;
        case 103: return Key::Key_G; break;
        case 104: return Key::Key_H; break;
        case 105: return Key::Key_I; break;
        case 106: return Key::Key_J; break;
        case 107: return Key::Key_K; break;
        case 108: return Key::Key_L; break;
        case 109: return Key::Key_M; break;
        case 110: return Key::Key_N; break;
        case 111: return Key::Key_O; break;
        case 112: return Key::Key_P; break;
        case 113: return Key::Key_Q; break;
        case 114: return Key::Key_R; break;
        case 115: return Key::Key_S; break;
        case 116: return Key::Key_T; break;
        case 117: return Key::Key_U; break;
        case 118: return Key::Key_V; break;
        case 119: return Key::Key_W; break;
        case 120: return Key::Key_X; break;
        case 121: return Key::Key_Y; break;
        case 122: return Key::Key_Z; break;
        case 123: return Key::Key_LeftCurlyBrace; break;
        case 125: return Key::Key_RightCurlyBrace; break;
        case 27: return Key::Key_Escape; break;
        case 9: return Key::Key_Tab; break;
        case 127: return Key::Key_Delete; break;
        default:
            return Key::Key_Unknown;
            break;
    }

    return Key::Key_Unknown;
}

Key EMGLUTGraphicsDriver::specialKeymap(int key)
{
    switch (key) {
        case GLUT_KEY_INSERT: return Key::Key_Insert; break;
        case GLUT_KEY_RIGHT: return Key::Key_Right; break;
        case GLUT_KEY_LEFT: return Key::Key_Left; break;
        case GLUT_KEY_DOWN: return Key::Key_Down; break;
        case GLUT_KEY_UP: return Key::Key_Up; break;
        case GLUT_KEY_PAGE_DOWN: return Key::Key_PageDown; break;
        case GLUT_KEY_PAGE_UP: return Key::Key_PageUp; break;
        case GLUT_KEY_HOME: return Key::Key_Home; break;
        case GLUT_KEY_END: return Key::Key_End; break;
        case GLUT_KEY_F1: return Key::Key_F1; break;
        case GLUT_KEY_F2: return Key::Key_F2; break;
        case GLUT_KEY_F3: return Key::Key_F3; break;
        case GLUT_KEY_F4: return Key::Key_F4; break;
        case GLUT_KEY_F5: return Key::Key_F5; break;
        case GLUT_KEY_F6: return Key::Key_F6; break;
        case GLUT_KEY_F7: return Key::Key_F7; break;
        case GLUT_KEY_F8: return Key::Key_F8; break;
        case GLUT_KEY_F9: return Key::Key_F9; break;
        case GLUT_KEY_F10: return Key::Key_F10; break;
        case GLUT_KEY_F11: return Key::Key_F11; break;
        case GLUT_KEY_F12: return Key::Key_F12; break;
        case 112: return Key::Key_Shift; break;
        case 116: return Key::Key_Alt; break;
        default:
            return Key::Key_Unknown;
            break;
    }

    return Key::Key_Unknown;
}

} /* namespace ptgl */

#endif /* PTGL_DRIVER_EMGLUTGRAPHICSDRIVER_H_ */
