#include "GraphicsView.h"
#include <set>
#include <iostream>
#include "ptgl/Util/MathUtil.h"
#include "StandardCamera.h"
#include "GraphicsDriver.h"
#include "DefaultShaderSource.h"

//#define PTGL_DBG_PICKING
//#define PTGL_DBG_CHECK_PICKING_COLOR

namespace ptgl {

GraphicsView::GraphicsView(std::unique_ptr<GraphicsDriver> driver)
    : driver_(std::move(driver))
{
    initialized_ = false;

    // renderer
    renderer3D_ = std::make_unique<Renderer3D>(this);
    renderer2D_ = std::make_unique<Renderer2D>(this);
    textRenderer_ = std::make_unique<TextRenderer>(this);

    // event
    mouseEvent_ = std::make_unique<MouseEvent>(this);
    wheelEvent_ = std::make_unique<WheelEvent>(this);
    keyEvent_ = std::make_unique<KeyEvent>(this);
    dropEvent_ = std::make_unique<DropEvent>(this);
    pickingEvent_ = std::make_unique<PickingEvent>(this);

    // graphics item event
    graphicsItemMouseEvent_ = std::make_unique<GraphicsItemMouseEvent>(this);
    graphicsItemWheelEvent_ = std::make_unique<GraphicsItemWheelEvent>(this);
    graphicsItemHoverEvent_ = std::make_unique<GraphicsItemHoverEvent>(this);
    graphicsItemSelectEvent_ = std::make_unique<GraphicsItemSelectEvent>(this);
    graphicsItemKeyEvent_ = std::make_unique<GraphicsItemKeyEvent>(this);

    // camera
    enableCameraManipulate_ = true;
    camera_ = std::make_shared<StandardCamera>();

    // perspective
    perspectiveFovy_ = 45;        // perspective fov (deg)
    perspectiveZnear_ = 0.01;    // perspective z near
    perspectiveZfar_ = 100;        // perspective z far

    // background
    backgroundColor_ = {{0.8, 0.8, 0.8, 1.0}};

    // view mode
    viewMode_ = ViewMode::NormalViewMode;

    // time
    startTimePoint_ = std::chrono::system_clock::now();
    previousTime_ = getCurrentTime();
    currentTime_ = previousTime_;
    deltaTime_ = currentTime_ - previousTime_;
}

GraphicsView::~GraphicsView()
{

}

void GraphicsView::initialize()
{
    driver_->initialize(this);
}

void GraphicsView::execute()
{
    driver_->execute();
}

void GraphicsView::terminate()
{
    driver_->terminate();
}

bool GraphicsView::terminated()
{
    return  driver_->terminated();
}

// Window
void GraphicsView::setWindowTitle(const std::string& title)
{
    driver_->setWindowTitle(title);
}

const std::string& GraphicsView::windowTitle() const
{
    return driver_->windowTitle();
}

void GraphicsView::setWindowSize(int width, int height)
{
    driver_->setWindowSize(width, height);
}

int GraphicsView::width() const
{
    return driver_->width();
}

int GraphicsView::height() const
{
    return driver_->height();
}

// FrameRate
void GraphicsView::setFrameRate(int fps)
{
    driver_->setFrameRate(fps);
}

int GraphicsView::frameRate() const
{
    return driver_->frameRate();
}

// Camera
void GraphicsView::setCamera(CameraPtr camera)
{
    if (!camera) return;

    camera_ = camera;
    camera_->initializeConfiguration();
}

// Light
void GraphicsView::addLight(LightPtr light)
{
    if (!light) return;

    lightList_.push_back(light);
    light->initializeConfiguration();
}

void GraphicsView::removeLight(LightPtr light)
{
    auto itr = std::find(lightList_.begin(), lightList_.end(), light);
    if (itr != lightList_.end()) {
        lightList_.erase(itr);
    }
}

LightPtr GraphicsView::light(size_t index) const
{
    return index < lightList_.size() ? lightList_[index] : nullptr;
}

//  settings
void GraphicsView::setBackgroundColor(double r, double g, double b, double a)
{
    backgroundColor_ = {{r, g, b, a}};
}

// --------- GraphicsItem ------------
void GraphicsView::addGraphicsItem(GraphicsItemPtr item)
{
    if (!item) return;

    graphicsItems_.push_back(item);

    GraphicsItem::traverse(item, [&](GraphicsItemPtr ptr){
        ptr->setGraphicsView(this);
    });
}

void GraphicsView::removeGraphicsItem(GraphicsItemPtr item)
{
    if (!item) return;

    auto itr = std::find(graphicsItems_.begin(), graphicsItems_.end(), item);
    if (itr != graphicsItems_.end()) {
        graphicsItems_.erase(itr);
        GraphicsItem::traverse(item, [&](GraphicsItemPtr ptr){
            ptr->setGraphicsView(nullptr);
        });
    }
}

GraphicsItemList GraphicsView::getTraversedGraphicsItems() const
{
    GraphicsItemList items;
    for (auto& item : graphicsItems_) {
        GraphicsItem::traverse(item, [&](GraphicsItemPtr ptr){
            items.push_back(ptr);
        });
    }
    return items;
}

// shader program
void GraphicsView::setDefaultShaderProgram(ShaderProgramPtr shaderProgram)
{
    renderer3D_->setDefaultShaderProgram(shaderProgram);
}

void GraphicsView::setPickingUpShaderProgram(ShaderProgramPtr shaderProgram)
{
    if (!shaderProgram) return;
    pickingUpShaderProgram_ = shaderProgram;
}

void GraphicsView::setDepthRenderShaderProgram(ShaderProgramPtr shaderProgram)
{
    if (!shaderProgram) return;
    depthRenderShaderProgram_ = shaderProgram;
}

void GraphicsView::setDefault2DShaderProgram(ShaderProgramPtr shaderProgram)
{
    renderer2D_->setDefaultShaderProgram(shaderProgram);
}

// ---------- execute event ----------
void GraphicsView::executeInitializeEvent()
{
    // initialize renderer
    renderer3D_->initializeConfiguration();
    renderer2D_->initializeConfiguration();
    textRenderer_->initializeConfiguration();

    // load Shader
    loadDefaultShader();

    ///----------------
    glClearColor(backgroundColor_[0], backgroundColor_[1], backgroundColor_[2], backgroundColor_[3]);

    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glEnable(GL_POINT_SPRITE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    camera_->initializeConfiguration();

    initProcess();

    initialized_ = true;
}

void GraphicsView::executeResizeEvent(int width, int height)
{
    double aspect = (double)width / (double)height;
    glViewport(0, 0, width, height);
    camera_->setPerspective(perspectiveFovy_, aspect, perspectiveZnear_, perspectiveZfar_);
    camera_->updateViewport();

    resizeEvent(width, height);
}

void GraphicsView::executeRenderEvent()
{
    // update camera
    camera()->updateViewport();
    camera()->update();

    // set camera
    renderer3D_->setProjectionViewMatrix(camera_->projection()*camera_->modelview());

    // traverse GraphicsItem
    traversedItems_.clear();
    for (auto& item : graphicsItems_) {
        GraphicsItem::traverse(item, [&](GraphicsItemPtr ptr){
            if (ptr->isEnabled()) {
                traversedItems_.push_back(ptr);
             }
        });
    }

    // execute GraphicsItem prev process
    for (auto& item : traversedItems_) {
        item->executePrevProcess();
    }

#if 1
    // -------- render picking scene ------
#ifdef PTGL_DBG_PICKING
    glClearColor(0.8, 0.8, 0.8, 1.0);    // set background == 0
#else    // PTGL_DBG_PICKING
    glClearColor(0.0, 0.0, 0.0, 1.0);    // set background == 0
#endif    // PTGL_DBG_PICKING
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glDisable(GL_BLEND);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc( GL_LEQUAL );

    glDisable(GL_MULTISAMPLE);    // disable anti-aliasing

    if (!pickingUpShaderProgram_->valid()) {
        std::cout << "picking shader invalid" << std::endl;
    }

    // init pick id
    pickIdToItemList_.clear();
    pickIdToItemList_.push_back(nullptr);    // id=0 is background

    // set picking up shader
    renderer3D_->setForceUseShaderProgram(pickingUpShaderProgram_);
    // renderOverlayScene
    executeRenderPickingOverlayScene(renderer3D_.get());
    // restore shader
    renderer3D_->setForceUseShaderProgram(nullptr);

    // render2DScene
    // set picking up shader
    renderer2D_->setForceUseShaderProgram(pickingUpShaderProgram_);
    // render2DScene
    executeRenderPicking2DScene(renderer2D_.get());
    // restore shader
    renderer2D_->setForceUseShaderProgram(nullptr);

    // handle picking
    pickedGraphicsItem_ = pickingUpGraphicsItem(mouseX_, height() - mouseY_);

    // renderScene
    if (!pickedGraphicsItem_) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        // set picking up shader
        renderer3D_->setForceUseShaderProgram(pickingUpShaderProgram_);
        // renderScene
        executeRenderPickingScene(renderer3D_.get());
        // restore shader
        renderer3D_->setForceUseShaderProgram(nullptr);

        // handle picking
        pickedGraphicsItem_ = pickingUpGraphicsItem(mouseX_, height() - mouseY_);
    }
#endif

#ifdef PTGL_DBG_PICKING
#else    // PTGL_DBG_PICKING
    // ------- render depth scene --------
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glDisable(GL_BLEND);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc( GL_LEQUAL );

    glDisable(GL_MULTISAMPLE);    // disable anti-aliasing

    renderer3D_->setForceUseShaderProgram(depthRenderShaderProgram_);
    depthRenderShaderProgram_->bind();
    depthRenderShaderProgram_->setParameter("perspectiveZnear", perspectiveZnear());
    depthRenderShaderProgram_->setParameter("perspectiveZfar", perspectiveZfar());

    // renderScene
    executeRenderScene(renderer3D_.get());

    // renderOverlayScene
    executeRenderOverlayScene(renderer3D_.get());

    renderer3D_->setForceUseShaderProgram(nullptr);

    // render2DScene
    executeRender2DScene(renderer2D_.get());

    // picking
    calcPickedDepth();
    handlePickingUpEvent();

#endif    // PTGL_DBG_PICKING

#ifdef PTGL_DBG_PICKING
#else    // PTGL_DBG_PICKING

    // ------- render scene --------
    glClearColor(backgroundColor_[0], backgroundColor_[1], backgroundColor_[2], backgroundColor_[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc( GL_LEQUAL );

    glEnable(GL_MULTISAMPLE);    // enable anti-aliasing

    // renderBackground2DScene
    glClear(GL_DEPTH_BUFFER_BIT);    // clear depth buffer
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    executeRenderBackground2DScene(renderer2D_.get());

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // renderScene
    executeRenderScene(renderer3D_.get());

    // post renderScene
    executeRenderScenePostProcess(renderer3D_.get());

    // renderOverlayScene
    glClear(GL_DEPTH_BUFFER_BIT);    // clear depth buffer
    executeRenderOverlayScene(renderer3D_.get());

    // render2DScene
    glClear(GL_DEPTH_BUFFER_BIT);    // clear depth buffer
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    executeRender2DScene(renderer2D_.get());

    // renderTextScene
    executeRenderTextScene(textRenderer_.get());

#endif    // PTGL_DBG_PICKING

    // execute GraphicsItem post process
    for (auto& item : traversedItems_) {
        item->executePostProcess();
    }

}

// execute prev/post process (render)
void GraphicsView::executePrevProcess()
{
    prevProcess();
}

void GraphicsView::executePostProcess()
{
    postProcess();

    ++currentFrame_;
    currentTime_ = getCurrentTime();
    previousTime_ = currentTime_;
    deltaTime_ = currentTime_ - previousTime_;
}

// execute prev/post process (event)
void GraphicsView::executePrevEventProcess()
{
    prevEventProcess();
}

void GraphicsView::executePostEventProcess()
{
    postEventProcess();
}

// render scene
void GraphicsView::executeRenderBackground2DScene(ptgl::Renderer2D* r)
{
    r->beginRender(Renderer2D::RenderBackgroundSceneState);

    // render GraphicsView
    renderBackground2DScene(r);

    // render GraphicsItem
    for (auto item : traversedItems_) {
        if (item->isEnabled() && item->isVisible()) {
            item->renderBackground2DScene(r);
        }
    }

    r->endRender(Renderer2D::RenderBackgroundSceneState);
}

void GraphicsView::executeRenderScene(ptgl::Renderer3D* r)
{
    r->beginRender(Renderer3D::RenderSceneState);

    // render GraphicsView
    renderScene(r);

    // render GraphicsItem
    for (auto item : traversedItems_) {
        if (item->isEnabled() && item->isVisible()) {
            item->renderScene(r);
        }
    }

    r->endRender(Renderer3D::RenderSceneState);
}

void GraphicsView::executeRenderOverlayScene(ptgl::Renderer3D* r)
{
    renderer3D_->beginRender(Renderer3D::RenderOverlayState);

    // render GraphicsView
    renderOverlayScene(r);

    // render GraphicsItem
    for (auto item : traversedItems_) {
        if (item->isEnabled() && item->isVisible()) {
            item->renderOverlayScene(r);
        }
    }

    renderer3D_->endRender(Renderer3D::RenderOverlayState);
    renderer3D_->setRenderState(Renderer3D::RenderNoneState);
}

void GraphicsView::executeRender2DScene(ptgl::Renderer2D* r)
{
    renderer2D_->beginRender(Renderer2D::RenderSceneState);

    // render GraphicsItem
    for (auto item : traversedItems_) {
        if (item->isEnabled() && item->isVisible()) {
            item->render2DScene(r);
        }
    }

    // render GraphicsView
    render2DScene(r);

    renderer2D_->endRender(Renderer2D::RenderSceneState);
    renderer2D_->setRenderState(Renderer2D::RenderNoneState);
}

void GraphicsView::executeRenderTextScene(ptgl::TextRenderer* r)
{
    textRenderer_->beginRender(TextRenderer::RenderTextSceneState);

    // render GraphicsItem
    for (auto item : traversedItems_) {
        if (item->isEnabled() && item->isVisible()) {
            item->renderTextScene(r);
        }
    }

    // render GraphicsView
    renderTextScene(r);

    textRenderer_->endRender(TextRenderer::RenderTextSceneState);
    textRenderer_->setRenderState(TextRenderer::RenderNoneState);
}

// render picking scene
void GraphicsView::executeRenderPickingScene(ptgl::Renderer3D* r)
{
    r->beginRender(Renderer3D::RenderPickingState);

    // render GraphicsItem
    for (auto item : traversedItems_) {
        if (item->isEnabled() && item->isVisible() && item->isPickable()) {
            uint32_t pickId = (uint32_t)pickIdToItemList_.size();
            auto pickColor = pickIdToColor(pickId);
            pickingUpShaderProgram_->setParameter("pickingUpColor", pickColor[0], pickColor[1], pickColor[2], pickColor[3]);
            item->renderPickingScene(r);
            pickIdToItemList_.push_back(item);
        }
    }

    r->endRender(Renderer3D::RenderPickingState);
}

void GraphicsView::executeRenderPickingOverlayScene(ptgl::Renderer3D* r)
{
    renderer3D_->beginRender(Renderer3D::RenderPickingOverlayState);

    // render GraphicsItem
    for (auto item : traversedItems_) {
        if (item->isEnabled() && item->isVisible() && item->isPickable()) {
            uint32_t pickId = (uint32_t)pickIdToItemList_.size();
            auto pickColor = pickIdToColor(pickId);
            pickingUpShaderProgram_->setParameter("pickingUpColor", pickColor[0], pickColor[1], pickColor[2], pickColor[3]);
            item->renderPickingOverlayScene(r);
            pickIdToItemList_.push_back(item);
        }
    }

    pickingUpShaderProgram_->setParameter("pickingUpColor", 0.0, 0.0, 0.0, 1.0);

    renderer3D_->endRender(Renderer3D::RenderPickingOverlayState);
    renderer3D_->setRenderState(Renderer3D::RenderNoneState);
}

void GraphicsView::executeRenderPicking2DScene(ptgl::Renderer2D* r)
{
    renderer2D_->beginRender(Renderer2D::RenderPickingState);

    // render GraphicsItem
    for (auto item : traversedItems_) {
        if (item->isEnabled() && item->isVisible() && item->isPickable()) {
            uint32_t pickId = (uint32_t)pickIdToItemList_.size();
            auto pickColor = pickIdToColor(pickId);
            renderer2D_->setPickColor(pickColor);
            item->renderPicking2DScene(r);
            pickIdToItemList_.push_back(item);
        }
    }

    pickingUpShaderProgram_->setParameter("pickingUpColor", 0.0, 0.0, 0.0, 1.0);

    renderer2D_->endRender(Renderer2D::RenderPickingState);
    renderer2D_->setRenderState(Renderer2D::RenderNoneState);
}

// execute render scene post process
void GraphicsView::executeRenderScenePostProcess(ptgl::Renderer3D* r)
{
    renderScenePostProcess(r);
}

// input event
void GraphicsView::executeMousePressEvent(MouseEvent* e)
{
    if (e->button() == MouseEvent::MouseButton::RightButton
     ||    e->button() == MouseEvent::MouseButton::MiddleButton) {
        if (enableCameraManipulate_) {
            camera_->mousePressEvent(e);
        }
    }

    mouseX_ = e->x();
    mouseY_ = e->y();

    executeGraphicsItemMousePressEvent(e);

    // post event
    mousePressEvent(e);

    if (e->button() == MouseEvent::MouseButton::LeftButton) {
        // handle picking up event
        executePickingUpEvent(pickingEvent_.get());
    }
}

void GraphicsView::executeMouseMoveEvent(MouseEvent* e)
{
    if (e->button() == MouseEvent::MouseButton::RightButton
    ||    e->button() == MouseEvent::MouseButton::MiddleButton) {
        if (enableCameraManipulate_) {
            camera_->mouseMoveEvent(e);
        }
    }

    mouseX_ = e->x();
    mouseY_ = e->y();

    executeGraphicsItemMouseMoveEvent(e);

    // post event
    mouseMoveEvent(e);
}

void GraphicsView::executeMouseReleaseEvent(MouseEvent* e)
{
    if (e->button() == MouseEvent::MouseButton::RightButton ||
            e->button() == MouseEvent::MouseButton::MiddleButton) {
        if (enableCameraManipulate_) {
            camera_->mouseReleaseEvent(e);
        }
    }

    mouseX_ = e->x();
    mouseY_ = e->y();

    executeGraphicsItemMouseReleaseEvent(e);

    // post event
    mouseReleaseEvent(e);
}

void GraphicsView::executeWheelEvent(WheelEvent* e)
{
    graphicsItemWheelEvent_->setWheelEvent(e->x(), e->y(), e->delta(), e->orientation());
    graphicsItemWheelEvent_->setAccepted(false);
#if 0
    GraphicsItemPtr item = focusedGraphicsItem_ ? focusedGraphicsItem_ : hoveredGraphicsItem_;
#else
    GraphicsItemPtr item = hoveredGraphicsItem_;
#endif
    if (item && item->isPickable() && item->isEnabledWheelEvent()) {
        item->wheelEvent(graphicsItemWheelEvent_.get());
    }

    if (!graphicsItemWheelEvent_->isAccepted() && enableCameraManipulate_) {
        camera_->wheelEvent(e);
    }

    wheelEvent(e);
}

void GraphicsView::executeKeyPressEvent(KeyEvent* e)
{
    graphicsItemKeyEvent_->setKeyPressEvent(e->key(), e->keyAction(), e->modifierKey());
    graphicsItemKeyEvent_->setAccepted(false);
    if (focusedGraphicsItem_ && focusedGraphicsItem_->isPickable() && focusedGraphicsItem_->isEnabledKeyEvent()) {
        focusedGraphicsItem_->keyPressEvent(graphicsItemKeyEvent_.get());
    }

    e->setAccepted(graphicsItemKeyEvent_->isAccepted());
    keyPressEvent(e);
}

void GraphicsView::executeDropEvent(DropEvent* e)
{
    dropEvent(e);
}

void GraphicsView::executePickingUpEvent(PickingEvent* e)
{
    pickingUpEvent(e);
}

// -------- picking -----------
std::array<double, 4> GraphicsView::pickIdToColor(uint32_t id)
{
    double r = (0xFF & (id >> 16)) / 255.0;
    double g = (0xFF & (id >> 8)) / 255.0;
    double b = (0xFF & id) / 255.0;
    double a = 1.0;
    return {{r,g,b,a}};
}

uint32_t GraphicsView::colorToPickId(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    uint32_t ir = r;
    uint32_t ig = g;
    uint32_t ib = b;
    (void)a;
    uint32_t id = (0xFF0000 & (ir << 16)) | (0xFF00 & (ig << 8)) | (0xFF & ib);
    return id;
}

GraphicsItemPtr GraphicsView::pickingUpSceneGrahicsItem(int x, int y)
{
    // read pixel
    uint8_t pixel[4];   // rgba
     GLsizei    width = 1;
     GLsizei    height = 1;
     GLenum  format = GL_RGBA;
     GLenum  type = GL_UNSIGNED_BYTE;
    GLvoid *data = pixel;

    glReadPixels(x, y, width, height, format, type, data);
    uint32_t pickId = colorToPickId(pixel[0], pixel[1], pixel[2], 1.0);

#ifdef PTGL_DBG_PICKING
    std::cout << "pickingUpSceneGrahicsItems x = " << x << ", y = " << y << std::endl;
    std::cout << "r = " << (int)pixel[0] << ", g = " << (int)pixel[1] << ", b = " << (int)pixel[2] << ", pickId = " << pickId << std::endl;
#endif    // PTGL_DBG_PICKING


#ifdef PTGL_DBG_CHECK_PICKING_COLOR    // check picking id color
    {
        GLenum  format = GL_RGBA;
        GLenum  type = GL_UNSIGNED_BYTE;

        std::vector<uint8_t> pixels;
        std::set<std::array<uint8_t, 3>> colorSet;
        pixels.resize(4*this->width() * this->height());

        glReadPixels(0, 0, this->width(), this->height(), format, type, pixels.data());
        for (int i = 0; i < pixels.size()/4;++i) {
            uint8_t r = pixels[i*4];
            uint8_t g = pixels[i*4+1];
            uint8_t b = pixels[i*4+2];
            colorSet.insert({{r,g,b}});
         }

        std::cout << "picking scene unique color = " << colorSet.size() << std::endl;
        for (auto & c : colorSet) {
            std::cout << "c = " << (int)c[0] << ", " << (int)c[1] << ", " << (int)c[2] << std::endl;
         }
    }
#endif    // PTGL_DBG_CHECK_PICKING_COLOR

    // picked
    GraphicsItemPtr pickedItem;
    if (pickId < pickIdToItemList_.size()) {
        pickedItem = pickIdToItemList_[pickId];

#ifdef PTGL_DBG_PICKING
        if (pickedItem) {
            std::cout << "picked id = " << pickId << ", name = " << pickedItem->name() << ", rgb=" << (int)pixel[0] << "," << (int)pixel[1] << "," << (int)pixel[2] << std::endl;
         }
#endif    // PTGL_DBG_PICKING
    }

    return pickedItem;
}

GraphicsItemPtr GraphicsView::pickingUpOverlaySceneGrahicsItem(int x, int y)
{
    (void)x;
    (void)y;
    return nullptr;
}

GraphicsItemPtr GraphicsView::pickingUp2DSceneGrahicsItem(int x, int y)
{
    (void)x;
    (void)y;
    return nullptr;
}

GraphicsItemPtr GraphicsView::pickingUpGraphicsItem(int x, int y)
{
    GraphicsItemPtr item;

    // picking up 2D scene
    item = pickingUp2DSceneGrahicsItem(x, y);
    if (item) return item;

    // picking up overlay 3D scene
    item = pickingUpOverlaySceneGrahicsItem(x, y);
    if (item) return item;

    // picking up 3D scene
    item = pickingUpSceneGrahicsItem(x, y);

    return item;
}

void GraphicsView::calcPickedDepth()
{
    // read from depth render
    uint8_t rgba[4];
    glReadPixels(mouseX_, height() - mouseY_, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

    auto rgbToValue = [](float r, float g, float b)    {
        double v = 255.0*255.0*b + 255.0*g + r;
        return v / (255.0*255.0*255.0);
    };
    auto clamp = [](double x, double mi, double ma) {
        return std::min(std::max(x, mi), ma);
    };

    double f = this->perspectiveZfar();
    double n = this->perspectiveZnear();
    double depth = clamp(rgbToValue(rgba[0], rgba[1], rgba[2]), 0.0, 1.0);
    double z = (f - n)*depth + n;

    pickedZ_ = z;
    pickedDepth_ = depth;
}

void GraphicsView::handlePickingUpEvent()
{
    int x = mouseX_;
    int y = height() - mouseY_;

    GraphicsItemPtr pickedItem = pickedGraphicsItem_;

    // near
    Eigen::Vector3d nv = ptgl::unProject(Eigen::Vector3d(x, y, 0), camera()->modelview(), camera()->projection(), camera()->viewport());
    // far
    Eigen::Vector3d fv = ptgl::unProject(Eigen::Vector3d(x, y, 1), camera()->modelview(), camera()->projection(), camera()->viewport());

    Eigen::Vector3d direction = fv - nv;
    direction.normalize();

    Eigen::Vector3d origin = camera()->position();
    Eigen::Vector3d point = origin + pickedZ_ * direction;

    bool isPicked = false;
    if (pickedItem) {
        isPicked = true;
    }

    pickingEvent_->setPickingEvent(isPicked, x, y, origin, direction, point, pickedItem);
}

// handle GraphicsItem event
void GraphicsView::executeGraphicsItemMousePressEvent(MouseEvent* e)
{
    graphicsItemMouseEvent_->setPressEvent(e->x(), e->y(), e->button(), e->modifierKey());

    GraphicsItemPtr pickedGraphicsItem = pickedGraphicsItem_;

    if (e->button() == MouseEvent::MouseButton::LeftButton) {

        // pick changed
        if (focusedGraphicsItem_ != pickedGraphicsItem) {
            // select enter/leave event
            if (focusedGraphicsItem_) {
                // set select leave event
                focusedGraphicsItem_->setPicked(false);
                focusedGraphicsItem_->selectLeaveEvent(graphicsItemSelectEvent_.get());
                focusedGraphicsItem_ = nullptr;
            }

            // set select enter event
            if (pickedGraphicsItem && pickedGraphicsItem->isPickable()) {
                pickedGraphicsItem->setPicked(true);
                pickedGraphicsItem->selectEnterEvent(graphicsItemSelectEvent_.get());
            }

            focusedGraphicsItem_ = pickedGraphicsItem;
        }

        // picked
        if (pickedGraphicsItem && pickedGraphicsItem->isPickable()) {
            // mouse press event
            // check another item clicked
            if (prevMousePressGraphicsItem_ != pickedGraphicsItem) {
                graphicsItemMouseEvent_->clearEventState();
            }
            mouseGraphicsItem_ = pickedGraphicsItem;
            prevMousePressGraphicsItem_ = mouseGraphicsItem_;
            // set picking
            if (pickingEvent_->pickedGraphicsItem() == pickedGraphicsItem) {
                graphicsItemMouseEvent_->setPickingEvent(
                        pickingEvent_->isPicked(),
                        pickingEvent_->origin(),
                        pickingEvent_->direction(),
                        pickingEvent_->point());
            }
            mouseGraphicsItem_->executeMousePressEvent(graphicsItemMouseEvent_.get());
            graphicsItemMouseEvent_->setPickingEvent(false);    // clear
        } else {
            mouseGraphicsItem_ = nullptr;
        }
    }
}

void GraphicsView::executeGraphicsItemMouseMoveEvent(MouseEvent* e)
{
    graphicsItemMouseEvent_->setMoveEvent(e->x(), e->y(), e->button());

    if (e->button() == MouseEvent::MouseButton::LeftButton) {

        // mouse event
        if (mouseGraphicsItem_) {
            mouseGraphicsItem_->executeMouseMoveEvent(graphicsItemMouseEvent_.get());
        }
    }

    GraphicsItemPtr pickedGraphicsItem = pickedGraphicsItem_;

    if (pickedGraphicsItem) {
        // mouse hover event
        if (hoveredGraphicsItem_ != pickedGraphicsItem) {
            // mouse hover leave event
            if (hoveredGraphicsItem_) {
                hoveredGraphicsItem_->setHoverd(false);
                graphicsItemHoverEvent_->setHoverLeaveEvent(e->x(), e->y());
                hoveredGraphicsItem_->hoverLeaveEvent(graphicsItemHoverEvent_.get());
            }

            // mouse hover enter event
            if (pickedGraphicsItem->isPickable()) {
                hoveredGraphicsItem_ = pickedGraphicsItem;
                hoveredGraphicsItem_->setHoverd(true);
                graphicsItemHoverEvent_->setHoverEnterEvent(e->x(), e->y());
                hoveredGraphicsItem_->hoverEnterEvent(graphicsItemHoverEvent_.get());
            } else {
                hoveredGraphicsItem_ = nullptr;
            }
        } else {
            // mouse hover move event
            if (hoveredGraphicsItem_) {
                hoveredGraphicsItem_->setHoverd(true);
                graphicsItemHoverEvent_->setHoverMoveEvent(e->x(), e->y());
                hoveredGraphicsItem_->hoverMoveEvent(graphicsItemHoverEvent_.get());
            }
        }
    } else {
        // mouse hover leave event
        if (hoveredGraphicsItem_) {
            hoveredGraphicsItem_->setHoverd(false);
            graphicsItemHoverEvent_->setHoverLeaveEvent(e->x(), e->y());
            hoveredGraphicsItem_->hoverLeaveEvent(graphicsItemHoverEvent_.get());
            hoveredGraphicsItem_ = nullptr;
        }
    }
}

void GraphicsView::executeGraphicsItemMouseReleaseEvent(MouseEvent* e)
{
    graphicsItemMouseEvent_->setReleaseEvent(e->x(), e->y(), e->button(), e->modifierKey());

    // mouse event
    if (mouseGraphicsItem_) {
        mouseGraphicsItem_->executeMouseReleaseEvent(graphicsItemMouseEvent_.get());
        mouseGraphicsItem_ = nullptr;
    }
}

// shader
void GraphicsView::loadDefaultShader()
{
     pickingUpShaderProgram_ = std::make_shared<ptgl::ShaderProgram>();
     {
        ptgl::ShaderList shaders;
        shaders.push_back(ptgl::Shader::loadFromSource(ptgl::Shader::VertexShader, DefaultShaderSource::PickingUpVertexShaderSource));
        shaders.push_back(ptgl::Shader::loadFromSource(ptgl::Shader::FragmentShader, DefaultShaderSource::PickingUpFragmentShaderSource));

        pickingUpShaderProgram_->linkShaders(shaders);
        pickingUpShaderProgram_->setName("PickingUpShaderProgram");
     }

     depthRenderShaderProgram_ = std::make_shared<ptgl::ShaderProgram>();
     {
        ptgl::ShaderList shaders;
        shaders.push_back(ptgl::Shader::loadFromSource(ptgl::Shader::VertexShader, DefaultShaderSource::DepthRenderVertexShaderSource));
        shaders.push_back(ptgl::Shader::loadFromSource(ptgl::Shader::FragmentShader, DefaultShaderSource::DepthRenderFragmentShaderSource));

        depthRenderShaderProgram_->linkShaders(shaders);
        depthRenderShaderProgram_->setName("DepthRenderShaderProgram");
     }
}

// get time
double GraphicsView::getCurrentTime()
{
    return std::chrono::duration<double>(std::chrono::system_clock::now() - startTimePoint_).count();
}

// ------- for GraphicsDriver ---------
void GraphicsView::executeGraphicsViewInitializeEvent()
{
    executeInitializeEvent();
}

void GraphicsView::executeGraphicsViewResizeEvent(int width, int height)
{
    executeResizeEvent(width, height);
}

void GraphicsView::executeGraphicsViewRenderEvent()
{
    executeRenderEvent();
}

void GraphicsView::executeGraphicsViewPrevProcessEvent()
{
    executePrevProcess();
}

void GraphicsView::executeGraphicsViewPostProcessEvent()
{
    executePostProcess();
}

void GraphicsView::executeGraphicsViewPrevEventProcessEvent()
{
    executePrevEventProcess();
}

void GraphicsView::executeGraphicsViewPostEventProcessEvent()
{
    executePostEventProcess();
}

void GraphicsView::executeGraphicsViewMousePressEvent(MouseEvent* e)
{
    executeMousePressEvent(e);
}

void GraphicsView::executeGraphicsViewMouseMoveEvent(MouseEvent* e)
{
    executeMouseMoveEvent(e);
}

void GraphicsView::executeGraphicsViewMouseReleaseEvent(MouseEvent* e)
{
    executeMouseReleaseEvent(e);
}

void GraphicsView::executeGraphicsViewWheelEvent(WheelEvent* e)
{
    executeWheelEvent(e);
}

void GraphicsView::executeGraphicsViewKeyPressEvent(KeyEvent* e)
{
    executeKeyPressEvent(e);
}

void GraphicsView::executeGraphicsViewDropEvent(DropEvent* e)
{
    executeDropEvent(e);
}

} /* namespace ptgl */
