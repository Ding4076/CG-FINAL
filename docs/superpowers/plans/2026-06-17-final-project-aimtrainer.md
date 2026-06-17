# AimTrainer (Final Project) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a self-contained OpenGL 3.3 first-person aim-training game (`AimTrainer`) under `projects/finalproject/` that satisfies all 7 course base requirements + 4 bonus requirements (NURBS, real-time shadows, collision, complete 3D game).

**STATUS (handoff marker):**
- ✅ Task 0 DONE (skeleton + triangle builds & runs; verified NVIDIA RTX 4060 / OpenGL 3.3)
- ✅ Task 1 DONE (hand-written OBJ loader+export; selftest PASS; sphere.obj loads & renders)
- ✅ Task 2 DONE (6 parametric primitives + Blinn-Phong renderer)
- ✅ Task 3 DONE (multi-light: dir+point+spot+hemisphere; ImGui edit panel)
- ✅ Task 4 DONE (FPS + Orbit cameras, Tab switch, zoomToFit, IME-immune keys)
- ✅ Task 5 DONE (shadow mapping: directional + spotlight, PCF 3x3)
- ✅ Task 6 DONE (hand-written NURBS, clamped + periodic, seamless loop)
- ✅ Task 7 DONE (collision: sphere-AABB player vs railing/crates/walls; sphere-sphere wild repulsion; ramps+platform with height system)
- ✅ Task 8 DONE (gameplay: Standard + Wild modes, pistol/sniper weapons, ray-sphere hit, particle bursts, scoring + in-memory leaderboard, round-over modal, F1 pause menu, scope zoom + circular mask, top-center 0.01s timer, fullscreen toggle, mouse-sensitivity slider, Exit Game button)
- ⏳ Task 9 PENDING (OBJ-sequence opening animation + F2 screenshot) — base req 7
- ⏳ Task 10 PENDING (skybox, OBJ export UI, polish, report)
- Build: `cmake --preset default` then `cmake --build out/build/default --config Release --target finalproject`
- Run from the exe dir (double-click works): `out/build/default/projects/finalproject/Release/finalproject.exe`
- Known gotcha: this machine's bash PATH has no `cmake`/`ninja`; use the VS-bundled cmake at `/c/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe` and the `default` (VS generator) preset.
- CMake note: GLOB covers source/*.cpp + source/game/*.cpp; selftest.cpp excluded via list(FILTER). media/ copied POST_BUILD next to exe; asset path is relative "media/" (no absolute paths in source). **NEW files require a `cmake --preset default` reconfigure** (GLOB is cached; build alone won't pick up new .cpp files).
- miniaudio.h is present (fetched); audio code + load() calls are wired. Needs wav files (see TODO).

**REMAINING WORK (TODO for the next session/contributor):**
- **SFX (no assets yet):** drop `pistol.wav`, `sniper.wav`, `pop.wav` into
  `projects/finalproject/media/sounds/` (CC0 from pixabay.com/sound-effects ,
  freesound.org, or opengameart.org). Code already loads + plays them; missing
  files fail silently. (User wanted a CS2-USP-like silenced pistol sound.)
- **Weapon viewmodel + fire animation + textures:** no art assets yet. See the
  'Textures / materials' section below for how to add albedo PNGs and wire them.
- **Task 9:** OBJ-sequence opening animation (read frame_XXX.obj, swap mesh) +
  F2 screenshot (glReadPixels -> PNG via stb_image_write). base req 7.
- **Task 10:** skybox, 'export scene to .obj' button, difficulty tuning, report.
- See 'Known issues to fix later (logged)' below for the bug list.

**Textures / materials (for a later contributor):**
- The renderer uses Blinn-Phong (one diffuse/albedo texture via `uTexture`), NOT
  PBR. A downloaded PBR material pack unzips to 5-7 maps (Albedo/Color, Normal,
  Roughness, Metallic, AO, Height) -- only the **Albedo/Color/Diffuse** map is
  used here. Keep just that one PNG/JPG in `media/textures/` and reference it by
  filename; the others (Normal/Roughness/Metallic/AO) are unused until a PBR
  path is added (see the pbr_viewer project for how).
- Weapons viewmodel / crate / balloon textures are NOT yet wired in (no asset on
  hand). To add them: drop the albedo PNG into `media/textures/`, then load via
  `ImageTexture2D(getAssetFullPath("textures/<name>.png"))`, bind it on the
  material's `mapKd`, and set `uHasTexture=true` in the draw call.
- Good CC0 sources: cc0-textures.com, opengameart.org (CC0 filter),
  freepbr.com, texturecan.com. All free for commercial use, no attribution.
- Optional enhancement: implement a simple PBR shader (cook-torrance BRDF,
  metallic/roughness maps) for more realistic weapon/crate surfaces.

**Known issues to fix later (logged):**
- LIGHTING STACKING: the spot light and the hemisphere/ambient light do not add
  up correctly — the spot light does not illuminate the areas that the ambient
  light leaves in shadow (spot contribution is masked or the ambient term is too
  strong). Investigate in blinn_phong.frag: ambient is added unconditionally and
  the spot's cone/attenuation may be under-applying. Fix in a later pass.
- BACK FACES TOO DARK: the crates' back (unlit) faces are pure black. Needs more
  indirect/bounce light — add an ambient-occlusion-free ambient raise, a second
  fill/hemisphere light from the opposite side, or a simple ambient-term boost so
  shadowed faces are not fully black. (blinn_phong.frag ambient term.)
- WILD MODE randomization: currently the Wild-mode NURBS paths are fixed (just
  offset copies of the same control points) and a popped ball respawns near its
  old spot. Make each path's control points RANDOM on round start, and respawn
  popped balls at a random position/phase so they don't reappear locally. (resetRound + the per-target path build + the on-hit respawn.)
- SPAWN IN VIEW: randomly-spawned targets (Standard respawn + Wild) can appear
  OUTSIDE the camera's view frustum. On spawn/respawn, project candidate points
  into clip space and only accept ones inside the frustum (and not behind the
  player / not occluded by crates). (spawnTarget + Wild respawn.)
- Spawn/collision tuning is ongoing (crate/ramp/railing dimensions).

**Architecture:** A standalone CMake project with `source/base/` (framework copied from `projects/base/`, with a hand-written OBJ loader replacing tinyobjloader), `source/game/` (game logic), `media/` (assets), and `external/` (physically copied third-party libs, minus tinyobjloader, plus miniaudio). The game subclasses `Application`, runs a multi-pass renderer (shadow map → Blinn-Phong → skybox → NURBS debug → particles → weapon viewmodel → ImGui HUD), and drives two modes (Standard single-static-target, Wild multi-NURBS-target with sphere-sphere collision).

**Tech Stack:** C++17, CMake ≥3.15, OpenGL 3.3 core, GLFW, glad, GLM, Dear ImGui, stb_image, miniaudio. Builds on the existing course framework (`projects/base/`).

---

## How to read this plan (IMPORTANT — testing model)

This is a **graphics/CMake project**, not a typical Python unit-tested codebase. There is no test runner. Verification is a **build + run + visual/behavioral check** loop. Conventions:

- **Build command** (from repo root, first time): `cmake -B build -S .` then `cmake --build build --config Release --parallel 8`
- **Rebuild after code changes:** `cmake --build build --config Release --parallel 8`
- **Run:** the executable lands at `build/bin/Release/finalproject.exe` (root build) or `build/bin/Release/FinalProject.exe` (standalone). Assets resolve via the absolute `AIMTRAINER_MEDIA` path baked at configure time (Task 0 Step 9), so the working directory does **not** matter — run it from anywhere.
- **"Test" step = Acceptance check:** build succeeds + run + observe the stated visual/behavioral result. Each task states exactly what to look for.
- **Pure-logic units (OBJ parsing, NURBS eval, collision math)** get a tiny `#ifdef AIMTRAINER_SELFTEST` block or a separate throwaway test target that asserts values and prints PASS/FAIL — these can be verified without a window.
- **Commit after every task.** This repo's remote is the TA's — do NOT push; commits stay local.

> The plan uses the **root CMake** to build `finalproject` (the root `CMakeLists.txt` auto-globs every `projects/*` dir with a `CMakeLists.txt`). This keeps one build for all projects. The `projects/finalproject/CMakeLists.txt` is written so it is ALSO independently buildable (`cmake -B build -S projects/finalproject`) to satisfy the TA's "from scratch" requirement — see Task 0.

---

## File Structure

```
projects/finalproject/
├── CMakeLists.txt                 # Task 0 — standalone + root-includeable
├── source/
│   ├── base/                      # Task 0 — copied & trimmed from projects/base/
│   │   ├── application.{h,cpp}    # copied
│   │   ├── camera.{h,cpp}         # copied
│   │   ├── transform.{h,cpp}      # copied
│   │   ├── glsl_program.{h,cpp}   # copied
│   │   ├── texture2d.{h,cpp}      # copied
│   │   ├── texture_cubemap.{h,cpp}# copied
│   │   ├── skybox.{h,cpp}         # copied
│   │   ├── framebuffer.{h,cpp}    # copied
│   │   ├── fullscreen_quad.{h,cpp}# copied
│   │   ├── bounding_box.h         # copied
│   │   ├── frustum.h              # copied
│   │   ├── plane.h                # copied
│   │   ├── input.h                # copied
│   │   ├── vertex.h               # copied
│   │   ├── light.h                # copied
│   │   ├── gl_utility.h           # copied
│   │   ├── frame_rate_indicator.h # copied
│   │   ├── obj_loader.{h,cpp}     # Task 1 — NEW, hand-written OBJ parse+export
│   │   └── model.{h,cpp}          # Task 1 — copied then rewritten to use obj_loader
│   ├── primitives.{h,cpp}         # Task 2 — parametric primitive mesh generation
│   ├── nurbs.{h,cpp}              # Task 6 — hand-written NURBS curve
│   ├── collision.{h,cpp}          # Task 7 — sphere-AABB + sphere-sphere
│   ├── game/
│   │   ├── aim_trainer.{h,cpp}    # Task 0/2 — main Application subclass
│   │   ├── fps_camera.{h,cpp}     # Task 4
│   │   ├── orbit_camera.{h,cpp}   # Task 4
│   │   ├── target.{h,cpp}         # Task 8
│   │   ├── weapon.{h,cpp}         # Task 8
│   │   ├── particle_system.{h,cpp}# Task 8
│   │   ├── obj_sequence.{h,cpp}   # Task 9
│   │   ├── audio.{h,cpp}          # Task 8
│   │   ├── leaderboard.{h,cpp}    # Task 8
│   │   └── game_state.{h,cpp}     # Task 8
│   └── main.cpp                   # Task 0
├── media/
│   ├── models/                    # .obj models + opening-animation sequence
│   ├── textures/                  # .png (weapons/, balloon/, scene/)
│   ├── sounds/                    # .wav (pistol, sniper, scope, pop)
│   ├── shaders/                   # (build-copied)
│   └── config/                    # scores.json, scene layout
└── external/                      # Task 0 — physical copy, no tinyobjloader, +miniaudio
    ├── glad/  glfw/  glm/  imgui/  stb/  miniaudio/
```

**Responsibility of each NEW file:**
- `obj_loader.{h,cpp}` — pure data: parse `.obj` text → `{positions, normals, texcoords, indices}`; export mesh → `.obj` text. No OpenGL.
- `model.{h,cpp}` — owns GL resources (VAO/VBO/EBO), uploads `Vertex` array, draws. Uses `obj_loader` for file loading.
- `primitives.{h,cpp}` — free functions returning `std::pair<std::vector<Vertex>, std::vector<uint32_t>>` for sphere/box/cylinder/cone/frustum/prism.
- `nurbs.{h,cpp}` — pure math: knot vector, basis functions (de Boor), `evaluate(t) -> vec3`.
- `collision.{h,cpp}` — pure math: `collideSphereAABB`, `resolveSphereAABB`, `collideSphereSphere`.
- `fps_camera` / `orbit_camera` — camera control logic writing into a base `Camera`.
- `target.{h,cpp}` — a ball target: static or NURBS-driven, alive/popping state, respawn.
- `weapon.{h,cpp}` — pistol/sniper state, fire-rate gating, viewmodel transform, muzzle-flash trigger, scope FOV.
- `particle_system.{h,cpp}` — GPU or CPU particle pool for pops + muzzle flash.
- `obj_sequence.{h,cpp}` — loads `frame_000.obj…frame_NNN.obj`, advances on a timer, swaps the active model mesh.
- `audio.{h,cpp}` — miniaudio wrapper: load + play one-shot SFX.
- `leaderboard.{h,cpp}` — read/write `scores.json` (minimal hand-rolled parser).
- `game_state.{h,cpp}` — score, timer, accuracy, mode, difficulty.
- `aim_trainer.{h,cpp}` — orchestrates everything: owns scene, cameras, lights, ImGui, render passes.

---

## Task 0: Standalone project skeleton + base copy + build a triangle

**Goal:** Create the directory structure, copy `base/` (minus tinyobjloader), copy `external/` libs, write a CMakeLists that builds both standalone and within the root, and render a triangle via a self-written `Application` subclass.

**Files:**
- Create: `projects/finalproject/CMakeLists.txt`
- Create: `projects/finalproject/source/base/*` (copy from `projects/base/*`)
- Create: `projects/finalproject/source/game/aim_trainer.{h,cpp}`
- Create: `projects/finalproject/source/main.cpp`
- Create: `projects/finalproject/external/*` (copy libs)
- Create: `projects/finalproject/media/shaders/triangle.vert`, `triangle.frag`

- [x] **Step 1: Create directory tree**

```bash
cd e:/GIT/GIS-AGENT/CG-projects/projects/finalproject
mkdir -p source/base source/game media/models media/textures media/sounds media/config media/shaders external
```

- [x] **Step 2: Copy external libraries (physical copy, no tinyobjloader)**

Copy these from repo root `external/` into `projects/finalproject/external/` (keep their CMakeLists so `add_subdirectory` works). **Exclude tinyobjloader.** **Add miniaudio** (single header — downloaded in Step 3).

```bash
cd e:/GIT/GIS-AGENT/CG-projects

# PREREQUISITE: glfw & glm are git submodules. Ensure they're initialized, or the
# copies below will be empty directories:
git submodule update --init external/glfw external/glm

cp -r external/glad  projects/finalproject/external/
cp -r external/glfw  projects/finalproject/external/
cp -r external/glm   projects/finalproject/external/
cp -r external/imgui projects/finalproject/external/
cp -r external/stb   projects/finalproject/external/
# DO NOT copy tinyobjloader — it must be self-written (Task 1)
```

- [x] **Step 3: Add miniaudio (single-header audio)**

Download `miniaudio.h` into `projects/finalproject/external/miniaudio/`:

```bash
cd e:/GIT/GIS-AGENT/CG-projects/projects/finalproject/external
mkdir -p miniaudio
curl -L -o miniaudio/miniaudio.h https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h
```

Acceptance: `projects/finalproject/external/miniaudio/miniaudio.h` exists and is >100KB. If offline, fetch via browser and place manually.

- [x] **Step 4: Copy base framework files (exclude tinyobjloader dependency)**

```bash
cd e:/GIT/GIS-AGENT/CG-projects/projects/finalproject/source/base
SRC=../../../base
cp $SRC/application.h $SRC/application.cpp .
cp $SRC/camera.h $SRC/camera.cpp .
cp $SRC/transform.h $SRC/transform.cpp .
cp $SRC/glsl_program.h $SRC/glsl_program.cpp .
cp $SRC/texture.h $SRC/texture.cpp .
cp $SRC/texture2d.h $SRC/texture2d.cpp .
cp $SRC/texture_cubemap.h $SRC/texture_cubemap.cpp .
cp $SRC/skybox.h $SRC/skybox.cpp .
cp $SRC/framebuffer.h $SRC/framebuffer.cpp .
cp $SRC/fullscreen_quad.h $SRC/fullscreen_quad.cpp .
cp $SRC/bounding_box.h $SRC/frustum.h $SRC/plane.h $SRC/input.h .
cp $SRC/vertex.h $SRC/light.h $SRC/gl_utility.h $SRC/frame_rate_indicator.h .
cp $SRC/model.h $SRC/model.cpp .
```

**Do NOT copy `model.h`/`model.cpp` yet without editing** — they `#include <tiny_obj_loader.h>`. Task 1 rewrites them. For Task 0 (triangle), we temporarily skip compiling `model.cpp` (see CMake note in Step 6). Actually simplest: **delete the copied `model.cpp` for now** and leave `model.h`; Task 1 restores it.

```bash
rm projects/finalproject/source/base/model.cpp projects/finalproject/source/base/model.h
```

The triangle task needs none of model — it uses raw GL calls.

- [x] **Step 5: Write the triangle shaders**

`projects/finalproject/media/shaders/triangle.vert`:
```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
void main() {
    gl_Position = vec4(aPos, 1.0);
}
```

`projects/finalproject/media/shaders/triangle.frag`:
```glsl
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(0.2, 0.8, 0.3, 1.0);
}
```

- [x] **Step 6: Write `aim_trainer.h`**

`projects/finalproject/source/game/aim_trainer.h`:
```cpp
#pragma once
#include "application.h"
#include "glsl_program.h"

class AimTrainer : public Application {
public:
    AimTrainer(const Options& options);
    ~AimTrainer() override;

protected:
    void handleInput() override;
    void renderFrame() override;

private:
    GLuint _vao = 0;
    GLuint _vbo = 0;
    std::unique_ptr<GLSLProgram> _shader;
    void initTriangle();
};
```

- [x] **Step 7: Write `aim_trainer.cpp`**

`projects/finalproject/source/game/aim_trainer.cpp`:
```cpp
#include "aim_trainer.h"
#include <glm/glm.hpp>

AimTrainer::AimTrainer(const Options& options) : Application(options) {
    _shader = std::make_unique<GLSLProgram>();
    _shader->attachVertexShaderFromFile(getAssetFullPath("shaders/triangle.vert"));
    _shader->attachFragmentShaderFromFile(getAssetFullPath("shaders/triangle.frag"));
    _shader->link();
    initTriangle();
}

AimTrainer::~AimTrainer() {
    if (_vbo) glDeleteBuffers(1, &_vbo);
    if (_vao) glDeleteVertexArrays(1, &_vao);
}

void AimTrainer::initTriangle() {
    float verts[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f,
    };
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void AimTrainer::handleInput() {
    _input.forwardState();
}

void AimTrainer::renderFrame() {
    // NOTE: the base Application ctor does NOT enable depth testing.
    // Every project enables it itself at the start of renderFrame (see
    // project2/3/6). Without it, 3D faces draw in submission order (wrong).
    glEnable(GL_DEPTH_TEST);
    glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _shader->use();
    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    _shader->unuse();
}
```

- [x] **Step 8: Write `main.cpp`**

`projects/finalproject/source/main.cpp`:
```cpp
#include "aim_trainer.h"
#include <iostream>

Options getOptions(int argc, char* argv[]) {
    Options options;
    options.windowTitle = "AimTrainer - Final Project";
    options.windowWidth = 1280;
    options.windowHeight = 720;
    options.windowResizable = false;
    options.vSync = true;
    options.msaa = true;
    options.glVersion = {3, 3};
    options.backgroundColor = glm::vec4(0.1f, 0.1f, 0.12f, 1.0f);
    // Asset root is baked at configure time (AIMTRAINER_MEDIA) so it works in
    // BOTH root build and standalone build (see CMakeLists Step 9).
    options.assetRootDir = AIMTRAINER_MEDIA;
    return options;
}

int main(int argc, char* argv[]) {
    try {
        AimTrainer app(getOptions(argc, argv));
        app.run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown Error" << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
```

> Asset path strategy: CMake bakes an **absolute** path to `projects/finalproject/media/` into the `AIMTRAINER_MEDIA` define (Step 9). This avoids the relative-path trap — `../../media/` only works by accident in one layout and points at the root media (not finalproject's own shaders/models) in root-build mode. The absolute path is correct regardless of build mode.

- [x] **Step 9: Write `CMakeLists.txt` (dual-mode: standalone + root-included)**

`projects/finalproject/CMakeLists.txt`:
```cmake
# Dual-mode CMake:
#  - Root build (default, how all other projects build): libs already exist as targets.
#  - Standalone (cmake -B build -S projects/finalproject): we add the libs ourselves.
# CRITICAL: detect mode by checking whether the glfw target already exists.
# Do NOT check a THIRD_PARTY_PATH variable — the root CMakeLists never defines
# one, so any "defined" check is always-true and we'd add libs twice -> error.
if(NOT TARGET glfw)
    # ---- STANDALONE MODE ----
    cmake_minimum_required(VERSION 3.15)
    project(FinalProject LANGUAGES C CXX)
    set(CMAKE_CXX_STANDARD 17)

    set(FINAL_ROOT ${CMAKE_SOURCE_DIR})              # projects/finalproject
    set(TP ${FINAL_ROOT}/external)

    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    add_subdirectory(${TP}/glfw  ${CMAKE_BINARY_DIR}/_lib/glfw)
    add_subdirectory(${TP}/glad  ${CMAKE_BINARY_DIR}/_lib/glad)
    add_subdirectory(${TP}/glm   ${CMAKE_BINARY_DIR}/_lib/glm)
    add_subdirectory(${TP}/imgui ${CMAKE_BINARY_DIR}/_lib/imgui)
    add_subdirectory(${TP}/stb   ${CMAKE_BINARY_DIR}/_lib/stb)
    set(FINAL_NAME FinalProject)
else()
    # ---- ROOT-INCLUDED MODE (libs already added by root CMakeLists) ----
    set(FINAL_ROOT ${CMAKE_CURRENT_SOURCE_DIR})      # projects/finalproject
    set(FINAL_NAME finalproject)
    # Do NOT add_subdirectory the libs here (root already did).
    # Do NOT link tinyobjloader (TA requires self-written OBJ loader).
endif()

# ---- Sources (base + game; model.{h,cpp} restored in Task 1) ----
file(GLOB BASE_HDR  ${FINAL_ROOT}/source/base/*.h)
file(GLOB BASE_SRC  ${FINAL_ROOT}/source/base/*.cpp)
file(GLOB GAME_HDR  ${FINAL_ROOT}/source/game/*.h)
file(GLOB GAME_SRC  ${FINAL_ROOT}/source/game/*.cpp)
set(MAIN_SRC ${FINAL_ROOT}/source/main.cpp)

add_executable(${FINAL_NAME} ${BASE_HDR} ${BASE_SRC} ${GAME_HDR} ${GAME_SRC} ${MAIN_SRC})

target_include_directories(${FINAL_NAME} PRIVATE
    ${FINAL_ROOT}/source
    ${FINAL_ROOT}/source/base
    ${FINAL_ROOT}/source/game)
    # imgui impl headers (imgui_impl_glfw.h, imgui_impl_opengl3.h) live flat in
    # external/imgui/ — NOT in a backends/ subdir — and are already compiled into
    # the imgui library. Just link imgui; no extra include path needed.

# media path baked as absolute define -> works in both build modes
target_compile_definitions(${FINAL_NAME} PRIVATE
    AIMTRAINER_MEDIA="${FINAL_ROOT}/media/")

target_link_libraries(${FINAL_NAME} PRIVATE glfw glad glm imgui stb)

# miniaudio: single header compiled in exactly one TU (audio.cpp, Task 8).
# On non-MSVC Windows toolchains, link the system libs miniaudio relies on:
if (WIN32 AND NOT MSVC)
    target_link_libraries(${FINAL_NAME} PRIVATE ole32 winmm ksuser)
endif()
```

> Corrections baked into this version (from the plan review):
> - Mode detection uses `if(NOT TARGET glfw)` — the root CMakeLists never defines `THIRD_PARTY_PATH`, so the old `if(NOT DEFINED THIRD_PARTY_PATH)` was always-true and would add libs twice → CMake error.
> - Removed `${TP}/imgui/backends` include — no such directory; the impl files are flat in `external/imgui/` and already compiled into the `imgui` target.
> - Asset path is an absolute `AIMTRAINER_MEDIA` define instead of fragile `../../media/` (which pointed at the root media in root-build mode, hiding finalproject's own shaders/models).
> - miniaudio global `MINIAUDIO_IMPLEMENTATION` define removed (would hit every TU → duplicate symbols). It is defined locally in `audio.cpp` (Task 8) instead.

- [x] **Step 10: (merged into Step 9)**

The miniaudio-define handling and Windows system-lib linking are already folded into the Step 9 CMake above. No separate step. (Task 8's `audio.cpp` will `#define MINIAUDIO_IMPLEMENTATION` locally — that's the only TU that may define it.)

- [x] **Step 11: Build via root CMake**

From repo root:
```bash
cd e:/GIT/GIS-AGENT/CG-projects
cmake -B build -S .
cmake --build build --config Release --target finalproject --parallel 8
```
Expected: builds `finalproject` target. If `finalproject` not found, the glob didn't pick it up — re-run `cmake -B build -S .` (CMake caches the glob).

- [x] **Step 12: Acceptance check — triangle renders**

```bash
cd build/bin/Release && ./finalproject.exe
```
Expected: a window titled "AimTrainer - Final Project" opens showing a green triangle on dark background. Close it. If blank/black, check the console for `getAssetFullPath` shader-load errors.

- [x] **Step 13: Commit**

```bash
cd e:/GIT/GIS-AGENT/CG-projects
git add projects/finalproject
git commit -m "finalproject: scaffold standalone project, render triangle"
```

---

## Task 1: Hand-written OBJ loader + export (replaces tinyobjloader)

**Goal:** Implement `obj_loader.{h,cpp}` that parses v/vt/vn/f (all three face formats) and exports mesh to `.obj`. Rewrite `model.{h,cpp}` to use it. Restore model into the build.

**Files:**
- Create: `projects/finalproject/source/base/obj_loader.h`
- Create: `projects/finalproject/source/base/obj_loader.cpp`
- Create: `projects/finalproject/source/base/model.h` (copy + edit)
- Create: `projects/finalproject/source/base/model.cpp` (rewrite)

- [x] **Step 1: Write `obj_loader.h`**

`projects/finalproject/source/base/obj_loader.h`:
```cpp
#pragma once
#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <vector>

// Raw OBJ geometry (no materials). Faces are triangulated into indices
// referencing the flat arrays by vertex-index (pos/uv/norm share one index space
// after the loader reconciles them into Vertex structs — see toVertices).
struct RawObj {
    std::vector<glm::vec3> positions;   // 'v'
    std::vector<glm::vec2> texcoords;   // 'vt'
    std::vector<glm::vec3> normals;     // 'vn'
    // each face corner: which pos/uv/norm (1-based as in .obj; -1 if absent)
    struct Corner { int p, t, n; };
    std::vector<std::vector<Corner>> faces; // polygon per face
};

// Parse a .obj file from disk. Throws std::runtime_error on unrecoverable error.
RawObj loadObj(const std::string& filepath);

// Parse a .obj from an in-memory string (used by self-tests).
RawObj parseObjString(const std::string& text);

// Export a mesh (positions/normals/texcoords + triangle indices) to .obj text.
// indices are triangles (length % 3 == 0).
std::string writeObj(const std::vector<glm::vec3>& positions,
                     const std::vector<glm::vec3>& normals,
                     const std::vector<glm::vec2>& texcoords,
                     const std::vector<uint32_t>& indices);

// Convenience: write to disk.
void saveObj(const std::string& filepath,
             const std::vector<glm::vec3>& positions,
             const std::vector<glm::vec3>& normals,
             const std::vector<glm::vec2>& texcoords,
             const std::vector<uint32_t>& indices);
```

- [x] **Step 2: Write `obj_loader.cpp`**

`projects/finalproject/source/base/obj_loader.cpp`:
```cpp
#include "obj_loader.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

static std::vector<std::string> splitWhitespace(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream iss(s);
    std::string tok;
    while (iss >> tok) out.push_back(tok);
    return out;
}

// parse "1", "1/2", "1//3", "1/2/3" into Corner (1-based, -1 absent)
static RawObj::Corner parseCorner(const std::string& tok) {
    RawObj::Corner c{-1, -1, -1};
    size_t a = tok.find('/');
    if (a == std::string::npos) { c.p = std::stoi(tok); return c; }
    if (a > 0) c.p = std::stoi(tok.substr(0, a));
    size_t b = tok.find('/', a + 1);
    if (b == std::string::npos) {
        if (b - a > 1) { /* has one slash only: p/t */ c.t = std::stoi(tok.substr(a + 1)); }
        return c;
    }
    // two slashes present: middle is texcoord, after is normal
    std::string mid = tok.substr(a + 1, b - a - 1);
    if (!mid.empty()) c.t = std::stoi(mid);
    std::string tail = tok.substr(b + 1);
    if (!tail.empty()) c.n = std::stoi(tail);
    return c;
}

