#pragma once

#include <string>

// Per-round game state: mode, timer, score, accuracy. Driven by AimTrainer.
enum class GameMode { Standard, Wild };

class GameState {
public:
    GameMode mode = GameMode::Standard;
    float timeLimit = 60.0f;     // seconds; 0 = no limit
    float timeLeft = 60.0f;
    int hits = 0;
    int shots = 0;
    bool running = false;

    // Tunable params (edited via ImGui).
    float ballSize = 0.45f;       // target ball radius
    float wildSpeed = 1.0f;       // NURBS speed (wild mode)

    void start();
    void tick(float dt);
    void registerShot() { ++shots; }
    void registerHit() { ++hits; }
    float accuracy() const { return shots > 0 ? float(hits) / float(shots) : 0.0f; }
    bool finished() const { return timeLimit > 0.0f && timeLeft <= 0.0f; }
};
