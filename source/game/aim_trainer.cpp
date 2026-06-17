#include "aim_trainer.h"

#include "fps_camera.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "orbit_camera.h"
#include "primitives.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <cmath>

AimTrainer::AimTrainer(const Options& options)
    : Application(options),
      _camera(glm::radians(45.0f),
              static_cast<float>(options.windowWidth) / options.windowHeight, 0.1f, 100.0f) {
    _camera.transform.position = _fpsSpawn;  // spawn: behind railing, in front of crates, facing the targets

    // ImGui (GL context current after base ctor).
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    // Load a big bold font for the HUD timer (Windows system TTF). Falls back to
    // the default font if the file isn't found.
    ImGuiIO& io = ImGui::GetIO();
    // IMPORTANT: AddFontDefault() first so it becomes the default font (index 0)
    // used by all panels; the big Arial font is a SECOND font pushed only for the
    // HUD timer. Without this, the big font becomes the default and every panel
    // renders huge.
    io.Fonts->AddFontDefault();
    _bigFont = io.Fonts->AddFontFromFileTTF(
        "C:/Windows/Fonts/arialbd.ttf", 56.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init();

    _bpShader = std::make_unique<GLSLProgram>();
    _bpShader->attachVertexShaderFromFile(getAssetFullPath("shaders/blinn_phong.vert"));
    _bpShader->attachFragmentShaderFromFile(getAssetFullPath("shaders/blinn_phong.frag"));
    _bpShader->link();

    _depthShader = std::make_unique<GLSLProgram>();
    _depthShader->attachVertexShaderFromFile(getAssetFullPath("shaders/shadow_depth.vert"));
    _depthShader->attachFragmentShaderFromFile(getAssetFullPath("shaders/shadow_depth.frag"));
    _depthShader->link();

    _scopeShader = std::make_unique<GLSLProgram>();
    _scopeShader->attachVertexShaderFromFile(getAssetFullPath("shaders/scope_mask.vert"));
    _scopeShader->attachFragmentShaderFromFile(getAssetFullPath("shaders/scope_mask.frag"));
    _scopeShader->link();
    _fullscreenQuad = std::make_unique<FullscreenQuad>();

    _points.resize(2);
    _points[0].position = {-2.0f, 2.0f, 2.0f};
    _points[0].color = {1.0f, 0.7f, 0.5f};
    _points[1].position = {2.0f, 2.0f, 2.0f};
    _points[1].color = {0.5f, 0.7f, 1.0f};
    _spots.resize(1);

    // The six showcase primitives are off by default (they were just a base-req-1
    // demo). Re-enable by calling initPrimitives() here.
    // initPrimitives();
    initArena();

    // Shared target-ball mesh (scaled per-target via transform).
    Mesh ballMesh = createSphere(1.0f, 24, 16);   // unit sphere; scaled at draw time
    _targetModel = std::make_unique<Model>(ballMesh.first, ballMesh.second);
    _nurbsBall = std::make_unique<Model>(ballMesh.first, ballMesh.second);

    // Wild-mode NURBS control points (target range beyond the front railing).
    _nurbsControlPoints = {
        {-4.0f, 1.5f, 5.0f}, {-2.0f, 3.0f, 5.5f}, {0.5f, 1.2f, 6.0f},
        {3.0f, 3.2f, 5.5f}, {5.0f, 1.5f, 5.0f}, {5.5f, 2.8f, 6.0f},
        {2.5f, 1.0f, 6.5f}, {-1.0f, 2.5f, 6.0f},
    };
    _nurbsCurve.setPeriodicUniform(3, _nurbsControlPoints);

    // Audio (best-effort; game runs without it). Load the one-shot SFX if present.
    _audioOk = _audio.init();
    if (_audioOk) {
        std::string sfx = getAssetFullPath("sounds/");
        // Each load fails silently (returns false) if the file is missing.
        _audio.load("pistol", sfx + "pistol.wav");
        _audio.load("sniper", sfx + "sniper.wav");
        _audio.load("pop", sfx + "pop.wav");
    }


    // Ground plane (a large box face) to receive shadows.
    Mesh groundMesh = createBox(16.0f, 0.02f, 16.0f);
    _groundPlane = std::make_unique<Model>(groundMesh.first, groundMesh.second);
    _groundPlane->transform.position = {0.5f, 0.0f, 0.0f};

    initShadowMap();

    _fps = std::make_unique<FpsCamera>(&_camera, _window);
    _orbit = std::make_unique<OrbitCamera>(&_camera);
    // Orbit home: view the arena from the spawn side (+z), targets visible ahead.
    _orbit->setDefaultFraming(glm::vec3(0.5f, 1.5f, 2.0f), 12.0f, 90.0f, 15.0f);
    switchMode(CamMode::FPS);

    resetRound();   // start the first round
}

AimTrainer::~AimTrainer() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void AimTrainer::switchMode(CamMode mode) {
    _camMode = mode;
    if (mode == CamMode::FPS) {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        if (_fps) {
            _fps->setPosition(_fpsSpawn);   // restore the FPS spawn (orbit may have moved the camera)
            _fps->resyncCursor(_input);
        }
    } else {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        if (_orbit) {
            _orbit->zoomToFit({});   // go to the home framing when entering orbit
        }
    }
}

void AimTrainer::initPrimitives() {
    auto add = [&](Mesh mesh, glm::vec3 pos, BlinnPhongMaterial mat) {
        Primitive p;
        p.model = std::make_unique<Model>(mesh.first, mesh.second);
        p.position = pos;
        p.material = mat;
        _primitives.push_back(std::move(p));
    };

    add(createSphere(0.6f, 32, 24), {-3.0f, 0.5f, 0.0f},
        BlinnPhongMaterial{{0.1f, 0.4f, 0.9f}, {0.9f, 0.9f, 0.9f}, 64.0f});
    add(createBox(1.0f, 1.0f, 1.0f), {-1.5f, 0.5f, 0.0f},
        BlinnPhongMaterial{{0.85f, 0.3f, 0.2f}, {0.5f, 0.5f, 0.5f}, 32.0f});
    add(createCylinder(0.5f, 1.4f, 32), {0.0f, 0.5f, 0.0f},
        BlinnPhongMaterial{{0.2f, 0.75f, 0.3f}, {0.4f, 0.4f, 0.4f}, 32.0f});
    add(createCone(0.6f, 1.4f, 32), {1.5f, 0.5f, 0.0f},
        BlinnPhongMaterial{{0.9f, 0.75f, 0.1f}, {0.5f, 0.5f, 0.5f}, 32.0f});
    add(createPrismFrustum(0.4f, 0.65f, 1.2f, 6), {3.0f, 0.5f, 0.0f},
        BlinnPhongMaterial{{0.6f, 0.3f, 0.7f}, {0.4f, 0.4f, 0.4f}, 32.0f});
    add(createPrism(0.55f, 1.2f, 5), {4.5f, 0.5f, 0.0f},
        BlinnPhongMaterial{{0.85f, 0.85f, 0.85f}, {0.6f, 0.6f, 0.6f}, 16.0f});
}

void AimTrainer::initArena() {
    // Arena spans x in [-8.5, 9.5]. Player roams x in [-6.5, 7.5], z in
    // [-5.0, 3.2]. The target range is beyond the front railing (z > 3.5). The
    // rear platform is at z in [-5.0, -3.0], top at y = _platformHeight.

    // Helper: add a blocker box (render model + AABB).
    auto addBox = [&](glm::vec3 c, glm::vec3 h) {
        Obstacle o;
        Mesh m = createBox(h.x * 2.0f, h.y * 2.0f, h.z * 2.0f);
        o.model = std::make_unique<Model>(m.first, m.second);
        o.model->transform.position = c;
        o.box.min = c - h;
        o.box.max = c + h;
        _obstacles.push_back(std::move(o));
    };

    // --- Front RAILING: waist-high, spans the FULL width between the side walls
    // (no gap at the sides) so the player cannot slip into the target range.
    float railTop = 1.0f;
    addBox({0.0f, railTop * 0.5f, 3.3f}, {7.9f, railTop * 0.5f, 0.1f});  // spans x[-7.9,7.9]
    addBox({-7.9f, railTop * 0.5f, 3.3f}, {0.1f, railTop * 0.5f, 0.1f}); // left post
    addBox({7.9f, railTop * 0.5f, 3.3f}, {0.1f, railTop * 0.5f, 0.1f});  // right post

    // --- SYMMETRIC CRATES flanking the play area (not against the walls), so
    // the player can pass on either side. Tall enough to block the eye line.
    addBox({-3.0f, 1.0f, 0.0f}, {0.8f, 1.0f, 0.8f});   // top at y=2.0
    addBox({3.0f, 1.0f, 0.0f}, {0.8f, 1.0f, 0.8f});    // top at y=2.0

    // --- Rear PLATFORM behind the spawn (target range is +z in FRONT; the
    // platform is at -z, behind the player). Top at y = _platformHeight.
    // Footprint spans between the side walls so there is no gap at the sides.
    // Left wall inner face x=-7.9, right wall inner face x=7.9.
    float platXMin = -7.9f, platXMax = 7.9f;
    float platZMin = -5.0f, platZMax = -3.0f;
    float platHalfH = _platformHeight * 0.5f;
    {
        float w = platXMax - platXMin;
        float d = platZMax - platZMin;
        Mesh m = createBox(w, _platformHeight, d);
        _platform = std::make_unique<Model>(m.first, m.second);
        _platform->transform.position = {0.5f * (platXMin + platXMax), platHalfH,
                                         0.5f * (platZMin + platZMax)};
    }

    // --- RAMPS hugging the side walls. HIGH end (y = platformHeight) meets the
    // platform front edge (z = -3); LOW end (y = 0) extends to the spawn side.
    // Gentle slope (run = 3.0). Each ramp fills the wall-to-gap width so the
    // player cannot step off its side into a seam.
    float run = 3.0f;                          // ramp horizontal length (along z)
    float rise = _platformHeight;
    float rampLen = std::sqrt(run * run + rise * rise);
    float rampAngle = std::atan2(rise, run);
    // Left ramp x in [-7.9, -5.5], right ramp x in [5.5, 7.9] (narrower, leaving
    // ample room between the crates (x ~ +-3.8) and the ramps).
    float rampLXMin = platXMin, rampLXMax = -5.5f;
    float rampRXMin = 5.5f, rampRXMax = platXMax;
    auto makeRamp = [&](float xMin, float xMax) {
        float w = xMax - xMin;
        Mesh m = createBox(w, 0.1f, rampLen);  // thin slab, length along Z
        auto model = std::make_unique<Model>(m.first, m.second);
        model->transform.position = {0.5f * (xMin + xMax), rise * 0.5f, -1.5f};
        // Tilt about +X: +Z end down (low, toward spawn), -Z end up (high, at platform).
        model->transform.rotation = glm::angleAxis(rampAngle, glm::vec3(1.0f, 0.0f, 0.0f));
        return model;
    };
    _rampLeft = makeRamp(rampLXMin, rampLXMax);
    _rampRight = makeRamp(rampRXMin, rampRXMax);

    // --- Enclosing WALLS (blockers). Inner faces aligned to platform edges so
    // there is no seam to fall through.
    // Platform back edge is z = -5.0; back-wall front face touches it (center z=-5.3).
    addBox({0.5f, 2.0f, -5.3f}, {8.2f, 2.0f, 0.3f});    // back wall (flush with platform back)
    addBox({-8.2f, 2.0f, -1.0f}, {0.3f, 2.0f, 5.5f});   // left wall (inner face x=-7.9)
    addBox({8.2f, 2.0f, -1.0f}, {0.3f, 2.0f, 5.5f});    // right wall (inner face x=7.9)

    // --- GROUND PATCHES (walkable surfaces driving the height system).
    // Platform top: flat at _platformHeight over its full footprint.
    _ground.push_back({{platXMin, platZMin}, {platXMax, platZMax},
                       _platformHeight, _platformHeight});
    // Left ramp: x in [rampLXMin, rampLXMax], z in [-3, 0].
    // surfaceY at z=-3 (min.y) = yLow = platformHeight (high end, meets platform);
    // surfaceY at z=0 (max.y) = yHigh = 0 (low end, toward spawn).
    _ground.push_back({{rampLXMin, -3.0f}, {rampLXMax, 0.0f}, _platformHeight, 0.0f});
    _ground.push_back({{rampRXMin, -3.0f}, {rampRXMax, 0.0f}, _platformHeight, 0.0f});
}

std::vector<AABB> AimTrainer::obstacleBoxes() const {
    std::vector<AABB> boxes;
    boxes.reserve(_obstacles.size());
    for (const auto& o : _obstacles) {
        boxes.push_back(o.box);
    }
    return boxes;
}

BoundingBox AimTrainer::sceneBounds() const {
    BoundingBox b;
    b.min = {-4.0f, 0.0f, -1.0f};
    b.max = {5.0f, 2.0f, 1.0f};
    return b;
}

void AimTrainer::setLightUniforms() {
    _bpShader->setUniformVec3("uSkyColor", _skyColor);
    _bpShader->setUniformVec3("uGroundColor", _groundColor);

    _bpShader->setUniformBool("uDirLightOn", _dir.on);
    _bpShader->setUniformVec3("uDirLightDir", _dir.dir);
    _bpShader->setUniformVec3("uDirLightColor", _dir.color);
    _bpShader->setUniformFloat("uDirLightIntensity", _dir.intensity);

    int ptCount = 0;
    for (size_t i = 0; i < _points.size(); ++i) {
        if (!_points[i].on) {
            continue;
        }
        std::string base = "uPoints[" + std::to_string(ptCount) + "]";
        _bpShader->setUniformVec3(base + ".position", _points[i].position);
        _bpShader->setUniformVec3(base + ".color", _points[i].color);
        _bpShader->setUniformFloat(base + ".intensity", _points[i].intensity);
        _bpShader->setUniformFloat(base + ".kc", _points[i].kc);
        _bpShader->setUniformFloat(base + ".kl", _points[i].kl);
        _bpShader->setUniformFloat(base + ".kq", _points[i].kq);
        ++ptCount;
    }
    _bpShader->setUniformInt("uPointCount", ptCount);

    int spCount = 0;
    for (size_t i = 0; i < _spots.size(); ++i) {
        if (!_spots[i].on) {
            continue;
        }
        std::string base = "uSpots[" + std::to_string(spCount) + "]";
        _bpShader->setUniformVec3(base + ".position", _spots[i].position);
        _bpShader->setUniformVec3(base + ".direction", _spots[i].direction);
        _bpShader->setUniformVec3(base + ".color", _spots[i].color);
        _bpShader->setUniformFloat(base + ".intensity", _spots[i].intensity);
        _bpShader->setUniformFloat(base + ".cosAngle", _spots[i].cosAngle);
        _bpShader->setUniformFloat(base + ".kc", _spots[i].kc);
        _bpShader->setUniformFloat(base + ".kl", _spots[i].kl);
        _bpShader->setUniformFloat(base + ".kq", _spots[i].kq);
        ++spCount;
    }
    _bpShader->setUniformInt("uSpotCount", spCount);
}

void AimTrainer::drawImGui() {
    if (!_showUi) {
        return;
    }
    ImGui::Begin("AimTrainer (Paused)  [F1 to resume]", &_showUi,
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);

    // Resume button: closes the panel and unpauses.
    if (ImGui::Button("Resume Game (F1)")) {
        _showUi = false;
        _paused = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Exit Game")) {
        glfwSetWindowShouldClose(_window, GLFW_TRUE);
    }

    if (ImGui::CollapsingHeader("Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("mouse sensitivity", &_mouseSensitivity, 0.01f, 0.5f);
    }
    if (ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool fs = _fullscreen;
        if (ImGui::Checkbox("Fullscreen", &fs)) {
            _fullscreen = fs;
            toggleFullscreen();
        }
    }

    ImGui::Text("Camera: %s  [Tab to switch]", _camMode == CamMode::FPS ? "FPS" : "Orbit");
    if (_camMode == CamMode::Orbit) {
        ImGui::SameLine();
        if (ImGui::Button("Zoom To Fit (F)")) {
            _orbit->zoomToFit(sceneBounds());
        }
    }

    if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("On", &_dir.on);
        ImGui::SliderFloat3("dir (to light)", &_dir.dir[0], -1.0f, 1.0f);
        ImGui::ColorEdit3("color", &_dir.color[0]);
        ImGui::SliderFloat("intensity", &_dir.intensity, 0.0f, 5.0f);
        ImGui::Checkbox("Shadows (directional)", &_shadowsOn);
    }
    for (size_t i = 0; i < _points.size(); ++i) {
        std::string title = "Point " + std::to_string(i);
        if (ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox(("on##pt" + std::to_string(i)).c_str(), &_points[i].on);
            ImGui::SliderFloat3(("position##pt" + std::to_string(i)).c_str(),
                                &_points[i].position[0], -6.0f, 6.0f);
            ImGui::ColorEdit3(("color##pt" + std::to_string(i)).c_str(), &_points[i].color[0]);
            ImGui::SliderFloat(("intensity##pt" + std::to_string(i)).c_str(),
                               &_points[i].intensity, 0.0f, 10.0f);
        }
    }
    for (size_t i = 0; i < _spots.size(); ++i) {
        std::string title = "Spot " + std::to_string(i);
        if (ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox(("on##sp" + std::to_string(i)).c_str(), &_spots[i].on);
            ImGui::SliderFloat3(("position##sp" + std::to_string(i)).c_str(),
                                &_spots[i].position[0], -6.0f, 6.0f);
            ImGui::SliderFloat3(("direction##sp" + std::to_string(i)).c_str(),
                                &_spots[i].direction[0], -1.0f, 1.0f);
            ImGui::ColorEdit3(("color##sp" + std::to_string(i)).c_str(), &_spots[i].color[0]);
            ImGui::SliderFloat(("intensity##sp" + std::to_string(i)).c_str(),
                               &_spots[i].intensity, 0.0f, 20.0f);
            float halfDeg = glm::degrees(std::acos(_spots[i].cosAngle));
            if (ImGui::SliderFloat(("cone half-angle##sp" + std::to_string(i)).c_str(),
                                   &halfDeg, 1.0f, 89.0f)) {
                _spots[i].cosAngle = std::cos(glm::radians(halfDeg));
            }
            if (i == 0) {
                ImGui::Checkbox(("cast shadows##sp" + std::to_string(i)).c_str(),
                                &_spotShadowsOn);
            }
        }
    }
    if (ImGui::CollapsingHeader("Hemisphere Ambient", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorEdit3("sky color", &_skyColor[0]);
        ImGui::ColorEdit3("ground color", &_groundColor[0]);
    }
    if (ImGui::CollapsingHeader("Game", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Mode.
        if (ImGui::RadioButton("Standard (static)", _game.mode == GameMode::Standard)) {
            _game.mode = GameMode::Standard;
            resetRound();
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Wild (NURBS)", _game.mode == GameMode::Wild)) {
            _game.mode = GameMode::Wild;
            resetRound();
        }
        // Weapon.
        if (ImGui::RadioButton("Pistol [1]", _weapon == Weapon::Pistol)) {
            _weapon = Weapon::Pistol;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Sniper [2]", _weapon == Weapon::Sniper)) {
            _weapon = Weapon::Sniper;
        }
        // Tunable params.
        ImGui::SliderFloat("time limit (s)", &_game.timeLimit, 0.0f, 180.0f);
        ImGui::SliderFloat("ball size", &_game.ballSize, 0.2f, 1.0f);
        ImGui::SliderFloat("wild speed", &_game.wildSpeed, 0.0f, 4.0f);
        // Restart.
        if (ImGui::Button("Restart Round")) {
            resetRound();
        }
        // Score / timer.
        ImGui::Text("Hits: %d / Shots: %d  (%.0f%%)", _game.hits, _game.shots,
                    _game.accuracy() * 100.0f);
        if (_game.timeLimit > 0.0f) {
            ImGui::Text("Time left: %.1f s", _game.timeLeft);
            if (_game.finished()) {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "ROUND OVER");
            }
        }
    }

    ImGui::Text("F1 UI | WASD move | Tab camera | LMB fire | RMB scope (sniper)");
    ImGui::End();
}

void AimTrainer::handleInput() {
    float dt = getDeltaTime();

    // F1 toggles the pause/control panel (consume edge).
    if (_input.keyboard.keyStates[GLFW_KEY_F1] == GLFW_PRESS) {
        _showUi = !_showUi;
        _paused = _showUi;
        _input.keyboard.keyStates[GLFW_KEY_F1] = GLFW_RELEASE;
    }
    // If the panel was closed by other means (X button / Resume button), unpause.
    if (!_showUi) {
        _paused = false;
    }

    // Determine whether the UI needs the mouse (panel open or round-over modal).
    bool uiNeedsMouse = _showUi || _showRoundOver;
    // Cursor: free+visible when UI needs it; locked when playing in FPS mode.
    if (uiNeedsMouse) {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else if (_camMode == CamMode::FPS) {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    // When paused or the round is over, freeze the game (no tick / camera / fire).
    if (_paused || _showRoundOver) {
        _input.forwardState();
        return;
    }

    // --- Per-frame game updates (only while actively playing) ---
    _game.tick(dt);
    checkRoundEnd();
    updateWeapon(dt);
    updateTargets(dt);
    _particles.update(dt);
    tryFire();

    // Sniper scope: right mouse (hold).
    _scoped = (_weapon == Weapon::Sniper) && _input.mouse.press.right;
    // Weapon select (1/2).
    if (_input.keyboard.keyStates[GLFW_KEY_1] == GLFW_PRESS) {
        _weapon = Weapon::Pistol;
        _input.keyboard.keyStates[GLFW_KEY_1] = GLFW_RELEASE;
    }
    if (_input.keyboard.keyStates[GLFW_KEY_2] == GLFW_PRESS) {
        _weapon = Weapon::Sniper;
        _input.keyboard.keyStates[GLFW_KEY_2] = GLFW_RELEASE;
    }

    // Tab switches camera (consume edge).
    if (_input.keyboard.keyStates[GLFW_KEY_TAB] == GLFW_PRESS) {
        switchMode(_camMode == CamMode::FPS ? CamMode::Orbit : CamMode::FPS);
        _input.keyboard.keyStates[GLFW_KEY_TAB] = GLFW_RELEASE;
    }
    // F = zoom to fit in orbit mode.
    if (_camMode == CamMode::Orbit &&
        _input.keyboard.keyStates[GLFW_KEY_F] == GLFW_PRESS) {
        _orbit->zoomToFit(sceneBounds());
        _input.keyboard.keyStates[GLFW_KEY_F] = GLFW_RELEASE;
    }

    // Let the active camera consume input (but not while ImGui grabs the mouse).
    if (!ImGui::GetIO().WantCaptureMouse) {
        if (_camMode == CamMode::FPS) {
            _fps->setSensitivity(_mouseSensitivity);
            _fps->update(_input, dt, obstacleBoxes(), _ground, _baseGroundY);
        } else {
            _orbit->update(_input, dt);
        }
    }
    _input.forwardState();
}

void AimTrainer::initShadowMap() {
    _shadowTex = std::make_unique<Texture2D>(
        GL_DEPTH_COMPONENT, _shadowResolution, _shadowResolution, GL_DEPTH_COMPONENT, GL_FLOAT);
    _shadowTex->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    _shadowTex->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    _shadowTex->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    _shadowTex->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    _shadowTex->setParamterFloatVector(GL_TEXTURE_BORDER_COLOR, {1.0f, 1.0f, 1.0f, 1.0f});
    // (optional) disable comparison; we sample depth directly in the shader.

    _shadowFbo = std::make_unique<Framebuffer>();
    _shadowFbo->bind();
    _shadowFbo->drawBuffer(GL_NONE);
    _shadowFbo->readBuffer(GL_NONE);
    _shadowFbo->attachTexture2D(*_shadowTex, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D);
    GLenum status = _shadowFbo->checkStatus();
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Shadow FBO incomplete: " + _shadowFbo->getDiagnostic(status));
    }
    _shadowFbo->unbind();

    // Spotlight shadow map (same size; perspective depth).
    _spotShadowTex = std::make_unique<Texture2D>(
        GL_DEPTH_COMPONENT, _shadowResolution, _shadowResolution, GL_DEPTH_COMPONENT, GL_FLOAT);
    _spotShadowTex->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    _spotShadowTex->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    _spotShadowTex->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    _spotShadowTex->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    _spotShadowTex->setParamterFloatVector(GL_TEXTURE_BORDER_COLOR, {1.0f, 1.0f, 1.0f, 1.0f});

    _spotShadowFbo = std::make_unique<Framebuffer>();
    _spotShadowFbo->bind();
    _spotShadowFbo->drawBuffer(GL_NONE);
    _spotShadowFbo->readBuffer(GL_NONE);
    _spotShadowFbo->attachTexture2D(*_spotShadowTex, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D);
    status = _spotShadowFbo->checkStatus();
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Spot shadow FBO incomplete: " +
                                 _spotShadowFbo->getDiagnostic(status));
    }
    _spotShadowFbo->unbind();
}

void AimTrainer::updateLightSpaceMatrix() {
    // Build an orthographic frustum around the scene, viewed from the light.
    // _dir.dir is the direction TO the light, so the light sits at
    // center + dir * distance (toward the light source).
    glm::vec3 dir = glm::normalize(_dir.dir);
    glm::vec3 sceneCenter{0.5f, 1.0f, 0.0f};
    glm::vec3 lightPos = sceneCenter + dir * 12.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 40.0f);
    _lightSpaceMatrix = lightProj * lightView;
}

