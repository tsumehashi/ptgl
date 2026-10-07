Install and activate the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html),
plus CMake 3.13+, Ninja and Eigen. This sample requires WebGL 2.

From the project root:

```sh
emcmake cmake -S . -B build/emscripten -G Ninja -DCMAKE_BUILD_TYPE=Release -DEIGEN_DIR="<Eigen include directory>" -DPTGL_BUILD_EMSCRIPTEN_DEMO=ON
cmake --build build/emscripten --parallel
python -m http.server 8000 --bind 127.0.0.1 --directory build/emscripten
```

Open [SimpleEmscriptenDemo](http://localhost:8000/examples/SimpleEmscriptenDemo/SimpleEmscriptenDemo.html).
Keep its `.html`, `.js` and `.wasm` files together; use HTTP rather than opening
the HTML file directly.

The sample can also be configured on its own with
`emcmake cmake -S examples/SimpleEmscriptenDemo -B build/emscripten-simple -G Ninja -DEIGEN_DIR="<Eigen include directory>"`.

To build the Plastic/CAD scene editor as well, add `-DPTGL_BUILD_PLASTIC_DEMO=ON`
to the root configuration. See the [browser build notes](../../README.md#browser-samples-emscripten--webgl-2)
for driver behavior and platform limitations.
