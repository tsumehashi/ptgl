# ptgl
ptgl is a C++ graphics library for prototyping.

![screen_shot](https://raw.githubusercontent.com/wiki/tsumehashi/ptgl/images/readme/ScreenshotSimpleDemo.png "screen_shot")

## Installation
### Requirements
* Ubuntu 18.04
* C++17  
* CMake 3.10 or later

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
MSVC builds can specify `EIGEN_DIR`, `GLEW_INCLUDE_DIR`, and
`GLEW_SHARED_LIBRARY_RELEASE` as CMake cache paths when dependencies are not found
automatically. The directory containing `glew32.dll` must be on `PATH` when running
applications using the library.

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

## Plastic rendering

Use `QuickPlasticGraphicsView` instead of `QuickGraphicsView` with the same
driver and render callbacks. For subclass-based applications, derive from
`PlasticGraphicsView` instead of `GraphicsView`.

```cpp
#include "ptgl/Core/QuickGraphicsView.h"
#include "ptgl/Driver/GLFWGraphicsDriver.h"

ptgl::QuickPlasticGraphicsView view(std::make_unique<ptgl::GLFWGraphicsDriver>());
view.setDefaultMaterial({0.35, 0.04}); // roughness, dielectric reflectance
view.setRenderSceneFunction([](ptgl::Renderer3D* r) {
    r->setColor(0.8, 0.3, 0.2);
    r->drawSphere({0, 0, 0}, r->R0(), 1.0);
    // Optional per-object override; also supported by Render3DItem recordings.
    r->setMaterial({0.65, 0.04});
    r->drawCylinder({0, 3, 0}, r->R0(), 2.0, 0.8);
});
// May also be called while running; applies at the next frame boundary.
view.setRenderStyle(ptgl::PlasticGraphicsView::RenderStyle::Legacy);
view.setRenderStyle(ptgl::PlasticGraphicsView::RenderStyle::Plastic);
```

The plastic style uses per-fragment GGX specular lighting, two directional
lights, hemispheric ambient light, linear RGB lighting, Reinhard tone mapping
and sRGB output. The BRDF follows the equations described in
[Filament's rendering reference](https://google.github.io/filament/Filament.md.html).
`setColor()` is interpreted as sRGB; `PlasticLighting` colors are linear RGB.
Use `plasticLighting()` / `setPlasticLighting()` to adjust lighting and exposure.
Material roughness is clamped to `[0.12, 1]` and reflectance to `[0, 1]`;
nonfinite material values use their defaults. Materials reset at each render
pass, so apply per-object overrides in the render callback. View settings are
thread safe; renderer calls require the rendering thread and a current context.

Existing `GraphicsView` and `QuickGraphicsView` keep their original appearance.
The plastic view preserves the common picking/depth passes, overlays and 2D UI;
overlays retain legacy shading. Spheres, cylinders and capsules use smoother
meshes only in plastic mode, including picking. `generateRoundedBox()` provides
an optional bevelled mesh without changing the shape of `drawBox()`.
Plastic view owns its scene shader selection; use ordinary `GraphicsView` for
applications that install a custom default shader.

This first stage does not implement cast/contact shadows, ambient occlusion,
environment-map reflections or ray tracing. It uses the existing compatibility
shader API with GLES precision qualifiers and no extra texture formats or GL
extensions. Rendering and switching were checked on an NVIDIA RTX A4000 with
desktop OpenGL 4.6 and native OpenGL ES 3.2; WebGL/Emscripten and GLES 2.0-only
devices still need platform validation. A shader compilation
failure is logged and falls back to legacy rendering; query
`plasticRenderingAvailable()` after initialization to check availability.
Applications should leave framebuffer sRGB conversion disabled because the
plastic shader already encodes its output. Rebuild applications when updating
the library: the renderer's public class layout and virtual methods changed.

Build the comparison example from the project root:

```sh
cmake -S . -B build -DPTGL_BUILD_PLASTIC_DEMO=ON
cmake --build build --config Release
```

Run `build/examples/PlasticDemo/PlasticDemo` (or the corresponding `.exe` in
the configuration directory). Space toggles legacy/plastic shading, Up/Down
changes roughness, right-drag orbits the camera, and the wheel zooms.
If GLFW is not discovered automatically, set `GLFW_INCLUDE_DIR` and
`GLFW_LIBRARY`. Windows shared builds also need the ptgl, GLEW and GLFW DLL
directories on `PATH`. The example selects desktop OpenGL through the existing
`PTGL_DISABLE_GLES` driver option.

## License
Licensed under the MIT license. see LICENSE for details.