void AimTrainer::updateSpotSpaceMatrix() {
    // View the scene from the spot light along its direction, with a perspective
    // frustum a bit wider than the cone so the whole lit cone is captured.
    if (_spots.empty() || !_spots[0].on) {
        _spotSpaceMatrix = glm::mat4(1.0f);
        return;
    }
    const SpLight& s = _spots[0];
    glm::vec3 pos = s.position;
    glm::vec3 dir = glm::normalize(s.direction);
    glm::vec3 target = pos + dir;   // look along the spot direction
    glm::mat4 view = glm::lookAt(pos, target, glm::vec3(0.0f, 1.0f, 0.0f));
    // FOV = 2 * cone half-angle, plus margin.
    float halfDeg = glm::degrees(std::acos(s.cosAngle));
    float fovy = glm::radians(halfDeg * 2.0f + 8.0f);
    glm::mat4 proj = glm::perspective(fovy, 1.0f, 0.1f, 60.0f);
    _spotSpaceMatrix = proj * view;
}

void AimTrainer::renderSpotDepthPass() {
    if (!_spotShadowsOn || _spots.empty() || !_spots[0].on) {
        return;
    }
    _spotShadowFbo->bind();
    glViewport(0, 0, _shadowResolution, _shadowResolution);
    glClear(GL_DEPTH_BUFFER_BIT);

    _depthShader->use();
    _depthShader->setUniformMat4("uLightSpace", _spotSpaceMatrix);  // reuse uLightSpace uniform
    drawScene(*_depthShader, false);
    _depthShader->unuse();

    _spotShadowFbo->unbind();
    glViewport(0, 0, _windowWidth, _windowHeight);
}

