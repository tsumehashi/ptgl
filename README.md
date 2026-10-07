# ptgl
ptgl is a C++ graphics library for prototyping.

![screen_shot](https://raw.githubusercontent.com/wiki/tsumehashi/ptgl/images/readme/ScreenshotSimpleDemo.png "screen_shot")

## Installation
### Requirements
* Ubuntu 18.04
* C++17 compiler and standard library with `std::filesystem` support (GCC 9+ on Ubuntu)
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

## Rendering styles

Use `QuickStyledGraphicsView` instead of `QuickGraphicsView` with the same
driver and render callbacks. For subclass-based applications, derive from
`StyledGraphicsView` instead of `GraphicsView`, including
`ptgl/Core/StyledGraphicsView.h`. These views support Plastic (the default), CAD
and Legacy rendering. `QuickStyledGraphicsView` is defined in `QuickGraphicsView.h`.

```cpp
#include "ptgl/Core/QuickGraphicsView.h"
#include "ptgl/Driver/GLFWGraphicsDriver.h"

ptgl::QuickStyledGraphicsView view(std::make_unique<ptgl::GLFWGraphicsDriver>());
view.setDefaultMaterial({0.35, 0.04}); // roughness, dielectric reflectance
view.setRenderSceneFunction([](ptgl::Renderer3D* r) {
    r->setColor(0.8, 0.3, 0.2);
    r->drawSphere({0, 0, 0}, r->R0(), 1.0);
    // Optional per-object override; also supported by Render3DItem recordings.
    r->setMaterial({0.65, 0.04});
    r->drawCylinder({0, 3, 0}, r->R0(), 2.0, 0.8);
});
// May also be called while running; applies at the next frame boundary.
view.setRenderStyle(ptgl::StyledGraphicsView::RenderStyle::Legacy);
view.setRenderStyle(ptgl::StyledGraphicsView::RenderStyle::Plastic);
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
The demo uses the shared `ObjectScene` editor and `GraphicsItem::setOpacity()` API. Its shadow volume
automatically follows moved and rotated objects.
Press `1` to add a sphere, `2` to add a rounded box, `3` to add a cylinder,
or `4` to add a cube with sharp corners (side length 1.5). `5` adds a rounded
cylinder and `6` adds a rounded cone. The objects use `ObjectScene::addPrimitive()`
with inspectable shape parameters and reusable meshes.
New objects use an unoccupied grid position near the scene origin and are selected
automatically, ready to move with the handle. `Delete` removes the selected object
and clears its handle; without a selection it does nothing. Holding an add/delete
key does not repeat the operation. Adding or deleting during a handle drag ends
that gesture. The HUD shows the current object count. These controls work in all
three rendering styles, and scene changes refresh shadows, picking and cached edges.
Drop an STL or OBJ file onto the window to add and select a mesh; the camera fits
the imported object. Failed imports leave the scene intact and report the error
in the HUD. X toggles a horizontal section, F toggles its cap, K reverses the
retained side, and PageUp/PageDown moves the plane by 0.1 along world Z. The HUD
also shows the selected mesh's section status, including cap failures.
If GLFW is not discovered automatically, set `GLFW_INCLUDE_DIR` and
`GLFW_LIBRARY`. Windows shared builds also need the ptgl, GLEW and GLFW DLL
directories on `PATH`. The example selects desktop OpenGL through the existing
`PTGL_DISABLE_GLES` driver option.

### Object selection and editing

`GraphicsView::objectScene()` enables the reusable object editor on any ordinary
or Plastic/CAD view. It owns a `TransformHandle` and the objects added through it;
existing view callbacks remain available. Include `ptgl/Core/ObjectScene.h`:

```cpp
auto& scene = view.objectScene();
auto box = scene.addObject("Rounded box", {0, 0, 0.75}, [](ptgl::Renderer3D* r) {
    r->setColor(0.16, 0.52, 0.83);
    r->drawRoundedBox(r->p0(), r->R0(), {1.5, 1.5, 1.5}, 0.18);
});
scene.selectObject(box); // Optional: clicking also selects automatically.
scene.setSelectionOpacity(0.5); // Multiply the original opacity while selected.
scene.transformHandle()->setEnableRotate(false); // Translation only.
box->transform()->setPosition({1, 0, 0.75});
// scene.removeSelectedObject();
// scene.clear();
```

Draw callbacks use local coordinates. The object's transform applies equally to
normal rendering, picking, shadows and CAD edges. `objects()` and `selectedObject()`
expose the current state. Click a handle to manipulate the current object; clicking
the background or an unmanaged item clears selection. Selection runs before the
view's picking callback. `selectObject(nullptr)` clears it programmatically;
`setEditingEnabled(false)` clears it and stops automatic selection.

Selection uses half the object's original opacity by default and restores the
original value on deselection or removal. `setSelectionOpacity(1)` disables fading.
Changing or removing the selection ends any active handle gesture, so a held mouse
button cannot move a newly selected object. Translation/rotation enable settings
survive selection changes. Direct `view.removeGraphicsItem(object)` also clears
the editor's selection and object list. `removeObject()` and `removeSelectedObject()`
return false when there is nothing to remove. Hiding/disabling a selected object,
or making it unpickable, clears selection before the next rendered frame.

Adding/removing items, visibility/enabled state, double-sided state and changes
to/from zero opacity automatically invalidate cached CAD edges. Managed object
positions and rotations (including changes through a shared `Transform`) are
checked before each frame's scene passes. Replacing a draw callback through
`setDrawFunction()` also invalidates edges; call `notifyGeometryChanged()` when
changing geometry captured by that callback. Other custom rendering can call
`view.notifySceneChanged()` or `invalidateEdgeCache()` on Plastic views.

Object editing and item mutations run on the view/event thread, or before starting
the view. Render callbacks should only draw: change the object list from input or
frame callbacks. `ObjectScene` belongs to its view; retained objects are detached
when the view is destroyed. Shape choices, spawn placement, shortcut keys and HUD
text remain application policy; see `examples/PlasticDemo/main.cpp` for an example.

### Objects with shape and material properties

`ObjectScene` also creates objects whose geometry and appearance can be inspected
and edited without replacing a draw callback:

```cpp
auto& scene = view.objectScene();
auto box = scene.addPrimitive("Housing",
    ptgl::RoundedBoxShape{{2.0, 1.2, 1.0}, 0.12}, {0, 0, 0.5});
box->setColor(0.16, 0.52, 0.83); // sRGB components in [0,1]
box->setMaterial({0.4, 0.04});   // roughness, dielectric reflectance
box->setOpacity(0.8);

auto shape = std::get<ptgl::RoundedBoxShape>(*box->shape());
shape.sides[0] = 2.5;
box->setShape(shape); // Keeps the pose, color, material and selection.
auto worldBounds = box->worldBounds();
box->clearMaterial(); // Inherit the current/view material again.
```

`PrimitiveShape` is a variant of seven types defined in
`ptgl/Core/PrimitiveShape.h`: `BoxShape`, `SphereShape`, `CylinderShape`,
`ConeShape`, `RoundedBoxShape`, `RoundedCylinderShape` and `RoundedConeShape`.
Dimensions are full side lengths or full Z length; radii and segment counts follow
the rounded primitive rules below. Each type is centered at the local origin.
Their meshes stay the same across Legacy, Plastic and CAD styles.

`shape()` returns an optional shape description; imported/custom meshes have no
primitive description. `mesh()` exposes a shared, immutable CPU mesh for either
kind of geometry. `localBounds()` and `worldBounds()` describe the full geometry,
before section clipping; callback-only objects have empty bounds. `color()` and
`material()` expose the stored appearance, while `transform()` and the existing
opacity API remain available. Object materials require finite roughness in
`[0.12,1]` and reflectance in `[0,1]`; invalid shape/color/material input throws.

`setShape()` and `setMesh()` replace geometry only after successful validation.
They update bounds and invalidate CAD edges automatically. `setDrawFunction()`
switches back to callback rendering and clears the stored geometry/bounds. A
callback may override an object's color or material with renderer calls.

### Imported and custom mesh objects

```cpp
auto imported = scene.loadMesh("Part", "models/part.stl");
imported->setColor(0.7, 0.75, 0.8);
imported->transform()->setPosition({2, 0, 1});
scene.selectObject(imported);

ptgl::VertexSet geometry = ptgl::generatePrimitiveMesh(ptgl::ConeShape{2.0, 0.8});
ptgl::MeshProcessingSettings processing;
processing.creaseAngle = 40;
auto custom = scene.addMesh("Custom", geometry, processing);
// custom->setMesh(updatedGeometry, processing);
```

`loadMesh()` accepts ASCII/binary STL and OBJ with case-insensitive extensions
and UTF-8 paths. OBJ faces are triangulated and groups are combined into one
object; textures, material libraries and per-face colors are not imported. Units
and coordinates are preserved. Binary STL counts are checked against the file
length before allocating vertices; malformed/truncated data is rejected.

Both `addMesh()` and `loadMesh()` use `prepareCadMesh()` to remove degenerate
triangles, prepare normals and extract feature edges. They accept indexed
triangles or triangle soup and copy the input into immutable storage. Invalid,
empty or failed imports throw an exception before adding anything to the scene.
Mesh replacement also preserves the previous geometry on failure. Objects use
the existing selection, transform handle, transparency, shadow and CAD paths.

For direct rendering, `Renderer3D::drawSharedMesh()` accepts a
`std::shared_ptr<const VertexSet>` and uploads it once per renderer. Reusing a
mesh, moving its object or changing its color does not upload it again. Cached
buffers are reclaimed at a render-frame boundary after the last CPU owner is
released, or when the renderer is finalized.

### Section planes and caps

`StyledGraphicsView` supports one world-space section plane in all three styles:

```cpp
ptgl::SectionSettings section;
section.enabled = true;
section.point = {0, 0, 0.8};
section.normal = {0, 0, 1};
section.keepPositiveSide = false; // Retain z <= 0.8 in this example.
section.capEnabled = true;
section.capColor = {1.0, 0.65, 0.2};
view.setSectionSettings(section);
// section.enabled = false; view.setSectionSettings(section);
```

The plane normal can point in any direction and is normalized on assignment.
Nonfinite values, a zero normal and cap colors outside `[0,1]` are rejected.
`sectionSettings()` retrieves the current settings; changes apply at the next
frame boundary. The renderer also exposes the same setters for direct use.

Clipping affects standard CPU-backed triangle drawing, including primitive draw
callbacks and mesh objects. Display, picking, shadows, transparency and CAD edges
use the same clipped surface and cap; transform handles, overlays and line
primitives remain intact. Original mesh data and object bounds are preserved.
Raw OpenGL buffer changes outside the library's mesh upload API are not tracked.

Caps support concave contours, holes and disconnected sections. They require a
closed, consistently wound manifold source mesh. Open/nonmanifold meshes or
invalid contours still show the clipped surface, with no cap. Inspect the most
recent mesh draw using `SceneObject::sectionStatus()` or
`Renderer3D::lastSectionStatus()`; `sectionStatusMessage()` provides a readable
description. Results include `Disabled`, `Unchanged`, `Empty`, `Clipped`, `Capped`,
`OpenMesh`, `InvalidContour` and `InvalidTransform`. A singular/nonfinite object
transform reports `InvalidTransform` and leaves that draw uncut.

The CPU-only `sectionMesh()` function in `ptgl/Util/MeshSection.h` returns separate
surface/cap meshes and a status without modifying its input. Its plane uses the
input mesh's coordinates. Clipping uses a tolerance relative to mesh size;
features near that tolerance may merge or fail cap generation. This provides
mesh sections, not exact CAD solid operations.

Mesh buffers retain CPU triangle data for clipping, increasing memory use.
The renderer caches up to 32 mesh/plane combinations; changing a plane or moving
an object can require CPU clipping again. Cost grows with mesh complexity, so
large imported meshes may need simplification for interactive plane movement.

### GUI controls and scene editor

The reusable scene editor works with `GraphicsView`; its rendering-settings page
requires `StyledGraphicsView`. Include `ptgl/GUI/SceneEditorPanel.h`:

```cpp
auto editor = std::make_shared<ptgl::gui::SceneEditorPanel>(view);
editor->setPos(900, 10);
editor->setSize(340, 700);
view.addGraphicsItem(editor);
editor->setPage(1); // 0: objects, 1: properties, 2: rendering
```

The objects page adds any of the seven primitive types, imports an STL/OBJ path,
selects, hides/shows and deletes objects. `setOnAddObjectFunction()` lets an
application choose its own spawn positions. Properties include the name,
position, XYZ Euler angles in degrees, dimensions, fillet radius, color, opacity
and optional material override. Mesh objects retain their imported geometry.
Invalid geometry edits leave the object intact and display an error. Values
follow scene selection and transform-handle changes; unfinished text edits are
not overwritten during refresh. The view must outlive its editor panel.

The rendering page controls style, CAD edges/width/brightness, shadows/softness,
environment lighting, ambient occlusion and the section plane's point, normal,
retained side, cap and cap color. It also provides Classic/Dark/Light GUI themes and a
scale control. The demo docks this editor on the right and keeps its keyboard
shortcuts and mesh file-drop controls.

For application-specific panels, use `ptgl/GUI/Controls.h` and
`ptgl/GUI/Layout.h`:

```cpp
auto panel = std::make_shared<ptgl::gui::Panel>("Settings");
auto form = std::make_shared<ptgl::gui::FormLayout>();
auto roughness = std::make_shared<ptgl::gui::DoubleSpinBox>();
roughness->setRange(0.12, 1.0);
roughness->setStep(0.02);
roughness->setDecimals(2);
roughness->setValue(0.35, false);
roughness->setOnValueChangedFunction([&](double value) {
    auto material = view.defaultMaterial();
    material.roughness = value;
    view.setDefaultMaterial(material);
});
form->addRow("Roughness", roughness);
panel->add(form);
panel->setPos(10, 10);
panel->setSize(320, 160);
view.addGraphicsItem(panel);
```

* `DoubleSpinBox` supports finite decimal values, step/precision settings,
  arrow buttons, Up/Down keys and the wheel. Invalid text restores the last value.
* `DoubleSlider` supports finite ranges, pointer dragging, wheel/arrow steps and
  Home/End. Equal endpoints produce a fixed value.
* `CheckBox`, `ComboBox` and `ColorPicker` provide toggles, dropdown choices and
  RGB sliders. Popups render and receive input above ordinary controls.
* `Layout` arranges vertically or horizontally with padding, spacing and stretch;
  `FormLayout` aligns labels/editors. `Widget::setPreferredSize()` and
  `setMinimumSize()` use logical pixels. `ScrollArea` clips both child rendering
  and hit regions, and reveals focused children during Tab navigation.
* `Theme` centralizes colors and metrics for the new controls and buttons/text
  fields. `setTheme()` on a parent is inherited by descendants. `Theme::light()`
  selects the light palette and `Theme::dark()` selects the dark palette. The default
  `Theme{}` preserves the original translucent gray panels, white fields, black text,
  pale pink checked buttons and pale blue editing fields. `scale` converts logical GUI metrics to framebuffer
  pixels. Explicit `setPos()`/`setSize()` coordinates remain framebuffer pixels.

GUI changes belong on the view/event thread, or before execution. GUI pointer
events are consumed before camera controls and scene picking. Using a panel does
not deselect the edited 3D object. Consumed keys do not reach the view's shortcut
callback. Tab/Shift+Tab moves focus, while removal, hiding and window focus loss
clear invalid focus/capture. Releasing a button outside its bounds cancels its
click. Child visibility/enabled/pickable states now inherit the parent's effective
state without overwriting the child's own setting.

GLFW input is queued in arrival order, including key press/repeat/release events,
so applications should check `KeyEvent::keyAction()` in shortcut callbacks.
Text editing uses GLFW's character events instead of deriving characters from
physical keys. It supports UTF-8 codepoint movement/deletion, mouse/Shift selection,
Ctrl/Cmd+A/C/X/V, Enter to commit and Escape to cancel. This is single-line editing;
combined grapheme clusters are not treated as a single editing unit. Older drivers
without character events retain the ASCII-key fallback.

On Windows, `GLFWGraphicsDriver` includes IME preedit text, composition cursor,
native candidate positioning and committed text handling through the
[Windows IME composition messages](https://learn.microsoft.com/en-us/windows/win32/intl/wm-ime-composition).
Other platforms use GLFW committed-character input; native preedit/candidate
integration is currently Windows-specific. The GLFW driver also supplies the
system clipboard. Custom drivers can override the corresponding protected
`GraphicsDriver` methods.

Japanese text needs a font containing Japanese glyphs. Call
`view.loadGuiFont(utf8Path)` from `initProcess()` or the view thread with a current
GL context; it returns false on load failure. TrueType fonts and the first face
of TrueType collections are supported. The Windows demo uses the installed
Meiryo font when available; no system font is bundled. Text measurement uses the
loaded font's actual advances, including proportional and multibyte text.

`ObjectScene::objectOpacity()` / `setObjectOpacity()` read and update an object's
base opacity while keeping selection fading separate. Use these for editor
controls that must preserve opacity after deselection.

Rebuild the library and applications together: GUI, driver and view class layouts
and virtual interfaces have changed.

### Rounded primitives

`Renderer3D` supports rounded solids directly in Legacy, Plastic and CAD modes.
Position, rotation and the transform stack work like `drawBox()`/`drawCylinder()`.
Rounding is part of the geometry, so picking, shadows and transparent selection
all use the same surface.

```cpp
r->drawRoundedBox({0, 0, 0.75}, r->R0(), {1.5, 1.5, 1.5}, 0.18);
r->drawRoundedBox({0, 2, 0.5}, r->R0(), {2.0, 1.2, 1.0}, 0.12);
r->drawRoundedCylinder({2, 0, 0.8}, r->R0(), 1.6, 0.72, 0.18);
r->drawRoundedCone({2, 2, 0.9}, r->R0(), 1.8, 0.82, 0.14);
// Optional final argument controls tessellation (default 6, range [1,64]):
r->drawRoundedCylinder({-2, 0, 0.8}, r->R0(), 1.6, 0.72, 0.18, 12);
```

| Method | Size parameters | Rounded parts | Fillet-radius range |
| --- | --- | --- | --- |
| `drawRoundedBox(pos, R, sides, fillet, segments)` | Full XYZ side lengths | All twelve edges and eight corners | `[0, min(sides)/2]` |
| `drawRoundedCylinder(pos, R, length, radius, fillet, segments)` | Full Z length and outer radius | Top and bottom rims | `[0, min(radius, length/2)]` |
| `drawRoundedCone(pos, R, length, radius, fillet, segments)` | Sharp cone's Z length and base radius | Base rim and tip | `[0, radius*length/(hypot(length,radius)+radius)]` |

Dimensions must be positive, finite and representable as floats. Invalid dimensions,
fillet radii or segment counts throw `std::invalid_argument` on drawing/generation.
Zero fillet gives sharp edges. All shapes are closed, centered on the Z axis; the
box and cylinder preserve their outer dimensions. The cone is rounded **inside its
original sharp envelope**, with its base at `-length/2`: its base perimeter contracts
and its rounded tip lies below `+length/2`. At the maximum cone fillet, the result
is its inscribed sphere. The cylinder becomes a capsule when the fillet reaches
its radius, and a sphere when its full length also equals its diameter.

`segments` controls fillet detail. Revolution shapes use at least 64 circumferential
segments, increasing to `8 * segments` for higher settings. Smooth analytic normals
are included, along with tangent-boundary metadata for CAD edges. These edges obey
the view's edge enable/color/width settings; ordinary `drawVertex()` callers may
ignore metadata as before.

The renderer caches the 16 most recently used rounded geometry parameter sets in
GPU buffers, shared across frames and render passes. Position/rotation/color do
not create separate meshes. Eviction and view finalization release the buffers.
For many different shapes, applications can generate and register meshes explicitly
using `PrimitiveShapeVertex::generateRoundedBox()`, `generateRoundedCylinder()` or
`generateRoundedCone()`, then `registerMesh()`/`drawRegisteredVertices()`. Call
`invalidateEdgeCache()` after changing geometry if static edge caching is enabled.

`Render3DItem` provides the same three methods with pointer-based position/rotation
arguments, including optional `segments`. Recording serialization retains these
parameters and replays through the standard APIs; geometry validation occurs during
playback. Existing command IDs are preserved. Older library versions cannot read
the newly appended rounded-shape commands. Rebuild consumers after updating the
library because the renderer's class layout changed.

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

Both `GraphicsView` and `StyledGraphicsView` use
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
view.setRenderStyle(ptgl::StyledGraphicsView::RenderStyle::CAD);
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
fillet boundaries. Import face/edge metadata for those boundaries; the built-in
rounded primitives supply their tangent seams automatically. No CAD topology is inferred from
triangle normals beyond geometric creases. Tiny details remain resolution-limited.

#### Quality, cache and measurements

```cpp
auto quality = view.renderQualitySettings();
quality.edges = ptgl::EdgeQuality::Balanced; // Default: 2x per axis
quality.cacheStaticEdges = true; // Opt-in; default false
quality.collectTimings = true;   // Default false
view.setRenderQualitySettings(quality);
// After changes to geometry captured by custom render callbacks:
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
Item addition/removal, visibility, enabled state, sidedness and changes to/from zero
opacity also trigger recapture automatically. `ObjectScene` tracks its objects'
transforms, including TransformHandle edits. For other changing geometry/transforms
drawn directly by callbacks, call `invalidateEdgeCache()` or `notifySceneChanged()`.
Lighting, edge width/color/angle, and opacity changes that stay above zero do not
require recapture. Keep caching disabled for animated content unless invalidation
is provided. The demo retains cached surfaces while the light moves.

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

`StyledGraphicsView` exposes the rendering settings and nested `RenderStyle` name
(also available as `ptgl::RenderStyle`). It snapshots settings and coordinates the view;
`SceneStyleRenderer` selects the style, `PlasticSurfaceRenderer` and
`CadSurfaceRenderer` own surface shading, and `CadEdgeRenderer` owns edge passes.
Shadows, picking, transparency and overlays remain shared. Rebuild consumers after
updating because `VertexSet` and renderer class layouts have changed.

## License
Licensed under the MIT license. see LICENSE for details.
