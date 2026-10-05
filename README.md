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
meshes in Plastic and CAD modes, including picking. `generateRoundedBox()` provides
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
the configuration directory). Space cycles Plastic / CAD / Legacy shading, Up/Down
changes roughness, right-drag orbits the camera, and the wheel zooms.
S toggles shadows, `[` / `]` adjusts their softness, and L animates the key light.
E toggles environment reflections and A toggles ambient occlusion.
In CAD mode, B toggles edges and `[` / `]` adjusts their width instead of shadow softness.
S toggles shadows in both CAD and Plastic modes; L moves their shared key light.
Left-click an object to attach a `TransformHandle`. Drag its arrows or planes to
move the object, or its rings to rotate it. Clicking the floor or background
clears the selection. Object transforms persist when switching shading styles,
and shadows follow the transformed objects in Plastic and CAD modes.
The selected object is shown at 50% opacity in all three styles and returns to opaque
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

### CAD edge rendering

```cpp
view.setRenderStyle(ptgl::PlasticGraphicsView::RenderStyle::CAD);
auto edges = view.edgeSettings();
edges.enabled = true;
edges.color = {0.08, 0.10, 0.13};
edges.width = 1.0;        // Default: 1 framebuffer pixel, [0.5, 4].
edges.normalAngle = 35;  // Normal discontinuity threshold in degrees, [5, 120].
view.setEdgeSettings(edges);
```

CAD uses bright ambient shading with silhouettes and sharp normal/depth
transitions. Its key light follows `PlasticLighting::keyDirection`, with a soft
camera-relative fill. Shadows affect the key light while leaving ambient and fill
illumination intact, so shaded faces remain readable. CAD and Plastic share
`ShadowSettings`, including enable, strength, softness and automatic bounds;
shadows are enabled by default. It reuses the smooth primitive geometry and shared
picking, transparency and TransformHandle paths. Plastic's environment and ambient
occlusion settings are retained for switching back to Plastic.
`cadRenderingAvailable()` reports surface shader availability; `edgesActive()`
reports whether edge composition succeeded in the last frame. Unsupported edge
targets/shaders fall back to shaded surfaces; a failed surface shader falls back
to Legacy. Settings are validated and snapshotted at frame boundaries.

The edge renderer captures packed depth and view normals in two extra scene passes
(`RenderEdgeState`), then composites edges before overlays and text. Keep draw
callbacks free of animation updates. The targets follow the viewport size and
are released with the GL context. Lit surfaces with normals participate, including
items with `castsShadow == false`. Zero-opacity items and unlit helpers are excluded.
Partially transparent items contribute their nearest surface: their visible edges
remain solid, while edges of geometry behind them are suppressed. This does not
display dashed hidden lines or trace edges through multiple transparent layers.

CAD lighting can be adjusted independently from Plastic materials:

```cpp
auto cad = view.cadSettings();
cad.brightness = 1.0; // [0,4]
cad.ambient = 0.70;   // [0,2]
cad.key = 0.36;       // [0,2], receives shadows
cad.fill = 0.10;      // [0,2]
cad.specular = 0.06;  // [0,1]
cad.shininess = 32;   // [1,256]
cad.fillDirection = {0.8, -0.2, 0.4}; // Nonzero view-space direction
view.setCadSettings(cad);
```

#### Imported meshes and explicit boundaries

`prepareCadMesh()` in `ptgl/Util/MeshProcessing.h` preprocesses indexed triangles
or triangle soups from STL/OBJ loaders. It welds coincident positions for adjacency,
removes degenerate triangles, calculates area-weighted smooth normals across soft
joints, and preserves sharp normals and UV seams. Boundary, nonmanifold and sharp
dihedral edges become explicit index pairs in `VertexSet::edges`. Triangle winding
is retained; repair inconsistent winding in the importer. The returned mesh splits
triangle corners to preserve seams, so allow additional CPU/GPU mesh memory.

```cpp
ptgl::MeshProcessingSettings options;
options.creaseAngle = 35; // [0,180] degrees, geometric edge extraction
options.weldTolerance = 1e-6; // [1e-12,1], fraction of the largest bounds dimension
options.smoothNormals = true;
auto mesh = ptgl::prepareCadMesh(loadedMesh, options);
// Optional tangent/face boundary, using indices into mesh.vertices:
mesh.edges.insert(mesh.edges.end(), {endpointA, endpointB});
// On the render thread with its context current, upload once:
renderer->registerMesh("part", mesh);
// In the ordinary scene callback, after applying the object's transform:
renderer->drawRegisteredVertices("part");
// For changing geometry, drawMesh(mesh) uploads immediately instead.
```