void AimTrainer::renderDepthPass() {
    if (!_shadowsOn || !_dir.on) {
        return;
    }
    _shadowFbo->bind();
    glViewport(0, 0, _shadowResolution, _shadowResolution);
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT);  // reduce peter-panning

    _depthShader->use();
    _depthShader->setUniformMat4("uLightSpace", _lightSpaceMatrix);
    drawScene(*_depthShader, false);
    _depthShader->unuse();

    glCullFace(GL_BACK);
    _shadowFbo->unbind();
    // Restore the default viewport.
    glViewport(0, 0, _windowWidth, _windowHeight);
}

void AimTrainer::drawScene(GLSLProgram& shader, bool withMaterial) {
    // Primitives.
    for (const auto& p : _primitives) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), p.position);
        model = glm::rotate(model, glm::radians(_angle * 0.3f), glm::vec3(0, 1, 0));
        shader.setUniformMat4("uModel", model);
        if (withMaterial) {
            shader.setUniformVec3("uKd", p.material.kd);
            shader.setUniformVec3("uKs", p.material.ks);
            shader.setUniformFloat("uShininess", p.material.shininess);
            shader.setUniformBool("uHasTexture", false);
        }
        p.model->draw();
    }
    // Target balls (Standard static / Wild NURBS-driven). Cast + receive shadows.
    if (withMaterial) {
        drawTargets(shader);
    } else {
        // depth pass: draw targets without setting material uniforms
        drawTargets(shader);
    }
    // Rear platform + ramps (walkable via the ground-height system).
    auto drawGroundModel = [&](Model* mdl, glm::vec3 kd) {
        if (!mdl) {
            return;
        }
        shader.setUniformMat4("uModel", mdl->transform.getLocalMatrix());
        if (withMaterial) {
            shader.setUniformVec3("uKd", kd);
            shader.setUniformVec3("uKs", glm::vec3(0.1f));
            shader.setUniformFloat("uShininess", 16.0f);
            shader.setUniformBool("uHasTexture", false);
        }
        mdl->draw();
    };
    drawGroundModel(_platform.get(), glm::vec3(0.45f, 0.4f, 0.38f));
    drawGroundModel(_rampLeft.get(), glm::vec3(0.5f, 0.42f, 0.35f));
    drawGroundModel(_rampRight.get(), glm::vec3(0.5f, 0.42f, 0.35f));

    // Arena obstacles (collidable boxes: railing, crates, walls).
    for (const auto& o : _obstacles) {
        shader.setUniformMat4("uModel", o.model->transform.getLocalMatrix());
        if (withMaterial) {
            shader.setUniformVec3("uKd", glm::vec3(0.5f, 0.45f, 0.4f));
            shader.setUniformVec3("uKs", glm::vec3(0.1f));
            shader.setUniformFloat("uShininess", 16.0f);
            shader.setUniformBool("uHasTexture", false);
        }
        o.model->draw();
    }
    // Ground (flat grey, receives shadows).
    if (_groundPlane) {
        glm::mat4 model = _groundPlane->transform.getLocalMatrix();
        shader.setUniformMat4("uModel", model);
        if (withMaterial) {
            shader.setUniformVec3("uKd", glm::vec3(0.35f, 0.35f, 0.38f));
            shader.setUniformVec3("uKs", glm::vec3(0.0f));
            shader.setUniformFloat("uShininess", 8.0f);
            shader.setUniformBool("uHasTexture", false);
        }
        _groundPlane->draw();
    }
}

