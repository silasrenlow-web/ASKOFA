# ASKOFA

A useful kit for developers and people to use for daily.

## First software: ASKOFA Engine

A **lightweight but powerful 3D game engine** with a 3D editor, built to be
super easy to learn — and built like an actual engine: **C++20, with the
graphics written by us** (our own shaders, meshes, and OpenGL loader; no
ready-made renderer). OpenGL first, a Vulkan backend later.

- Native engine + build/test instructions: [`engine-native/`](engine-native/)
- Example scene (loads in the engine): [`examples/first-scene.askofa.json`](examples/first-scene.askofa.json)

### Roadmap

- **N0 (now):** core library (math / JSON / scene / procedural geometry,
  unit-tested), our OpenGL renderer, GLFW app with orbit camera and Play
  mode (behaviors + WASD player controller).
- **N1:** the editor — panels, hierarchy, inspector, transform gizmo,
  shadow maps.
- **N2:** physics between objects, .glb model import, scripting.
- **Later:** standalone game export, terrain, particles, sound, Vulkan
  backend.

The editor was first sketched as a quick web prototype; the native engine
above is the real direction.

License: GPL-3.0 (see LICENSE).
