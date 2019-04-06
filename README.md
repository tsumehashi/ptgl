# ptgl
ptgl is a C++ graphics library for prototyping.

![screen_shot](https://raw.githubusercontent.com/wiki/tsumehashi/ptgl/images/readme/ScreenshotSimpleDemo.png "screen_shot")

## Installation
### Requirements
* Ubuntu 18.04
* C++17  

### Install dependencies
~~~
apt-get install libeigen3-dev
apt-get install libglfw3-dev
apt-get install libglew-dev
~~~
### Build
~~~
mkdir build
cd build  
cmake ..  
make  
~~~
### Install
~~~
make install
~~~


## Example
~~~C++
#include "ptgl/Core/QuickGraphicsView.h"
#include "ptgl/Driver/GLFWGraphicsDriver.h"
#include "ptgl/Handle/TransformHandle.h"

int main(int argc, char* argv[])
{
    ptgl::GraphicsDriverPtr driver = std::make_unique<ptgl::GLFWGraphicsDriver>();
    ptgl::QuickGraphicsView view(std::move(driver));
    view.setWindowTitle("SimpleDemo");

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

    while (!view.terminated()) {

    }

    return 0;
}
~~~

## License
Licensed under the MIT license. see LICENSE for details.