void AimTrainer::renderFrame() {
    glEnable(GL_DEPTH_TEST);

    // Keep aspect in sync with the framebuffer size.
    _camera.aspect = static_cast<float>(_windowWidth) / static_cast<float>(_windowHeight);

    _angle += 0.5f;
    updateLightSpaceMatrix();
    updateSpotSpaceMatrix();

    // --- Pass 1: directional shadow depth ---
    renderDepthPass();
    // --- Pass 1b: spotlight shadow depth ---
    renderSpotDepthPass();

    // --- Pass 2: main Blinn-Phong (sampling both shadow maps) ---
    // Sniper scope zooms in: narrow the FOV while scoped (reset after drawing).
    float baseFovy = glm::radians(45.0f);
    if (_scoped) {
        _camera.fovy = glm::radians(15.0f);   // ~3x zoom
    } else {
        _camera.fovy = baseFovy;
    }

    glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, _windowWidth, _windowHeight);

    glm::mat4 proj = _camera.getProjectionMatrix();
    glm::mat4 view = _camera.getViewMatrix();

    _bpShader->use();
    _bpShader->setUniformMat4("uView", view);
    _bpShader->setUniformMat4("uProj", proj);
    _bpShader->setUniformMat4("uLightSpace", _lightSpaceMatrix);
    _bpShader->setUniformMat4("uSpotSpace", _spotSpaceMatrix);
    _bpShader->setUniformVec3("uViewPos", _camera.transform.position);
    setLightUniforms();
    _bpShader->setUniformBool("uShadowsOn", _shadowsOn);
    _bpShader->setUniformBool("uSpotShadowsOn", _spotShadowsOn);
    // Bind shadow maps to slots 1 and 2 (slot 0 reserved for material texture).
    _bpShader->setUniformInt("uShadowMap", 1);
    _shadowTex->bind(1);
    _bpShader->setUniformInt("uSpotShadowMap", 2);
    _spotShadowTex->bind(2);

    drawScene(*_bpShader, true);
    _bpShader->unuse();

    // Particles (additive points) on top of the scene.
    drawParticles();

    // Sniper scope mask: black everywhere outside the scope circle (zoom is done
    // above by narrowing fovy). The crosshair/ring are drawn by ImGui (HUD).
    if (_scoped) {
        glDisable(GL_DEPTH_TEST);
        _scopeShader->use();
        float aspect = static_cast<float>(_windowWidth) / static_cast<float>(_windowHeight);
        _scopeShader->setUniformVec2("uCenter", glm::vec2(0.5f, 0.5f));
        // Radius is relative to the shorter axis (height), matching the ImGui ring.
        _scopeShader->setUniformFloat("uRadius", 0.36f);
        _scopeShader->setUniformFloat("uAspect", aspect);
        _fullscreenQuad->draw();
        _scopeShader->unuse();
        glEnable(GL_DEPTH_TEST);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    drawImGui();
    drawGameHud();          // crosshair / scope vignette (foreground draw list)
    drawRoundOverModal();   // big result modal when a round ends
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// ===================== Gameplay (Task 8) =====================

void AimTrainer::resetRound() {
    _game.start();
    _targets.clear();
    _wildPaths.clear();
    if (_game.mode == GameMode::Wild) {
        // Several targets, each on its own offset NURBS path. Build ALL paths
        // first (reserve to avoid reallocation), THEN take pointers -- otherwise
        // push_back would invalidate the pointers stored on earlier targets.
        const int n = 3;
        _wildPaths.reserve(n);
        for (int i = 0; i < n; ++i) {
            NurbsCurve c;
            std::vector<glm::vec3> cps = _nurbsControlPoints;
            float off = 1.5f * i;
            for (auto& p : cps) {
                p.y += off * 0.3f;
                p.x += off * 0.2f;
            }
            c.setPeriodicUniform(3, cps);
            _wildPaths.push_back(std::move(c));
        }
        // Now that _wildPaths is stable, create targets referencing each path.
        for (int i = 0; i < n; ++i) {
            Target t;
            t.path = &_wildPaths[i];          // stable: no more push_back
            t.param = (float)i / n;            // staggered phase
            t.radius = _game.ballSize;
            t.color = glm::vec3(1.0f, 0.5f + 0.2f * i, 0.2f);
            t.alive = true;
            _targets.push_back(t);
        }
    } else {
        // Standard: one static target, centered in the target range at eye
        // height so the spawn camera (aimed at it) starts dead-center on the ball.
        Target t;
        t.radius = _game.ballSize;
        t.color = glm::vec3(1.0f, 0.3f, 0.3f);
        t.alive = true;
        _targets.push_back(t);
        _targets[0].position = glm::vec3(0.0f, 1.7f, 5.5f);  // centered
    }
    // Aim the camera dead-center on the first target.
    if (_fps && !_targets.empty()) {
        _fps->setPosition(_fpsSpawn);
        _fps->lookToward(_targets[0].position);
    }
    _showRoundOver = false;
}

void AimTrainer::spawnTarget(Target& t) {
    // Random position in the target range (beyond the railing), kept in view.
    float x = glm::linearRand(-4.0f, 4.0f);
    float y = glm::linearRand(1.0f, 3.0f);
    float z = glm::linearRand(5.0f, 6.5f);
    t.position = glm::vec3(x, y, z);
    t.alive = true;
}

void AimTrainer::updateTargets(float dt) {
    if (_game.mode == GameMode::Wild) {
        float period = (float)_nurbsControlPoints.size();
        for (auto& t : _targets) {
            if (!t.path) {
                continue;
            }
            t.param += _game.wildSpeed * dt;
            t.param = std::fmod(t.param, period);
            if (t.param < 0.0f) {
                t.param += period;
            }
            t.position = t.path->evaluate(t.param);
        }
        // sphere-sphere repulsion between wild targets (bonus collision)
        for (size_t i = 0; i < _targets.size(); ++i) {
            for (size_t j = i + 1; j < _targets.size(); ++j) {
                glm::vec3 nrm;
                if (intersectSphereSphere(_targets[i].position, _targets[i].radius,
                                          _targets[j].position, _targets[j].radius, nrm)) {
                    // push apart along the NURBS param (simple: nudge positions)
                    float push = 0.05f;
                    _targets[i].position += nrm * push;
                    _targets[j].position -= nrm * push;
                }
            }
        }
    }
}

glm::vec3 AimTrainer::aimRayDir() const {
    // Camera looks along its transform front; scope does not change direction.
    return _camera.transform.getFront();
}

void AimTrainer::updateWeapon(float dt) {
    if (_fireCooldown > 0.0f) {
        _fireCooldown -= dt;
    }
}

void AimTrainer::tryFire() {
    bool wantFire = _input.mouse.press.left && !_leftMousePrev;
    _leftMousePrev = _input.mouse.press.left;
    if (!wantFire || _camMode != CamMode::FPS) {
        return;
    }
    // Sniper enforces a cooldown; pistol is unrestricted.
    if (_weapon == Weapon::Sniper && _fireCooldown > 0.0f) {
        return;
    }
    if (_weapon == Weapon::Sniper) {
        _fireCooldown = 1.0f;   // ~1 shot/sec
    }

    _game.registerShot();
    glm::vec3 origin = _camera.transform.position;
    glm::vec3 dir = glm::normalize(aimRayDir());

    // Ray-sphere intersection; pick the closest alive target hit.
    int best = -1;
    float bestT = 1e9f;
    for (int i = 0; i < (int)_targets.size(); ++i) {
        if (!_targets[i].alive) {
            continue;
        }
        glm::vec3 oc = origin - _targets[i].position;
        float b = glm::dot(oc, dir);
        float c = glm::dot(oc, oc) - _targets[i].radius * _targets[i].radius;
        float disc = b * b - c;
        if (disc < 0.0f) {
            continue;
        }
        float t = -b - std::sqrt(disc);
        if (t > 0.0f && t < bestT) {
            bestT = t;
            best = i;
        }
    }

    // Muzzle flash + sound.
    _particles.burst(origin + dir * 0.6f, glm::vec3(1.0f, 0.8f, 0.3f), 8, 2.0f, 0.1f, 6.0f);
    if (_audioOk) {
        _audio.play(_weapon == Weapon::Sniper ? "sniper" : "pistol");
    }

    if (best >= 0) {
        _game.registerHit();
        _targets[best].alive = false;
        _particles.burst(_targets[best].position, _targets[best].color, 30, 4.0f, 0.6f, 10.0f);
        if (_audioOk) {
            _audio.play("pop");
        }
        if (_game.mode == GameMode::Standard) {
            spawnTarget(_targets[best]);   // immediately respawn
        } else {
            // Wild: respawn at a new phase so the ball keeps moving.
            _targets[best].alive = true;
            _targets[best].param = glm::linearRand(0.0f, 1.0f);
        }
    }
}

void AimTrainer::drawTargets(GLSLProgram& shader) {
    if (!_targetModel) {
        return;
    }
    for (const auto& t : _targets) {
        if (!t.alive) {
            continue;
        }
        glm::mat4 model = glm::translate(glm::mat4(1.0f), t.position);
        model = glm::scale(model, glm::vec3(t.radius));   // unit sphere -> radius
        shader.setUniformMat4("uModel", model);
        shader.setUniformVec3("uKd", t.color);
        shader.setUniformVec3("uKs", glm::vec3(0.6f));
        shader.setUniformFloat("uShininess", 48.0f);
        shader.setUniformBool("uHasTexture", false);
        _targetModel->draw();
    }
}

void AimTrainer::drawParticles() {
    _particles.draw(_camera.getViewMatrix(), _camera.getProjectionMatrix(),
                    getAssetFullPath("shaders/"));
}

void AimTrainer::drawGameHud() {
    ImGuiIO& io = ImGui::GetIO();

    // ---- Top-center countdown timer (0.01s precision) ----
    if (_game.timeLimit > 0.0f && !_showRoundOver) {
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, 12.0f),
                                ImGuiCond_Always, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.55f);
        int flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;
        if (ImGui::Begin("TimerHud", nullptr, flags)) {
            if (_bigFont) {
                ImGui::PushFont(_bigFont);
            }
            // Color: white normally, yellow under 10s, red under 3s.
            ImVec4 tcol(1.0f, 1.0f, 1.0f, 1.0f);
            if (_game.timeLeft <= 3.0f) {
                tcol = ImVec4(1.0f, 0.25f, 0.25f, 1.0f);
            } else if (_game.timeLeft <= 10.0f) {
                tcol = ImVec4(1.0f, 0.85f, 0.2f, 1.0f);
            }
            ImGui::TextColored(tcol, "%.2f", _game.timeLeft);
            if (_bigFont) {
                ImGui::PopFont();
            }
        }
        ImGui::End();
    }

    // Crosshair at screen center.
    ImVec2 ctr(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImU32 col = IM_COL32(255, 255, 255, 220);
    if (_weapon == Weapon::Sniper && _scoped) {
        // Scope vignette: mask everything outside the scope circle. ImGui is not
        // destination-out, so we can't "punch a hole". Instead cover the four
        // rectangular regions around the circle (top/bottom/left/right bands),
        // approximating the circle closely.
        float r = std::min(io.DisplaySize.x, io.DisplaySize.y) * 0.36f;
        ImU32 black = IM_COL32(0, 0, 0, 255);
        dl->AddRectFilled(ImVec2(0, 0), ImVec2(io.DisplaySize.x, ctr.y - r), black);            // top band
        dl->AddRectFilled(ImVec2(0, ctr.y + r), ImVec2(io.DisplaySize.x, io.DisplaySize.y), black); // bottom
        dl->AddRectFilled(ImVec2(0, ctr.y - r), ImVec2(ctr.x - r, ctr.y + r), black);           // left
        dl->AddRectFilled(ImVec2(ctr.x + r, ctr.y - r), ImVec2(io.DisplaySize.x, ctr.y + r), black); // right
        // Scope ring + crosshair.
        dl->AddCircle(ctr, r, IM_COL32(255, 255, 255, 255), 64, 4.0f);
        dl->AddLine(ImVec2(ctr.x - r, ctr.y), ImVec2(ctr.x + r, ctr.y), col, 1.0f);
        dl->AddLine(ImVec2(ctr.x, ctr.y - r), ImVec2(ctr.x, ctr.y + r), col, 1.0f);
    } else if (_weapon == Weapon::Pistol) {
        // Small crosshair only for the pistol; the sniper has no crosshair when
        // not scoped (you aim down the scope).
        dl->AddLine(ImVec2(ctr.x - 10, ctr.y), ImVec2(ctr.x - 3, ctr.y), col, 2.0f);
        dl->AddLine(ImVec2(ctr.x + 3, ctr.y), ImVec2(ctr.x + 10, ctr.y), col, 2.0f);
        dl->AddLine(ImVec2(ctr.x, ctr.y - 10), ImVec2(ctr.x, ctr.y - 3), col, 2.0f);
        dl->AddLine(ImVec2(ctr.x, ctr.y + 3), ImVec2(ctr.x, ctr.y + 10), col, 2.0f);
    }
}