RawObj parseObjString(const std::string& text) {
    RawObj obj;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        auto parts = splitWhitespace(line);
        if (parts.empty()) continue;
        const std::string& tag = parts[0];
        if (tag == "v" && parts.size() >= 4) {
            obj.positions.push_back({std::stof(parts[1]), std::stof(parts[2]), std::stof(parts[3])});
        } else if (tag == "vt" && parts.size() >= 3) {
            obj.texcoords.push_back({std::stof(parts[1]), std::stof(parts[2])});
        } else if (tag == "vn" && parts.size() >= 4) {
            obj.normals.push_back({std::stof(parts[1]), std::stof(parts[2]), std::stof(parts[3])});
        } else if (tag == "f" && parts.size() >= 4) {
            std::vector<RawObj::Corner> face;
            for (size_t i = 1; i < parts.size(); ++i) face.push_back(parseCorner(parts[i]));
            obj.faces.push_back(std::move(face));
        }
        // other tags (o, g, s, mtllib, usemtl, # comments) ignored
    }
    return obj;
}

RawObj loadObj(const std::string& filepath) {
    std::ifstream f(filepath);
    if (!f) throw std::runtime_error("Cannot open obj: " + filepath);
    std::stringstream ss;
    ss << f.rdbuf();
    return parseObjString(ss.str());
}

