#include "game_state.h"

void GameState::start() {
    timeLeft = timeLimit;
    hits = 0;
    shots = 0;
    running = true;
}

void GameState::tick(float dt) {
    if (!running || timeLimit <= 0.0f) {
        return;
    }
    timeLeft -= dt;
    if (timeLeft <= 0.0f) {
        timeLeft = 0.0f;
        running = false;
    }
}