void AimTrainer::checkRoundEnd() {
    if (_game.finished() && !_showRoundOver) {
        // Record the result into the leaderboard (in-memory, per mode).
        int idx = (_game.mode == GameMode::Wild) ? 1 : 0;
        _scores[idx].push_back({_game.hits, _game.shots, _game.accuracy(), _game.mode});
        _showRoundOver = true;
    }
}

void AimTrainer::restartRound() {
    resetRound();   // resets state + re-aims camera at the centered first target
}

void AimTrainer::toggleFullscreen() {
    if (_fullscreen) {
        // Go exclusive-fullscreen on the primary monitor at its native resolution.
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        // Remember the windowed geometry so we can restore it later.
        glfwGetWindowPos(_window, &_windowedX, &_windowedY);
        glfwGetWindowSize(_window, &_windowedW, &_windowedH);
        glfwSetWindowMonitor(_window, monitor, 0, 0, mode->width, mode->height,
                             mode->refreshRate);
    } else {
        // Restore windowed mode.
        glfwSetWindowMonitor(_window, nullptr, _windowedX, _windowedY, _windowedW,
                             _windowedH, 0);
    }
    // Scale the whole UI up a bit in fullscreen (1.5x), normal in windowed.
    // FontGlobalScale is cheap; the font atlas itself is unchanged (it's
    // rasterized at native size and scaled when drawing).
    ImGui::GetIO().FontGlobalScale = _fullscreen ? 1.5f : 1.0f;
    // The framebuffer-size callback (in the base) updates _windowWidth/Height and
    // the viewport; the camera aspect is recomputed each frame in renderFrame.
}