std::string writeObj(const std::vector<glm::vec3>& positions,
                     const std::vector<glm::vec3>& normals,
                     const std::vector<glm::vec2>& texcoords,
                     const std::vector<uint32_t>& indices) {
    std::ostringstream out;
    out << "# exported by AimTrainer\n";
    for (auto& p : positions) out << "v " << p.x << ' ' << p.y << ' ' << p.z << '\n';
    for (auto& t : texcoords) out << "vt " << t.x << ' ' << t.y << '\n';
    for (auto& n : normals)   out << "vn " << n.x << ' ' << n.y << ' ' << n.z << '\n';
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        // emit p/t/n (all 1-based) — assumes arrays are parallel & index-aligned
        out << "f "
            << indices[i] + 1 << '/' << indices[i] + 1 << '/' << indices[i] + 1 << ' '
            << indices[i + 1] + 1 << '/' << indices[i + 1] + 1 << '/' << indices[i + 1] + 1 << ' '
            << indices[i + 2] + 1 << '/' << indices[i + 2] + 1 << '/' << indices[i + 2] + 1 << '\n';
    }
    return out.str();
}

void saveObj(const std::string& filepath,
             const std::vector<glm::vec3>& positions,
             const std::vector<glm::vec3>& normals,
             const std::vector<glm::vec2>& texcoords,
             const std::vector<uint32_t>& indices) {
    std::ofstream f(filepath);
    if (!f) throw std::runtime_error("Cannot write obj: " + filepath);
    f << writeObj(positions, normals, texcoords, indices);
}
```

> Note: `parseCorner` handles the `"1//3"` case (two slashes, no texcoord) correctly because the first slash yields empty middle only when slashes are adjacent — verify with the self-test in Step 5.

- [x] **Step 3: Restore & rewrite `model.h`**

Copy from `projects/base/model.h` verbatim (it has no tinyobjloader include). 
```bash
cp e:/GIT/GIS-AGENT/CG-projects/projects/base/model.h e:/GIT/GIS-AGENT/CG-projects/projects/finalproject/source/base/model.h
```
No edits needed to the header.

- [x] **Step 4: Rewrite `model.cpp` (file ctor uses obj_loader, vector ctor unchanged)**

`projects/finalproject/source/base/model.cpp` — copy `projects/base/model.cpp` then **replace** the file-loading constructor (lines 1-90 in original) with the version below. Keep the move ctor, destructor, draw, getVao, getVertexCount, initGLResources, computeBoundingBox, initBoxGLResources, cleanup **unchanged** from the original.

New top of file + file constructor:
```cpp
#include <algorithm>
#include <iostream>
#include <limits>
#include <unordered_map>

