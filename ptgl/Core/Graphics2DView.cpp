#include "Graphics2DView.h"
#include "GraphicsDriver.h"

namespace ptgl {

Graphics2DView::Graphics2DView(GraphicsDriverPtr driver)
    : GraphicsView(std::move(driver))
{

}

Graphics2DView::~Graphics2DView()
{

}

} /* namespace ptgl */
