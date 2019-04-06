#ifndef PTGL_GUI_SLIDER_H_
#define PTGL_GUI_SLIDER_H_

#include "AbstractSlider.h"

namespace ptgl {
namespace gui {

class GraphicsItemMouseEvent;
class Renderer2D;

class Slider;
typedef std::shared_ptr<Slider> SliderPtr;

class Slider : public AbstractSlider {
public:
    Slider();
    Slider(const std::string& text);
    virtual ~Slider();

    int sliderLength() const { return sliderLength_; }
    virtual void setSliderLength(int sliderLength);

protected:

    class SliderHandle : public Widget {
    public:
        SliderHandle(Slider* slider) : slider_(slider) {}
        virtual ~SliderHandle() {}

        virtual void render2DScene(ptgl::Renderer2D* r) override;
        virtual void renderPicking2DScene(ptgl::Renderer2D* r) override;
        virtual void mouseMoveEvent(ptgl::GraphicsItemMouseEvent* e) override;

    protected:
        Slider* slider_ = nullptr;
    };
    typedef std::shared_ptr<SliderHandle> SliderHandlePtr;

    void init();

    virtual void render2DScene(ptgl::Renderer2D* r) override;
    virtual void renderPicking2DScene(ptgl::Renderer2D* r) override;

    int sliderLength_ = 100;
    SliderHandlePtr sliderHandle_;
};

}
} /* namespace ptgl */

#endif /* PTGL_GUI_SLIDER_H_ */