#include "model.h"
#include "obj_loader.h"   // <-- self-written, replaces tiny_obj_loader.h

Model::Model(const std::string& filepath) {
    RawObj obj = loadObj(filepath);

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<Vertex, uint32_t> uniqueVertices;

    auto pushCorner = [&](const RawObj::Corner& c) {
        Vertex vertex{};
        if (c.p >= 1 && (size_t)c.p <= obj.positions.size())
            vertex.position = obj.positions[c.p - 1];
        if (c.n >= 1 && (size_t)c.n <= obj.normals.size())
            vertex.normal = obj.normals[c.n - 1];
        if (c.t >= 1 && (size_t)c.t <= obj.texcoords.size())
            vertex.texCoord = obj.texcoords[c.t - 1];

        if (uniqueVertices.count(vertex) == 0) {
            uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
            vertices.push_back(vertex);
        }
        indices.push_back(uniqueVertices[vertex]);
    };

    for (auto& face : obj.faces) {
        // fan-triangulate polygons (n-gon -> triangles)
        for (size_t i = 2; i < face.size(); ++i) {
            pushCorner(face[0]);
            pushCorner(face[i - 1]);
            pushCorner(face[i]);
        }
    }

    _vertices = vertices;
    _indices = indices;

    computeBoundingBox();
    initGLResources();
    initBoxGLResources();

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        cleanup();
        throw std::runtime_error("OpenGL Error: " + std::to_string(error));
    }
}

// ... (vector ctor, move ctor, ~Model, draw, accessors, initGLResources,
//      computeBoundingBox, initBoxGLResources, cleanup — copy verbatim
//      from projects/base/model.cpp lines 92-291) ...
```

- [x] **Step 5: Self-test the parser (logic test, no window)**

Add a temporary test target. Create `projects/finalproject/source/base/obj_loader_selftest.cpp`:
```cpp
#include "obj_loader.h"
#include <cassert>
#include <cstdio>
int main() {
    std::string src =
        "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
        "vt 0 0\nvt 1 0\nvt 0 1\n"
        "vn 0 0 1\n"
        "f 1/1/1 2/2/1 3/3/1\n";   // simple triangle
    RawObj obj = parseObjString(src);
    assert(obj.positions.size() == 3);
    assert(obj.texcoords.size() == 3);
    assert(obj.normals.size() == 1);
    assert(obj.faces.size() == 1);
    assert(obj.faces[0][0].p == 1 && obj.faces[0][0].t == 1 && obj.faces[0][0].n == 1);

    // the 1//3 format
    RawObj o2 = parseObjString("v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 0 0 1\nf 1//1 2//1 3//1\n");
    assert(o2.faces[0][0].p == 1 && o2.faces[0][0].t == -1 && o2.faces[0][0].n == 1);

    std::printf("obj_loader selftest PASS\n");
    return 0;
}
```
Add a temporary CMake entry at the bottom of `CMakeLists.txt`:
```cmake
add_executable(obj_selftest ${FINAL_ROOT}/source/base/obj_loader_selftest.cpp)
target_include_directories(obj_selftest PRIVATE ${FINAL_ROOT}/source/base)
target_link_libraries(obj_selftest PRIVATE glm)
```
Build & run:
```bash
cmake --build build --config Release --target obj_selftest --parallel 8
build/bin/Release/obj_selftest.exe   # or build/obj_selftest.exe in standalone
```
Expected: prints `obj_loader selftest PASS`. If it asserts, fix `parseCorner`.

- [x] **Step 6: Acceptance check — load a real model**

Add to `aim_trainer.cpp` a `std::unique_ptr<Model> _model` initialized in the ctor as `_model = std::make_unique<Model>(getAssetFullPath("models/cube.obj"));`. Drop a `cube.obj` (export one from Task 2's primitive generator, or grab any small obj) into `media/models/`. In `renderFrame`, set up a basic MVP and call `_model->draw()`. Build & run.

Expected: a model renders (not a triangle anymore). If it throws "Cannot open obj", the path is wrong.

- [x] **Step 7: Remove self-test scaffolding**

Delete `obj_loader_selftest.cpp` and its CMake `add_executable(obj_selftest ...)` block. (Keep the selftest code pattern documented; we won't ship test targets.)

- [x] **Step 8: Commit**

```bash
git add projects/finalproject
git commit -m "finalproject: hand-written OBJ loader+export, replace tinyobjloader"
```

---

## Task 2: Parametric primitives + Blinn-Phong renderer

**Goal:** Generate 6 primitive meshes (sphere/box/cylinder/cone/frustum/prism) in code. Render models with a Blinn-Phong shader + one directional light + MVP. Proves base requirements 1 & 3.

**Files:**
- Create: `projects/finalproject/source/primitives.h`
- Create: `projects/finalproject/source/primitives.cpp`
- Create: `projects/finalproject/media/shaders/blinn_phong.vert`, `blinn_phong.frag`
- Modify: `projects/finalproject/source/game/aim_trainer.{h,cpp}`

- [x] **Step 1: Write `primitives.h`**

`projects/finalproject/source/primitives.h`:
```cpp
#pragma once
#include "vertex.h"
#include <utility>
#include <vector>