Explicit input edges are also preserved through preprocessing. Edges need valid
endpoint pairs and must lie on the mesh surface to pass its depth test. They are
expanded into screen-space triangles, so width does not depend on OpenGL's native
wide-line support. The nearest-surface depth suppresses hidden lines, including
behind translucent selection surfaces. Existing `drawVertex()` continues to use
screen-space edge detection; use `drawMesh()`/`registerMesh()` to include metadata.
`registerMesh(name, mesh, true)` replaces both geometry and its edge metadata.

Screen-space detection alone cannot recover coplanar part boundaries or tangent
fillet boundaries. Import face/edge metadata for those boundaries; the demo's
rounded box supplies explicit tangent seams. No CAD topology is inferred from
triangle normals beyond geometric creases. Tiny details remain resolution-limited.

#### Quality, cache and measurements

```cpp
auto quality = view.renderQualitySettings();
quality.edges = ptgl::EdgeQuality::Balanced; // Default: 2x per axis
quality.cacheStaticEdges = true; // Opt-in; default false
quality.collectTimings = true;   // Default false
view.setRenderQualitySettings(quality);
// After changes to geometry, transforms, visibility, sidedness, or zero opacity:
view.invalidateEdgeCache();
const auto stats = view.renderStatistics(); // Last completed frame
```

Fast uses 1x, Balanced 2x and High 3x edge targets per axis. Normal/depth detection
and explicit edges are composed into this target, then box-filtered to framebuffer
pixels. This reduces diagonal stair steps and motion flicker while retaining the
default 1px width. Higher quality costs approximately 4x/9x target pixels; allocation
falls back to a lower scale if resources cannot be allocated. `edgeSupersampling`
and `edgeTargetWidth/Height` report the scale and dimensions actually used.

Static caching reuses the depth/normal captures and explicit edge commands.
Camera, projection and target-size changes trigger recapture automatically.
Applications must call `invalidateEdgeCache()` when scene content changes, including
geometry drawn directly by callbacks. Lighting, edge width/color/angle, and selection
opacity changes that stay above zero do not require recapture. Keep caching disabled
for animated content unless invalidation is provided. The demo invalidates after
TransformHandle changes, and retains cached surfaces while the light moves.

`edgeDrawCalls` and `edgeTriangles` count geometry submitted during the two capture
passes; both are zero when reused. They exclude feature-line composition and other
render passes. `edgeCpuMilliseconds` covers edge allocation/capture/composition CPU
submission, excluding the enclosing frame. `edgeGpuMilliseconds` is an asynchronous
desktop GPU timer sample, identified by `edgeGpuSampleFrame`; it never waits for the
GPU and is `-1` until available. GLES or unsupported timer contexts provide CPU
measurements only. Disabling timing avoids query overhead. The demo's Q cycles edge
quality, C toggles caching, P shows timing, and comma/period adjust CAD brightness.

A local measurement on NVIDIA RTX A4000 (OpenGL 4.6, 640x480, 405,000
triangles in a registered mesh, shadows disabled, 12 warmup frames and 30 measured
frames) gave the following mean edge-pass times. These exclude the rest of the
frame and vary with hardware, resolution and geometry.

| Edge quality | CPU, uncached / cached | GPU, uncached / cached |
| --- | --- | --- |
| Fast (1x) | 0.047 / 0.026 ms | 0.127 / 0.049 ms |
| Balanced (2x) | 0.049 / 0.023 ms | 0.237 / 0.122 ms |
| High (3x) | 0.045 / 0.027 ms | 0.414 / 0.247 ms |

The GLFW driver uses framebuffer pixels for rendering, picking and pointer events,
converting GLFW cursor coordinates on HiDPI displays. `setWindowSize()` still uses
GLFW screen units. An edge width of 1 means one physical framebuffer pixel.

`PlasticGraphicsView` retains its public API and nested `RenderStyle` name (also
available as `ptgl::RenderStyle`). It snapshots settings and coordinates the view;
`SceneStyleRenderer` selects the style, `PlasticSurfaceRenderer` and
`CadSurfaceRenderer` own surface shading, and `CadEdgeRenderer` owns edge passes.
Shadows, picking, transparency and overlays remain shared. Rebuild consumers after
updating because `VertexSet` and renderer class layouts have changed.

## License
Licensed under the MIT license. see LICENSE for details.
