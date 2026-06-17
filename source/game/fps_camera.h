#pragma once

#include "camera.h"
#include "collision.h"
#include "input.h"

#include <vector>

// First-person camera: mouse-look (yaw/pitch), WASD movement constrained to the
// horizontal plane (no vertical motion), Shift to sprint. Writes into the
// provided PerspectiveCamera's transform.
class FpsCamera {
public:
    FpsCamera(PerspectiveCamera* camera, GLFWwindow* window);

    // dt in seconds. Reads mouse + WASD from input. The player sphere collides
    // against the given obstacle AABBs (axis-by-axis sliding). The player's feet
    // follow the ground patches (so they can walk up ramps onto a platform);
    // baseGroundY is the floor height where no patch applies.
    void update(const Input& input, float dt, const std::vector<AABB>& obstacles,
                const std::vector<GroundPatch>& ground, float baseGroundY);

    void setPosition(const glm::vec3& p) { _camera->transform.position = p; }
    glm::vec3 getPosition() const { return _camera->transform.position; }
    glm::vec3 getFront() const;

    // Aim the camera (set yaw/pitch) directly at a world-space point.
    void lookToward(const glm::vec3& target);

    void setMoveSpeed(float s) { _moveSpeed = s; }
    void setSensitivity(float s) { _sensitivity = s; }
    void setRadius(float r) { _radius = r; }

    // Sync xOld/xNow to the current cursor so the first update after this call
    // doesn't produce a huge mouse delta (avoids the first-frame snap).
    void resyncCursor(const Input& input);

private:
    PerspectiveCamera* _camera;
    GLFWwindow* _window;
    float _yaw = 90.0f;    // +90 => default front is +Z (facing the target range)
    float _pitch = 0.0f;
    float _moveSpeed = 4.0f;
    float _sensitivity = 0.12f;
    bool _firstFrame = true;
    float _eyeOffset = 1.7f;        // eye height above the ground under the player
    float _radius = 0.45f;          // player collision sphere radius
    float _collideOffset = 0.9f;    // collision-sphere center height above ground
                                    // (legs/torso height, so low railings block)
};