using Mesh = std::pair<std::vector<Vertex>, std::vector<uint32_t>>;

Mesh createSphere(float radius = 0.5f, int segments = 24, int rings = 16);
Mesh createBox(float w = 1.0f, float h = 1.0f, float d = 1.0f);
Mesh createCylinder(float radius = 0.5f, float height = 1.0f, int segments = 24);
Mesh createCone(float radius = 0.5f, float height = 1.0f, int segments = 24);
Mesh createPrismFrustum(float rTop = 0.4f, float rBottom = 0.6f, float height = 1.0f, int nSides = 6);
Mesh createPrism(float radius = 0.5f, float height = 1.0f, int nSides = 6);
```

- [x] **Step 2: Write `primitives.cpp`**

`projects/finalproject/source/primitives.cpp`:
```cpp
#include "primitives.h"
#define _USE_MATH_DEFINES
#include <cmath>

static void pushTri(std::vector<Vertex>& v, std::vector<uint32_t>& i,
                    const Vertex& a, const Vertex& b, const Vertex& c) {
    uint32_t base = (uint32_t)v.size();
    v.push_back(a); v.push_back(b); v.push_back(c);
    i.push_back(base); i.push_back(base + 1); i.push_back(base + 2);
}

Mesh createSphere(float radius, int segments, int rings) {
    std::vector<Vertex> v; std::vector<uint32_t> i;
    for (int r = 0; r <= rings; ++r) {
        float theta = (float)M_PI * r / rings;
        float st = std::sin(theta), ct = std::cos(theta);
        for (int s = 0; s <= segments; ++s) {
            float phi = 2.0f * (float)M_PI * s / segments;
            float sp = std::sin(phi), cp = std::cos(phi);
            glm::vec3 n(cp * st, ct, sp * st);
            v.push_back(Vertex{n * radius, n, {s / (float)segments, r / (float)rings}});
        }
    }
    for (int r = 0; r < rings; ++r)
        for (int s = 0; s < segments; ++s) {
            uint32_t a = r * (segments + 1) + s;
            uint32_t b = a + segments + 1;
            i.push_back(a); i.push_back(b); i.push_back(a + 1);
            i.push_back(a + 1); i.push_back(b); i.push_back(b + 1);
        }
    return {v, i};
}

// helper for prism/frustum side ring at height y with radius r
static void addRing(std::vector<glm::vec3>& ring, float r, float y, int nSides) {
    for (int k = 0; k < nSides; ++k) {
        float a = 2.0f * (float)M_PI * k / nSides;
        ring.push_back({r * std::cos(a), y, r * std::sin(a)});
    }
}

Mesh createPrismFrustum(float rTop, float rBottom, float height, int nSides) {
    std::vector<Vertex> v; std::vector<uint32_t> i;
    std::vector<glm::vec3> bot, top;
    addRing(bot, rBottom, -height / 2, nSides);
    addRing(top, rTop,    height / 2, nSides);
    // side faces (quads -> 2 tris), normals = outward horizontal
    for (int k = 0; k < nSides; ++k) {
        int kn = (k + 1) % nSides;
        glm::vec3 e0 = bot[kn] - bot[k];
        glm::vec3 edge = top[k] - bot[k];
        glm::vec3 n = glm::normalize(glm::cross(edge, e0)); // outward
        uint32_t base = (uint32_t)v.size();
        v.push_back({bot[k], n, {0,0}}); v.push_back({bot[kn], n, {1,0}});
        v.push_back({top[kn], n, {1,1}}); v.push_back({top[k], n, {0,1}});
        i.push_back(base); i.push_back(base+1); i.push_back(base+2);
        i.push_back(base); i.push_back(base+2); i.push_back(base+3);
    }
    // bottom cap fan (normal down)
    uint32_t cBot = (uint32_t)v.size();
    v.push_back({{0,-height/2,0}, {0,-1,0}, {0,0}});
    for (int k = 0; k < nSides; ++k) {
        int kn = (k + 1) % nSides;
        v.push_back({bot[k], {0,-1,0}, {0,0}}); v.push_back({bot[kn], {0,-1,0}, {0,0}});
    }
    for (int k = 0; k < nSides; ++k) {
        uint32_t b = cBot + 1 + k * 2;
        i.push_back(cBot); i.push_back(b); i.push_back(b + 1);
    }
    // top cap fan (normal up)
    uint32_t cTop = (uint32_t)v.size();
    v.push_back({{0, height/2,0}, {0,1,0}, {0,0}});
    for (int k = 0; k < nSides; ++k) {
        int kn = (k + 1) % nSides;
        v.push_back({top[k], {0,1,0}, {0,0}}); v.push_back({top[kn], {0,1,0}, {0,0}});
    }
    for (int k = 0; k < nSides; ++k) {
        uint32_t b = cTop + 1 + k * 2;
        i.push_back(cTop); i.push_back(b + 1); i.push_back(b);
    }
    return {v, i};
}

Mesh createPrism(float radius, float height, int nSides) {
    return createPrismFrustum(radius, radius, height, nSides);
}

Mesh createBox(float w, float h, float d) {
    // 24 verts (4 per face), explicit normals + UVs
    std::vector<Vertex> v; std::vector<uint32_t> i;
    float x = w/2, y = h/2, z = d/2;
    struct Face { glm::vec3 n; glm::vec3 p[4]; glm::vec2 uv[4]; };
    Face faces[6] = {
        {{ 0, 0, 1}, {{-x,-y, z},{ x,-y, z},{ x, y, z},{-x, y, z}}, {{0,0},{1,0},{1,1},{0,1}}},
        {{ 0, 0,-1}, {{ x,-y,-z},{-x,-y,-z},{-x, y,-z},{ x, y,-z}}, {{0,0},{1,0},{1,1},{0,1}}},
        {{ 0, 1, 0}, {{-x, y, z},{ x, y, z},{ x, y,-z},{-x, y,-z}}, {{0,0},{1,0},{1,1},{0,1}}},
        {{ 0,-1, 0}, {{-x,-y,-z},{ x,-y,-z},{ x,-y, z},{-x,-y, z}}, {{0,0},{1,0},{1,1},{0,1}}},
        {{ 1, 0, 0}, {{ x,-y, z},{ x,-y,-z},{ x, y,-z},{ x, y, z}}, {{0,0},{1,0},{1,1},{0,1}}},
        {{-1, 0, 0}, {{-x,-y,-z},{-x,-y, z},{-x, y, z},{-x, y,-z}}, {{0,0},{1,0},{1,1},{0,1}}},
    };
    for (auto& f : faces) {
        uint32_t base = (uint32_t)v.size();
        for (int k = 0; k < 4; ++k) v.push_back({f.p[k], f.n, f.uv[k]});
        i.push_back(base); i.push_back(base+1); i.push_back(base+2);
        i.push_back(base); i.push_back(base+2); i.push_back(base+3);
    }
    return {v, i};
}

Mesh createCylinder(float radius, float height, int segments) {
    return createPrismFrustum(radius, radius, height, segments);
}

Mesh createCone(float radius, float height, int segments) {
    return createPrismFrustum(0.0001f, radius, height, segments); // top radius ~0
}
```

> Note: `createBox` produces 6 distinct faces with correct flat normals — better for lighting than a shared-vertex cube.

- [x] **Step 3: Write Blinn-Phong shaders**

`projects/finalproject/media/shaders/blinn_phong.vert`:
```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vTexCoord;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vTexCoord = aTexCoord;
    gl_Position = uProj * uView * world;
}
```

`projects/finalproject/media/shaders/blinn_phong.frag`:
```glsl
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
out vec4 FragColor;

uniform vec3 uViewPos;
uniform sampler2D uTexture;
uniform bool  uHasTexture;
uniform vec3  uKd; uniform vec3 uKs; uniform float uShininess;

// directional light
uniform vec3 uLightDir;     // direction TO light (already normalized)
uniform vec3 uLightColor;
uniform float uLightIntensity;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 L = normalize(uLightDir);
    vec3 H = normalize(L + V);

    vec3 albedo = uHasTexture ? texture(uTexture, vTexCoord).rgb : uKd;
    vec3 ambient = 0.15 * albedo;
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * uLightColor * uLightIntensity * albedo;
    float spec = pow(max(dot(N, H), 0.0), uShininess);
    vec3 specular = spec * uLightColor * uLightIntensity * uKs;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
