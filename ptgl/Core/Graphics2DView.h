#ifndef PTGL_CORE_GRAPHICS2DVIEW_H_
#define PTGL_CORE_GRAPHICS2DVIEW_H_

#include "GraphicsView.h"

namespace ptgl {

class Graphics2DView : public GraphicsView {
public:
    Graphics2DView(GraphicsDriverPtr driver);
    virtual ~Graphics2DView();

protected:
    virtual void executeRenderScene(ptgl::Renderer3D* ) override { /* do nothing */ }
    virtual void executeRenderOverlayScene(ptgl::Renderer3D* ) override { /* do nothing */ }

    virtual void executeRenderPickingScene(ptgl::Renderer3D* ) override { /* do nothing */ }
    virtual void executeRenderPickingOverlayScene(ptgl::Renderer3D* ) override { /* do nothing */ }
};

} /* namespace ptgl */

#endif /* PTGL_CORE_GRAPHICS2DVIEW_H_ */
