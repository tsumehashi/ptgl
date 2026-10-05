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

The key directional light now casts shadows, with tent-weighted 5x5
[percentage-closer filtering](https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing)
to soften their edges. Fill and ambient lighting remain unshadowed. This is a
finite directional shadow map. Plastic shading also includes procedural studio
environment reflections and screen-space ambient occlusion (SSAO). Reflections
blur with material roughness; they represent the studio environment, not other
scene objects. No ray tracing or environment-image loading is required.
The shadow pass uses the existing compatibility shader API with GLES precision
qualifiers and no extra texture formats or GL extensions.
Rendering and switching were checked on an NVIDIA RTX A4000 with
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
S toggles shadows, `[` / `]` adjusts their softness, and L animates the key light.
E toggles environment reflections and A toggles ambient occlusion.
Left-click an object to attach a `TransformHandle`. Drag its arrows or planes to
move the object, or its rings to rotate it. Clicking the floor or background
clears the selection. Object transforms persist when switching shading styles,
and shadows follow the transformed objects in Plastic mode.
The selected object is shown at 50% opacity in both styles and returns to opaque
when deselected. Selection transparency preserves its picking geometry and shadows.
The demo uses the shared `GraphicsItem::setOpacity()` API. Its shadow volume
automatically follows moved and rotated objects.
If GLFW is not discovered automatically, set `GLFW_INCLUDE_DIR` and
`GLFW_LIBRARY`. Windows shared builds also need the ptgl, GLEW and GLFW DLL
directories on `PATH`. The example selects desktop OpenGL through the existing
`PTGL_DISABLE_GLES` driver option.

### Shadow settings

```cpp
auto shadows = view.shadowSettings();
shadows.enabled = true; // Default; Legacy mode skips the shadow pass entirely.
shadows.center = {0, 0, 1};
shadows.halfExtent = 6; // Light-space half-width in world units.
shadows.resolution = 1024;
shadows.softness = 2.5; // Filter radius in texels; 0 gives a hard edge.
shadows.autoFit = true; // Fit visible, lit meshes after their transforms.
view.setShadowSettings(shadows);
```

The orthographic shadow volume looks along `PlasticLighting::keyDirection`
toward `center`, with a width and height of `2 * halfExtent` and an approximate
depth of `4 * halfExtent`. Choose bounds containing both casters and receivers;
outside this volume surfaces remain lit. Tighter bounds give sharper shadows.
With `autoFit == false` (the compatibility default), these bounds stay fixed in
world space as the camera moves. With `autoFit == true`, an additional geometry
pass includes primitive meshes, registered vertices, `drawVertex()` meshes and
view callbacks. Invisible/disabled items and unlit helpers do not contribute.
Raw OpenGL draws and static VBO draw helpers cannot contribute bounds; use manual
bounds for those applications. `padding` adds a fractional margin (default 0.1).
Size and light-space center are quantized to limit movement of the shadow texels.
`effectiveShadowSettings()` returns the fitted bounds used by the latest frame.
Empty scenes retain the configured bounds. Resolution remains bounded, so widely
separated objects reduce shadow detail. Cascaded shadow maps are not implemented.

`bias` is measured in normalized map depth and `normalBias` in world units.
Increase them slightly if self-shadow artifacts appear; excessive values make
shadows detach from surfaces. `strength` scales only the key light's occlusion.
Resolution must be a power of two in `[256, 4096]` and is reduced to the GPU's
texture/renderbuffer limits. See `ShadowSettings` for the remaining ranges.
The settings are snapshotted on the rendering thread at each frame boundary.
`shadowsActive()` reports whether the last prepared frame used a valid map.

Offscreen rendering invokes ordinary scene callbacks with
`RenderShadowState`, `RenderBoundsState` or `RenderOcclusionState`; transparent
items also receive `RenderTransparentState`. Update animation in
`prevProcess`/item updates, not inside draw callbacks. Clicks also refresh picking
at the event position without advancing item updates. The camera object remains
the user's camera; the renderer's projection-view matrix describes the light
during that pass. Visible, lit scene surfaces cast shadows. Draw colors with
alpha below 1, unlit helpers, overlays and 2D UI do not; item opacity alone is a
display effect and preserves shadows unless `setCastsShadow(false)` is used.
Transparent surfaces can
receive shadows. Each custom scene shader must explicitly support shadow
sampling; the built-in plastic pipeline owns texture unit 7 during its color
pass and restores its binding afterward.
Sampler objects on that unit are also temporarily detached when supported, so
custom texture filtering cannot alter packed depth comparisons.