```

- [x] **Step 4: Wire primitives + Blinn-Phong into aim_trainer**

In `aim_trainer.h` add members:
```cpp
#include "model.h"
#include <memory>
struct BlinnPhongMaterial { glm::vec3 kd{0.8f}, ks{0.6f}; float shininess{32.0f}; };
// in class:
std::unique_ptr<Model> _sphere, _box, _cyl, _cone, _frustum, _prism;
std::unique_ptr<GLSLProgram> _bpShader;
glm::vec3 _lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
glm::vec3 _lightColor{1.0f};
float _lightIntensity = 1.0f;
BlinnPhongMaterial _mat;
glm::vec3 _camPos{0, 1.5f, 5};
```
In the ctor: build each primitive via `Model(vertices, indices)`; load `_bpShader` from files. In `renderFrame`: set view (`lookAt(_camPos, _camPos+front, up)`), proj (`perspective`), loop the 6 models with distinct transforms, set uniforms, draw.

- [x] **Step 5: Build & acceptance**

```bash
cmake --build build --config Release --target finalproject --parallel 8
cd build/bin/Release && ./finalproject.exe
```
Expected: 6 lit primitives (sphere, box, cylinder, cone, frustum, prism) on a dark background, shaded by Blinn-Phong (bright on the light-facing side, dark away). If flat/unlit, check normals in `primitives.cpp` and the normal-matrix line.

- [x] **Step 6: Commit**

```bash
git add projects/finalproject
git commit -m "finalproject: parametric primitives + Blinn-Phong renderer"
```

---

## Task 3: Multi-light + ImGui material/light editing

**Goal:** Add point/spot/hemisphere lights alongside directional. Add an ImGui panel (project5 pattern) to edit material kd/ks/shininess, texture, and each light's position/intensity/color. Proves base req 3 & 5.

**Files:**
- Modify: `projects/finalproject/media/shaders/blinn_phong.frag`
- Modify: `projects/finalproject/source/game/aim_trainer.{h,cpp}`

- [x] **Step 1: Add ImGui to aim_trainer (init/shutdown/newframe)**

In `aim_trainer.h` add `bool _showUi = true;` and these includes (impl headers are flat in `external/imgui/`, pulled in via the `imgui` target's include):
```cpp
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
```
In the ctor body (GL is already current after the base `Application` ctor — see application.cpp:38 `glfwMakeContextCurrent`), following project5's pattern:
```cpp
IMGUI_CHECKVERSION();
ImGui::CreateContext();
ImGui::StyleColorsDark();
ImGui_ImplGlfw_InitForOpenGL(_window, true);
ImGui_ImplOpenGL3_Init();
```
In dtor (before the base dtor destroys the window — so call it in `~AimTrainer`, which runs before `~Application`): `ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();`. At end of `renderFrame()` (after scene draws, before the base `run()` swaps):
```cpp
ImGui_ImplOpenGL3_NewFrame();
ImGui_ImplGlfw_NewFrame();
ImGui::NewFrame();
// ...build windows...
ImGui::Render();
ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
```

> ImGui config flags: enable mouse capture so clicks on UI panels don't also fire the gun. After `CreateContext`, set `ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;` is NOT needed; instead, in the camera/fire update, gate on `if (ImGui::GetIO().WantCaptureMouse) return;` (pbr_viewer pattern). The `Application::run()` order is handleInput → renderFrame → swap, and ImGui renders inside renderFrame before swap — correct timing (confirmed in application.cpp:86-94).

- [x] **Step 2: Extend frag shader with point + spot + hemisphere lights**

Add to `blinn_phong.frag` (uniforms + computation). Point light (attenuation), spot light (cone), hemisphere (sky/ground blend). Provide a `uniform int uPointCount; uniform PointLight uPoints[8];` etc. Keep directional as-is. Full shader grows; structure:
```glsl
struct PointLight { vec3 position; vec3 color; float intensity; float kc,kl,kq; };
struct SpotLight  { vec3 position; vec3 direction; vec3 color; float intensity; float angle; float kc,kl,kq; };
uniform int uPointCount; uniform PointLight uPoints[4];
uniform int uSpotCount;  uniform SpotLight  uSpots[2];
uniform vec3 uSkyColor, uGroundColor;  // hemisphere
```
Sum contributions (ambient = hemisphere term + 0.1 base).

- [x] **Step 3: Add light state + editing UI in aim_trainer**

Members: `PointLight _points[4]` (use the base `PointLight` struct), `SpotLight _spots[2]`, hemisphere colors, counts. In the ImGui block build a "Lighting" window with `SliderFloat3`/`ColorEdit3`/`SliderFloat` for each. A "Material" window edits `_mat`. Set all uniforms each frame.

- [x] **Step 4: Build & acceptance**

Expected: ImGui panel visible. Dragging a point light's position visibly moves the highlight on the primitives. Changing kd changes albedo. Toggling spot angle narrows the cone. `ImGui::GetIO().WantCaptureMouse` guards camera input (do this in Task 4).

- [x] **Step 5: Commit**

```bash
git commit -am "finalproject: multi-light + ImGui material/light editing"
```

---

## Task 4: FPS + Orbit cameras, Tab switch, zoom/pan/fit

**Goal:** First-person camera (mouse-look, WASD, horizontal-only movement). Orbit camera (right-drag rotate, wheel zoom, mid-drag pan, F = zoom-to-fit). Tab toggles. Proves base req 4 & 6.

**Files:**
- Create: `projects/finalproject/source/game/fps_camera.h`, `fps_camera.cpp`
- Create: `projects/finalproject/source/game/orbit_camera.h`, `orbit_camera.cpp`
- Modify: `aim_trainer.{h,cpp}`

- [x] **Step 1: Write `fps_camera.h/.cpp`**

Wraps a `PerspectiveCamera`. Holds yaw/pitch. `update(input, dt)`:
- mouse delta → yaw -= dx*sens; pitch -= dy*sens (clamp ±89°)
- rebuild rotation from yaw/pitch
- WASD → move along front/right **on the horizontal plane** (front.y = 0, renormalized) — this enforces "no vertical movement"
- store position into `_camera->transform.position`

**First-frame jump guard:** `_input.mouse.move.xOld` defaults to 0, so the first frame's `dx = xNow - xOld` is huge and snaps the camera. Keep a `_firstFrame` bool; on the first update, set `_firstFrame=false` and skip the mouse-look delta that frame (only seed xOld). Equivalently, recenter the cursor with `glfwSetCursorPos` on entering FPS mode and sync `xNow`/`xOld`.

Cursor lock handled by aim_trainer (sets `glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED)` in FPS mode).

- [x] **Step 2: Write `orbit_camera.h/.cpp`**

From pbr_viewer `CameraController`. State NONE/ROTATE/ZOOM/PAN. `update(input, dt)`:
- right-drag → spherical orbit (phi/theta)
- wheel → zoom (eye length)
- mid-drag → pan (eye + target along right/up)
- `zoomToFit(sceneBounds)` → frame the scene bounding sphere

- [x] **Step 3: Wire Tab switch in aim_trainer**

**Key-edge detection (important):** the framework stores `_input.keyboard.keyStates[key] = action` — the *last* action only, with no "just pressed" concept. So `if (keyStates[TAB] == GLFW_PRESS)` is true *every frame* the key is held → Tab would flip modes every frame (flicker). project3 handles this by **resetting the key state to `GLFW_RELEASE` after handling the edge** (scene_roaming.cpp:59). Do the same:

```cpp
if (_input.keyboard.keyStates[GLFW_KEY_TAB] == GLFW_PRESS) {
    _camMode = (_camMode == CamMode::FPS) ? CamMode::Orbit : CamMode::FPS;
    _input.keyboard.keyStates[GLFW_KEY_TAB] = GLFW_RELEASE;   // <-- consume the edge
    // apply cursor mode per _camMode ...
}
```

`enum class CamMode { FPS, Orbit }; CamMode _camMode = CamMode::FPS;` On Tab edge: toggle; if FPS → `glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED)`; if Orbit → `GLFW_CURSOR_NORMAL`. Only let the active camera consume input. Guard: `if (ImGui::GetIO().WantCaptureMouse) return;` at top of camera update. (The same consume-the-edge pattern applies to F2 screenshot in Task 9 and any other single-fire key.)

- [x] **Step 4: Build & acceptance**

Expected: FPS mode — mouse looks around, WASD walks on the floor (no flying). Press Tab — cursor appears; right-drag orbits, wheel zooms, mid-drag pans, F frames the scene. Press Tab — back to FPS.

- [x] **Step 5: Commit**

```bash
git add projects/finalproject/source/game/fps_camera.* orbit_camera.*
git commit -am "finalproject: FPS + Orbit cameras with Tab switch"
```

---

## Task 5: Shadow mapping

**Goal:** Render scene depth from the directional light into an FBO, sample it in the Blinn-Phong pass for real-time shadows. Proves bonus: complex lighting.

**Files:**
- Create: `projects/finalproject/media/shaders/shadow_depth.vert`, `shadow_depth.frag`
- Modify: `blinn_phong.{vert,frag}` (add shadow coord + lookup)
- Modify: `aim_trainer.{h,cpp}` (depth FBO + render pass)

- [x] **Step 1: Create depth FBO (Frame 1 of render)**

Use base `Framebuffer` + `Texture2D(GL_DEPTH_COMPONENT, 2048, 2048, ...)`. Set NEAREST + CLAMP_TO_BORDER white border (bonus4 pattern). Compute `lightSpaceMatrix = ortho(...) * lookAt(lightPos, center, up)`.

- [x] **Step 2: Depth shaders**

`shadow_depth.vert`: `gl_Position = uLightSpace * uModel * vec4(aPos,1);`. `shadow_depth.frag`: empty (depth-only). 

- [x] **Step 3: Sample shadow in Blinn-Phong**

In vert: compute `vLightSpace = uLightSpace * uModel * vec4(aPos,1)`. In frag: standard shadow lookup with bias, `PCF 3x3` filter for soft edges. Multiply diffuse/specular by shadow factor.

- [x] **Step 4: Render order**

`renderFrame`: (1) shadow depth pass → (2) bind shadow map to a texture slot, blinn-phong pass.

- [x] **Step 5: Build & acceptance**

Expected: primitives cast shadows onto the ground plane; shadow edges soft (PCF). Move the light in the ImGui panel → shadows rotate.

- [x] **Step 6: Commit**

```bash
git commit -am "finalproject: shadow mapping (directional)"
```

---

## Task 6: Hand-written NURBS curve + Wild-mode trajectory

**Goal:** Implement NURBS curve evaluation (knot vector + de Boor). Drive multiple targets along it in Wild mode. Visualize control points. Proves bonus: NURBS.

**Files:**
- Create: `projects/finalproject/source/nurbs.h`, `nurbs.cpp`
- Create: `projects/finalproject/media/shaders/nurbs_debug.vert`, `nurbs_debug.frag`
- Modify: `aim_trainer.{h,cpp}`

- [x] **Step 1: Write `nurbs.h`**

```cpp
#pragma once
#include <glm/glm.hpp>
#include <vector>
class NurbsCurve {
public:
    int degree = 3;
    std::vector<glm::vec3> controlPoints;
    std::vector<float> knots;
    void setClampedUniform(int deg, std::vector<glm::vec3> cps); // auto knots
    int findSpan(float t) const;
    float basis(int i, int p, float t) const;
    glm::vec3 evaluate(float t) const;   // t in [0,1] mapped to knot domain
};
```

- [x] **Step 2: Write `nurbs.cpp`** (de Boor / Cox–de Boor recursion + clamped uniform knot generation)

Implement `findSpan` (binary search), `basis` (recursive Cox–de Boor with zero-division guards), `evaluate` (sum basis*cp), `setClampedUniform` (build clamped knot vector). Self-test: a degree-3 curve with 4 control points equals the Bézier curve; at t=0 returns cp[0], at t=1 returns cp[last].

- [x] **Step 3: Self-test NURBS** (logic test, like Task 1 Step 5): assert `evaluate(0)==cps.front()`, `evaluate(1)==cps.back()`, midpoint is inside the control hull.

- [x] **Step 4: Debug shader + visualization**

`nurbs_debug` shaders draw control points as small spheres (instanced or many draws) and the curve as a `GL_LINE_STRIP` of sampled points. ImGui lets you drag control points (in Orbit mode, ray-pick — simplified: drag in a screen-space gizmo or edit numeric sliders if full picking is too much; **use numeric sliders** for v1, note ray-pick as a stretch).

- [x] **Step 5: Wild mode targets**

`_wildTargets` (vector of `Target` — see Task 8, but stub a minimal ball here): each has a `NurbsCurve` and a phase `t`; `update(dt)` advances `t` by `speed*dt`, sets position to `evaluate(t)`. ImGui slider for speed.

- [x] **Step 6: Build & acceptance**

Expected: in Wild mode, several balls travel smoothly along curved NURBS paths; control points visible as dots; speed slider changes pace.

- [x] **Step 7: Commit**

```bash
git add projects/finalproject/source/nurbs.* media/shaders/nurbs_debug.*
git commit -am "finalproject: hand-written NURBS curve + Wild-mode trajectories"
```

---

## Task 7: Collision (sphere-AABB player vs obstacles + sphere-sphere target repulsion)

**Goal:** Player moves on the floor, blocked by railing/boxes/walls (sphere-AABB). Wild-mode balls repel each other (sphere-sphere). Proves bonus: collision.

**Files:**
- Create: `projects/finalproject/source/collision.h`, `collision.cpp`
- Modify: `fps_camera.cpp` (apply collision to movement), `aim_trainer.cpp` (obstacle AABBs, target repulsion)

- [x] **Step 1: Write `collision.h`**

```cpp
#pragma once
#include <glm/glm.hpp>
struct AABB { glm::vec3 min, max; };
// returns true if sphere (c,r) intersects box; writes push-out vector (c - closest)
bool intersectSphereAABB(const glm::vec3& c, float r, const AABB& b, glm::vec3& outClosest);
// resolves player position against a list of AABBs (axis-by-axis sliding)
glm::vec3 resolvePlayer(const glm::vec3& pos, float r, const std::vector<AABB>& boxes, const glm::vec3& delta);
// sphere-sphere: returns true & normal if overlapping
bool intersectSphereSphere(const glm::vec3& a, float ra, const glm::vec3& b, float rb, glm::vec3& outNormal);
```

- [x] **Step 2: Write `collision.cpp`**

`intersectSphereAABB`: clamp center to box → closest point; `d = c - closest`; intersect if `dot(d,d) < r*r`. `resolvePlayer`: apply X then Z separately (so you slide along walls): for each axis, tentatively move, if that puts sphere inside any box, revert that axis. `intersectSphereSphere`: `n = a-b; return length(n) < ra+rb; outNormal = normalize(n)`.

- [x] **Step 3: Build the level (railing + left/right boxes + walls)**

In aim_trainer, create `std::vector<AABB> _obstacles`. Place: front railing (one long thin AABB across), a left box, a right box (with wide gap between), 4 walls. Render each as a `Model(createBox(...))` with a texture. **Dimensions are tunable** so "not too small" can be adjusted.

- [x] **Step 4: Apply collision in fps_camera**

`update`: compute `delta` from WASD; `newPos = resolvePlayer(pos, 0.4f, obstacles, delta)`; clamp Y to fixed floor height (enforce horizontal-only).

- [x] **Step 5: Sphere-sphere ball repulsion (Wild mode)**

Each frame, for each pair of wild balls, if overlapping, push both apart along the normal by half the penetration. Keeps them from clumping.

- [x] **Step 6: Self-test collision** (logic): two AABBs, a sphere clearly inside → `intersectSphereAABB` true; sphere clearly outside → false; resolvePlayer against a wall blocks X but allows Z sliding.

- [x] **Step 7: Build & acceptance**

Expected: in FPS mode you can walk around but cannot pass the railing into the target area, cannot walk through the left/right boxes or walls; you slide along them. In Wild mode, balls spread out and don't overlap.

- [x] **Step 8: Commit**

```bash
git add projects/finalproject/source/collision.*
git commit -am "finalproject: sphere-AABB + sphere-sphere collision"
```

---

## Task 8: Gameplay — weapons, targets, particles, audio, scoring, leaderboard

**Goal:** Pistol/sniper with muzzle flash + sounds; balloon targets (Standard single-static, Wild multi-NURBS); pop particle effect; timer/score/accuracy; leaderboard JSON; three tunable params. Proves bonus: complete 3D game.

**Files:**
- Create: `projects/finalproject/source/game/target.{h,cpp}`
- Create: `projects/finalproject/source/game/weapon.{h,cpp}`
- Create: `projects/finalproject/source/game/particle_system.{h,cpp}`
- Create: `projects/finalproject/source/game/audio.{h,cpp}`
- Create: `projects/finalproject/source/game/leaderboard.{h,cpp}`
- Create: `projects/finalproject/source/game/game_state.{h,cpp}`
- Modify: `aim_trainer.{h,cpp}`
- Assets: `media/textures/weapons/*`, `media/textures/balloon/*`, `media/sounds/*`, `media/config/scores.json`

- [ ] **Step 1: `audio.{h,cpp}` (miniaudio wrapper)**

In `audio.cpp` ONLY: `#define MINIAUDIO_IMPLEMENTATION` then `#include "miniaudio.h"`. Wrapper: `class Audio { init(); play(const std::string& wavPath); }`. Load sounds into `ma_sound` pool; `play` starts from 0. Provide pistol/sniper/scope/pop wav (generate tiny wavs or source CC0 — placeholder beeps acceptable if no assets; document).

- [ ] **Step 2: `weapon.{h,cpp}`**

`enum WeaponType { Pistol, Sniper };` State: `cooldownTimer`, `scoped` bool. `tryFire(now)` → Pistol always allowed (no rate cap); Sniper allowed only if `cooldownTimer<=0` (sets ~1s). `update(dt)` decrements cooldown. On fire: emit muzzle-flash particle, play sound, return a ray (origin+dir from camera). Viewmodel: build pistol/sniper from boxes+cylinders (Task 2 primitives) with weapon textures; draw as a separate pass with a fixed viewmodel matrix (screen-space, no camera movement) + slight bob from walk.

- [ ] **Step 3: `target.{h,cpp}`**

A balloon = sphere + small knot sphere on top + texture. State: `alive`, `position`, optional `NurbsCurve*`. `update(dt)` (Wild: move along curve; Standard: static). `pop()` → dead, spawn particles, schedule respawn. In Standard, respawn at a random point **within the camera frustum** (project candidate points, accept if in frustum & not behind obstacles). `scale` member for the size slider.

- [ ] **Step 4: `particle_system.{h,cpp}`**

CPU particle pool (a few hundred). Particle: pos, vel, life, color, size. Update integrates with gravity, fades alpha. Render as point sprites or small quads (additive blend for muzzle flash, normal blend for balloon shards). `burst(position, color, count)`.

- [ ] **Step 5: `game_state.{h,cpp}` + `leaderboard.{h,cpp}`**

`GameState { GameMode mode; float timeLimit, timeLeft; int hits, shots; float startTime; }`. Accuracy = hits/shots. Leaderboard: minimal hand-rolled JSON read/write of `{ "scores": [ {name, score, mode}, ... ] }` to `media/config/scores.json` (top 10). Keep the parser tiny (split on tokens) — no external JSON lib.

- [ ] **Step 6: Ray-sphere hit test + firing**

On weapon fire, cast ray from screen center (scope center if scoped). Intersect each alive target's sphere (analytic ray-sphere). Closest hit → `pop()`, increment hits, play pop sound, burst particles.

- [ ] **Step 7: HUD (ImGui overlay)**

Crosshair (center dot/ring drawn via ImGui `AddCircle` on the background draw list, or a fullscreen quad). Scope overlay (black vignette ring when scoped). Score/timer/accuracy text. Mode selector, and three sliders: time limit, Wild speed, ball size.

- [ ] **Step 8: Assets**

Add placeholder textures (solid colors OK to start) under `media/textures/weapons/{pistol,sniper}.png`, `media/textures/balloon/{red,blue,green}.png`. Add short wav beeps under `media/sounds/`. Document where real assets should go.

- [ ] **Step 9: Build & acceptance**

Expected: Standard mode — shoot balloons, they pop with particles + sound, a new one appears in view; timer counts down; at end, score saved to leaderboard. Pistol fires fast; Sniper has cooldown + right-click scope. Wild mode — multiple balls on NURBS paths, repelling each other. Sliders work live.

- [ ] **Step 10: Commit**

```bash
git add projects/finalproject
git commit -am "finalproject: weapons, targets, particles, audio, scoring, leaderboard"
```

---

## Task 9: OBJ-sequence opening animation + screenshot

**Goal:** Load `frame_000.obj … frame_NNN.obj`, play sequentially (opening cutscene). `F2` saves a PNG via `glReadPixels`. Proves base req 7.

**Files:**
- Create: `projects/finalproject/source/game/obj_sequence.{h,cpp}`
- Create: `projects/finalproject/media/shaders/screenshot` (none — uses glReadPixels)
- Assets: `media/models/seq/frame_000.obj …` (generate a small deformation sequence)
- Modify: `aim_trainer.{h,cpp}`

- [ ] **Step 1: `obj_sequence.{h,cpp}`**

`class ObjSequence { load(prefix, count); update(dt); draw(shader); }`. On update, advance a timer; when it crosses a frame period, load the next obj into a `Model` (rebuild GL resources) and draw it. Loop or stop at end. This is the "read consecutive OBJ → multiple draws" the TA requires.

- [ ] **Step 2: Generate a sequence**

Write a tiny offline generator (or use Task 2 + Task 1's `saveObj`) to emit ~20 frames of a deforming mesh (e.g., a sphere whose radius pulses, or a logo assembling). Save to `media/models/seq/frame_000.obj…frame_019.obj`. (A small throwaway script — not shipped.)

- [ ] **Step 3: Opening cutscene in aim_trainer**

On startup, show the ObjSequence for ~3s before entering the menu/game. ImGui play/pause/speed controls.

- [ ] **Step 4: Screenshot on F2**

On F2 edge: `glReadPixels(0,0,w,h, GL_RGBA, GL_UNSIGNED_BYTE, buf)` after the main pass; flip vertically; `stbi_write_png("media/screenshots/shot_XXX.png", ...)`. Increment filename counter.

- [ ] **Step 5: Build & acceptance**

Expected: on launch, an animated mesh plays (geometry changes per frame, not just transform) before the game. F2 writes a PNG matching the screen.

- [ ] **Step 6: Commit**

```bash
git add projects/finalproject
git commit -am "finalproject: OBJ-sequence opening animation + screenshot"
```

---

## Task 10: Polish — textures, skybox, OBJ export UI, tuning, docs

**Goal:** Skybox, real weapon/balloon textures, "export current scene to .obj" button (uses Task 1's `saveObj`), balance difficulty/dimensions, write the report.

**Files:**
- Modify: `aim_trainer.{h,cpp}` (skybox, export button)
- Add: real textures, skybox cubemap
- Create: report (separate, in `reports/finalproject/`)

- [ ] **Step 1: Skybox** — use base `Skybox` (6-face cubemap) like project6. Draw after main pass with depth write trick.

- [ ] **Step 2: Export-to-OBJ** — ImGui button "Export Scene": iterate visible models, call `saveObj` per object (or one merged file). Proves base req 2 export path end-to-end.

- [ ] **Step 3: Tuning** — adjust level dimensions (gap not too small), ball sizes, speeds, time limits to feel good.

- [ ] **Step 4: Real assets** — swap placeholder textures/sounds for real ones (or keep documented placeholders).

- [ ] **Step 5: Report** — write `reports/finalproject/实验X_3230100113_赵柳舟.{md,pdf}` documenting each requirement → implementation mapping (mirror the §2 score table), screenshots.

- [ ] **Step 6: Final build & full acceptance**

Run end-to-end: opening animation → menu → Standard mode (shoot balloons, timer, leaderboard) → Wild mode (NURBS balls, collision) → Tab to Orbit (inspect lights/shadows via ImGui) → F2 screenshot → export OBJ. Confirm every base requirement + 4 bonus items visibly work.

- [ ] **Step 7: Commit**

```bash
git add projects/finalproject reports/finalproject
git commit -m "finalproject: polish, skybox, OBJ export, report"
```

---

## Self-Review (completed by plan author)

**1. Spec coverage** — every spec section maps to a task:
- Base 1 (6 primitives): Task 2 ✓
- Base 2 (OBJ import/export, self-written): Task 1 ✓
- Base 3 (material/texture edit): Task 3 ✓
- Base 4 (transforms): Task 2/4 ✓
- Base 5 (multi-light): Task 3 ✓
- Base 6 (camera roam): Task 4 ✓
- Base 7a (OBJ-seq animation): Task 9 ✓
- Base 7b (screenshot): Task 9 ✓
- Bonus NURBS: Task 6 ✓
- Bonus shadow: Task 5 ✓
- Bonus collision: Task 7 ✓
- Bonus complete game: Task 8 ✓
- Engineering structure (助教): Task 0 ✓

**2. Placeholder scan** — Task 8 Step 8 and Task 9 Step 2 acknowledge placeholder assets (acceptable: documented, build doesn't depend on them). No TBD/TODO in code steps.

**3. Type consistency** — `Mesh = pair<vector<Vertex>, vector<uint32_t>>` (Task 2) consumed by `Model(vertices, indices)` ctor (Task 1). `RawObj::Corner{p,t,n}` consistent across loader (Task 1) and self-test. `AABB{min,max}` (Task 7) matches resolvePlayer signature. `NurbsCurve::evaluate(t)` (Task 6) consumed by Target (Task 8). `intersectSphereAABB`/`resolvePlayer`/`intersectSphereSphere` signatures consistent.

**4. Ambiguity** — "horizontal-only movement" made explicit (front.y=0). "not too small" made tunable (Task 7 Step 3). NURBS control-point editing: numeric sliders in v1 (Task 6 Step 4) to avoid ray-pick scope creep.

**5. Framework-contract review (post-draft audit against real code)** — verified against `application.cpp`, `glsl_program.h`, `external/imgui/CMakeLists.txt`, root `CMakeLists.txt`, project2/3/6. Fixed:
- CMake mode detection: `if(NOT TARGET glfw)` (old `THIRD_PARTY_PATH` check was always-true → double add_subdirectory).
- Removed nonexistent `imgui/backends` include path (impls are flat, already compiled into the `imgui` target).
- `glEnable(GL_DEPTH_TEST)` added in renderFrame (base ctor does not enable it).
- Asset path via absolute `AIMTRAINER_MEDIA` define (old `../../media/` pointed at root media in root-build mode).
- Key-edge consume pattern (`keyStates[K] = GLFW_RELEASE` after handling) for Tab/F2 (framework has no "just-pressed").
- miniaudio `MINIAUDIO_IMPLEMENTATION` defined in exactly one TU (`audio.cpp`), Windows non-MSVC libs (`ole32 winmm ksuser`) linked.
- First-frame mouse-delta jump guard in FPS camera.
- `git submodule update --init` prerequisite before copying glfw/glm.
- Remaining known limitations (acceptable): window-resize aspect not updated (existing projects ignore this too); `createCone` top cap is degenerate geometry (harmless); NURBS control-point drag is numeric sliders in v1.

> Known scope note: This is a large plan. Tasks 0–7 form the "guaranteed" path (all base reqs + NURBS + shadow + collision). Tasks 8–10 elevate the game polish. If time-constrained, ship a reduced Task 8 (pistol-only, no audio, no leaderboard) and still satisfy base + 3 bonus.
