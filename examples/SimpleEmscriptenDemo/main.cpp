#include "ptgl/Core/QuickGraphicsView.h"
#include "ptgl/Driver/EMGLUTGraphicsDriver.h"
#include "ptgl/Handle/TransformHandle.h"
#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif  // EMSCRIPTEN

int main(int argc, char* argv[])
{
    ptgl::GraphicsDriverPtr driver = std::make_unique<ptgl::EMGLUTGraphicsDriver>(argc, argv);
    ptgl::QuickGraphicsView view(std::move(driver));
    view.setWindowTitle("SimpleEmscriptenDemo");

    ptgl::handle::TransformHandlePtr handle = std::make_shared<ptgl::handle::TransformHandle>();
    view.addGraphicsItem(handle);

    view.setRenderSceneFunction([&](ptgl::Renderer3D* r){
        r->setColor(0.8, 0.8, 1);
        r->drawBox(handle->transform()->position().data(), handle->transform()->rotation().data(), r->v<0>(1,1,1));

        r->setColor(0.8, 1, 0.8);
        r->drawBox(r->p(1, 3, 1), r->R0(), r->v<0>(1,1,1));
    });

    view.setRenderTextSceneFunction([&](ptgl::TextRenderer* r){
        r->setTextColor(1,1,1);
        auto& p = handle->transform()->position();
        r->drawText(10, 20, "pos: " + std::to_string(p(0)) + ", " + std::to_string((p(1))) + ", " + std::to_string(p(2)));
    });

    view.initialize();
    view.execute();

#ifdef EMSCRIPTEN
#else   // EMSCRIPTEN
    while (!view.terminated()) {

    }
#endif  // EMSCRIPTEN

    return 0;
}
