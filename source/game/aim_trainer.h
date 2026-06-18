#pragma once

#include "application.h"
#include "audio.h"
#include "camera.h"
#include "collision.h"
#include "framebuffer.h"
#include "fullscreen_quad.h"
#include "game_state.h"
#include "glsl_program.h"
#include "model.h"
#include "nurbs.h"
#include "obj_sequence.h"
#include "particle_system.h"
#include "texture2d.h"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

struct ImFont;   // forward decl (avoid pulling imgui.h into the header)

struct BlinnPhongMaterial {
    glm::vec3 kd{0.8f, 0.8f, 0.8f};
    glm::vec3 ks{0.4f, 0.4f, 0.4f};
    float shininess = 32.0f;
};

// A target ball: either static (Standard) or NURBS-driven (Wild).
struct Target {
    glm::vec3 position{};
    float radius = 0.45f;
    bool alive = true;
    glm::vec3 color{1.0f, 0.3f, 0.3f};
    NurbsCurve* path = nullptr;   // wild mode: drives position
    float param = 0.0f;           // wild mode: NURBS parameter
};

class FpsCamera;
class OrbitCamera;

class AimTrainer : public Application {
public:
    AimTrainer(const Options& options);
    ~AimTrainer() override;

protected:
    void handleInput() override;
    void renderFrame() override;

private:
    struct Primitive {
        std::unique_ptr<Model> model;
        glm::vec3 position{};
        BlinnPhongMaterial material;
    };
    std::vector<Primitive> _primitives;
    std::unique_ptr<Model> _groundPlane;   // flat floor, receives shadows

    // NURBS-driven target (bonus: NURBS curve trajectory).
    std::unique_ptr<Model> _nurbsBall;
    NurbsCurve _nurbsCurve;
    std::vector<glm::vec3> _nurbsControlPoints;

    // --- Gameplay (Task 8) ---
    std::unique_ptr<Model> _targetModel;   // shared sphere mesh, scaled per-target
    std::vector<Target> _targets;          // alive targets (1 in Standard, N in Wild)
    std::vector<NurbsCurve> _wildPaths;    // one NURBS path per wild target
    GameState _game;
    ParticleSystem _particles;
    Audio _audio;
    bool _audioOk = false;

    // Leaderboard: best scores (hits) per mode, kept in memory this session.
    struct Score {
        int hits;
        int shots;
        float acc;
        GameMode mode;
    };
    std::vector<Score> _scores[2];   // [Standard, Wild], newest last
    bool _showRoundOver = false;     // big modal shown when a round ends

    // Weapon: Pistol (fast, no scope) / Sniper (slow, right-click scope).
    enum class Weapon { Pistol, Sniper } _weapon = Weapon::Pistol;
    float _fireCooldown = 0.0f;            // seconds until next shot allowed
    bool _scoped = false;                  // sniper scope active (right mouse)

    bool _leftMousePrev = false;           // edge detection for firing
    bool _paused = false;                  // F1 panel open -> game frozen, cursor free
    float _mouseSensitivity = 0.12f;       // FPS mouse-look sensitivity (F1 slider)
    bool _fullscreen = false;              // exclusive-fullscreen toggle (F1 panel)
    ImFont* _bigFont = nullptr;            // large bold font for the HUD timer
    int _windowedX = 0, _windowedY = 0;    // last windowed pos/size (to restore)
    int _windowedW = 1280, _windowedH = 720;

    // Opening cutscene (base req 7): a deforming-sphere OBJ sequence plays once
    // on launch, then hands off to the game. Skippable (any key/click) or auto.
    bool _intro = true;
    ObjSequence _introSeq;
    float _introTime = 0.0f;

    bool _screenshotQueued = false;        // F2 pressed -> grab the frame at end of render
    std::string _lastScreenshot;           // last saved PNG path (shown in the UI)
    std::string _lastExport;               // last exported .obj path (shown in the UI)


    // Arena obstacles for collision (bonus: real-time collision). Each obstacle
    // has an AABB for physics and a Model for rendering.
    struct Obstacle {
        std::unique_ptr<Model> model;
        AABB box;
    };
    std::vector<Obstacle> _obstacles;

    // Walkable ground patches (floor + ramps + platform top). The player's feet
    // follow these so they can walk up the ramps onto the rear platform.
    std::vector<GroundPatch> _ground;
    float _baseGroundY = 0.0f;
    float _platformHeight = 1.6f;   // height of the rear platform top
    // Render-only models for the platform + ramps (not collision blockers; the
    // platform face is blocked by the ground-height step check instead).
    std::unique_ptr<Model> _platform;
    std::unique_ptr<Model> _rampLeft;
    std::unique_ptr<Model> _rampRight;