void AimTrainer::drawRoundOverModal() {
    if (!_showRoundOver) {
        return;
    }
    // Big centered, non-closable modal.
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.7f),
                             ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::OpenPopup("Round Over");
    int flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
    if (ImGui::BeginPopupModal("Round Over", nullptr, flags)) {
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1.0f), "ROUND OVER");
        ImGui::Separator();
        ImGui::Text("Mode: %s", _game.mode == GameMode::Wild ? "Wild (NURBS)"
                                                              : "Standard");
        ImGui::Text("Targets hit: %d", _game.hits);
        ImGui::Text("Shots fired: %d", _game.shots);
        ImGui::Text("Accuracy:    %.1f%%", _game.accuracy() * 100.0f);

        // Leaderboard for this mode.
        int idx = (_game.mode == GameMode::Wild) ? 1 : 0;
        ImGui::Separator();
        ImGui::Text("Leaderboard (%s):", _game.mode == GameMode::Wild ? "Wild" : "Standard");
        auto& list = _scores[idx];
        // show most-recent first, top 8
        int shown = 0;
        for (auto it = list.rbegin(); it != list.rend() && shown < 8; ++it, ++shown) {
            ImGui::Text("  #%d   %d hits   %d shots   %.1f%%", shown + 1, it->hits,
                        it->shots, it->acc * 100.0f);
        }

        ImGui::Spacing();
        ImGui::Spacing();
        // Buttons.
        float bw = ImGui::GetWindowWidth() * 0.4f;
        if (ImGui::Button("Play Again", ImVec2(bw, 40))) {
            restartRound();
        }
        ImGui::SameLine();
        if (ImGui::Button("Quit", ImVec2(bw, 40))) {
            glfwSetWindowShouldClose(_window, GLFW_TRUE);
        }
        ImGui::EndPopup();
    }
}
