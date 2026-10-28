# ASKOFA Engine (native)

An **actual** engine: C++20, and the graphics are written by us — our own
shaders, our own mesh generation, our own OpenGL function loader (no GLAD,
no GLEW, no Three.js, no ready-made renderer). OpenGL 3.3 first; a Vulkan
backend slots in later behind the renderer interface.

## Status: N0

What works today:

- **Core library** (`src/core`) — hand-written math (Vec3/Mat4), a small
  JSON reader/writer written from scratch, the scene model, and procedural
  geometry (cube, sphere, cylinder, cone, torus, ground). Fully unit-tested.
- **Renderer** (`src/renderer`) — loads OpenGL functions itself, compiles
  our GLSL shaders (Blinn-Phong sun + hemisphere ambient, sRGB handling),
  uploads meshes to VAO/VBO/EBO, draws the scene.
- **App** (`src/main.cpp`) — GLFW window, orbit camera (drag / right-drag
  pan / wheel zoom), and **Play mode**: behaviors (Spin, Float, Spin+Float)
  and a Player controller (WASD + Space jump, gravity, camera follow).
  Esc stops and restores the edited scene, exactly like a real editor.
- Scenes are the same `.askofa.json` format as the web prototype, so the
  example scene in `../examples/` loads here too (pass it as an argument).

Not yet (by design — this is N0): editor panels and gizmo (N1, Dear ImGui),
shadow maps (N1), physics between objects and .glb model import (N2),
standalone game export (later).

## Build

Needs: a C++20 compiler and CMake 3.20+. GLFW is fetched automatically by
CMake on first configure (internet needed once), or use a system GLFW 3.3+.

Windows (your Cyborg — Visual Studio 2022 with the C++ workload):

```powershell
cd engine-native
cmake -S . -B build
cmake --build build --config Release
.\build\Release\askofa_engine.exe
# or open your own scene:
.\build\Release\askofa_engine.exe ..\examples\first-scene.askofa.json
```

Linux:

```bash
cd engine-native
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/askofa_engine
```

## Test

```bash
cmake -S . -B build -DASKOFA_BUILD_APP=OFF   # core only, no window needed
cmake --build build
ctest --test-dir build --output-on-failure
```

Core tests cover the math (TRS, perspective, lookAt), the JSON parser
(round-trip + junk rejection), the scene model (starter scene, duplicate,
remove, save/load) and geometry (vertex/index sanity, unit normals).

## Controls

| Input | Action |
| --- | --- |
| Left-drag | Orbit camera |
| Right-drag | Pan |
| Mouse wheel | Zoom |
| Space | Play |
| Esc (in Play) | Stop, restore edited scene |
| WASD + Space (in Play) | Move / jump the Player object |
| Ctrl+S | Save `scene.askofa.json` in the working folder |

## Layout

```
src/core/       math.hpp, json.*, scene.*, geometry.*   (testable, no GL)
src/renderer/   gl_loader.* (our GL loader), renderer.* (our shaders/draw)
src/main.cpp    window, input, orbit camera, Play-mode step
tests/          core_tests.cpp
```