    std::unique_ptr<GLSLProgram> _bpShader;
    std::unique_ptr<GLSLProgram> _depthShader;
    std::unique_ptr<GLSLProgram> _scopeShader;   // black outside the scope circle
    std::unique_ptr<FullscreenQuad> _fullscreenQuad;

    // Shadow map resources.
    std::unique_ptr<Texture2D> _shadowTex;
    std::unique_ptr<Framebuffer> _shadowFbo;
    static constexpr int _shadowResolution = 2048;
    glm::mat4 _lightSpaceMatrix{1.0f};
    bool _shadowsOn = true;

    // Spotlight shadow resources (single spot, perspective depth map).
    std::unique_ptr<Texture2D> _spotShadowTex;
    std::unique_ptr<Framebuffer> _spotShadowFbo;
    glm::mat4 _spotSpaceMatrix{1.0f};
    bool _spotShadowsOn = true;

    // --- Camera (FPS + Orbit, Tab to switch) ---
    PerspectiveCamera _camera;
    std::unique_ptr<FpsCamera> _fps;
    std::unique_ptr<OrbitCamera> _orbit;
    enum class CamMode { FPS, Orbit } _camMode = CamMode::FPS;
    glm::vec3 _fpsSpawn{0.0f, 1.7f, 1.0f};   // restored when switching back to FPS

    // --- Lighting state (edited via ImGui) ---
    glm::vec3 _skyColor{0.6f, 0.7f, 0.9f};
    glm::vec3 _groundColor{0.2f, 0.18f, 0.15f};

    struct DirLight {
        glm::vec3 dir{0.5f, 1.0f, 0.8f};
        glm::vec3 color{1.0f};
        float intensity = 1.0f;
        bool on = true;
    } _dir;

    struct PtLight {
        glm::vec3 position{0.0f, 2.0f, 0.0f};
        glm::vec3 color{1.0f};
        float intensity = 1.0f;
        float kc = 1.0f, kl = 0.7f, kq = 1.8f;
        bool on = true;
    };
    std::vector<PtLight> _points;

    struct SpLight {
        glm::vec3 position{0.75f, 4.0f, 0.0f};      // above the middle of the row
        glm::vec3 direction{0.0f, -1.0f, 0.0f};     // straight down
        glm::vec3 color{1.0f};
        float intensity = 1.0f;
        float cosAngle = 0.5f;   // ~60 deg half-angle (wide cone)
        float kc = 1.0f, kl = 0.0f, kq = 0.0f;
        bool on = true;
    };
    std::vector<SpLight> _spots;

    // Player flashlight: a spotlight attached to the camera that shines outward
    // along the view direction (bonus: real-time, editable spotlight). 'L' toggles
    // it; lives in shader slot uSpots[1] (the arena spot keeps slot 0).
    struct FlashLight {
        bool on = false;
        glm::vec3 color{1.0f, 0.95f, 0.82f};
        float intensity = 1.8f;
        float cosAngle = 0.86f;   // ~30 deg half-angle
    } _flash;

    float _angle = 0.0f;
    bool _showUi = false;    // F1 opens the pause/control panel

    void initPrimitives();
    void initArena();
    void initShadowMap();
    void setLightUniforms();
    void drawImGui();
    BoundingBox sceneBounds() const;
    void switchMode(CamMode mode);
    void updateLightSpaceMatrix();
    void updateSpotSpaceMatrix();
    void renderDepthPass();
    void renderSpotDepthPass();
    void drawScene(GLSLProgram& shader, bool withMaterial);
    void drawTargets(GLSLProgram& shader);
    void drawParticles();
    std::vector<AABB> obstacleBoxes() const;
    std::vector<GroundPatch> groundPatches() const { return _ground; }
    float baseGroundY() const { return _baseGroundY; }

    // --- Gameplay helpers ---
    void resetRound();
    void spawnTarget(Target& t);        // place a target (in view, in target range)
    void updateTargets(float dt);
    void tryFire();                     // ray-sphere hit test on left click
    void updateWeapon(float dt);
    glm::vec3 aimRayDir() const;        // current view direction (scope-aware)
    void drawGameHud();                 // crosshair, scope vignette, score
    void checkRoundEnd();               // record score + show modal when round ends
    void drawRoundOverModal();          // big centered result + buttons
    void restartRound();                // resetRound + re-aim at the center target
    void toggleFullscreen();            // switch exclusive fullscreen <-> windowed

    // --- Intro / capture / export (Task 9 + Task 10) ---
    void renderIntro();                 // deforming-sphere OBJ-sequence cutscene
    void drawIntroOverlay();            // title + "press any key" text
    void captureScreenshot();           // F2: glReadPixels -> PNG via stb_image_write
    void exportSceneObj();              // merge scene meshes -> one .obj (base req 2)
};