The shadow map uses an RGBA8 color texture containing packed depth and a
16-bit depth renderbuffer, avoiding a depth-texture extension requirement.
Fragment `highp` is required for reliable depth comparisons on GLES. A missing
capability or incomplete framebuffer logs a warning and keeps plastic lighting
without shadows. Shadow-map allocation and texture/raster state changes are
confined to the rendering thread. GLFW and EMGLUT release these resources in
the finalize event; custom drivers should call
`executeGraphicsViewFinalizeEvent()` while their GL context is current, before
destroying it. This event also releases the renderers' shaders, mesh buffers
and NanoVG contexts, so demo shutdown does not try to delete GL objects after
their context is gone. Call the base implementation when overriding it.
When a desktop GLEW build leaves GLES's `glClearDepthf` unresolved, shadows use
the default clear depth of 1; a different clear depth disables the shadow pass
until a complete GLES function loader or the default clear value is used.

### Environment reflections and contact shading

```cpp
auto environment = view.environmentSettings();
environment.enabled = true;
environment.strength = 0.7; // [0, 4]
environment.rotation = 30; // Degrees about world Z, [-360, 360]
view.setEnvironmentSettings(environment);

auto occlusion = view.ambientOcclusionSettings();
occlusion.enabled = true;
occlusion.radius = 0.5;   // World units, (0, 100]
occlusion.strength = 1;  // [0, 4]
occlusion.bias = 0.02;   // World units, [0, radius]
view.setAmbientOcclusionSettings(occlusion);
```

Both effects are enabled by default in Plastic mode and skipped in Legacy mode.
The environment uses the lighting's sky/ground colors and two procedural studio
softboxes. It approximates rough reflections without external texture assets.
SSAO samples a packed camera-depth image and darkens ambient illumination near
nearby surfaces; direct lights remain unchanged. It requires fragment `highp`
and a complete framebuffer, otherwise it is skipped. `ambientOcclusionActive()`
reports the last prepared frame. Screen-space shading cannot account for hidden
or offscreen geometry and is not a substitute for global illumination.
The depth texture temporarily uses texture unit 6; the prior texture and sampler
bindings are restored. Settings have the same thread-safe snapshot behavior as
the lighting and shadows.

### Shared transparent scene items

```cpp
item->setOpacity(0.5);       // Validated [0, 1]; multiplies the draw color's alpha.
item->setDoubleSided(true); // Optional: include back faces of open/complex meshes.
item->setCastsShadow(false); // Optional: no opaque shadow or SSAO occlusion.
```

Opacity applies to scene rendering; picking and handles remain usable. Items
default to opacity 1, single-sided rendering and casting shadows. This makes
selection fading preserve an object's shadow. Use `setCastsShadow(false)` for
physically transparent surfaces. Selection fading does not simulate refraction
or colored light transmission.

Both `GraphicsView` and `PlasticGraphicsView` use
[weighted blended order-independent transparency](https://jcgt.org/published/0002/02/09/)
for items whose opacity is below 1. This handles overlapping/intersecting meshes
without sorting triangles, with depth testing against opaque geometry. Blending
is approximate, particularly for overlapping surfaces with high opacity.
It requires OpenGL 3+ and renderable/blendable RGBA16F targets (on GLES, ES 3+
with `EXT_color_buffer_float` and `EXT_float_blend`). If unavailable, items are
sorted by geometry bounds and blended without writing depth; that fallback can
show ordering artifacts for intersecting geometry.
`orderIndependentTransparencyActive()` reports use in the latest color frame.
Draw callbacks that only set a color's alpha retain their previous rendering
behavior; set item opacity to opt into the common transparent pass. Custom scene
shaders must implement `objectOpacity` and the output convention in
`TransparencyRenderer.cpp` to participate in the corresponding paths.
The temporary targets resize with the view and are released before context
destruction. Overlays, text and picking remain separate from composition.

## License
Licensed under the MIT license. see LICENSE for details.
