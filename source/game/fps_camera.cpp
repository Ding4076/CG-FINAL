#include "fps_camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

FpsCamera::FpsCamera(PerspectiveCamera* camera, GLFWwindow* window)
    : _camera(camera), _window(window) {
    // Apply the initial orientation immediately so the first rendered frame is
    // already looking the right way (yaw=+90 => front is +Z, toward targets).
    glm::vec3 front = getFront();
    _camera->transform.lookAt(_camera->transform.position + front,
                              glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 FpsCamera::getFront() const {
    glm::vec3 f;
    f.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    f.y = sin(glm::radians(_pitch));
    f.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    return glm::normalize(f);
}

void FpsCamera::lookToward(const glm::vec3& target) {
    glm::vec3 d = glm::normalize(target - _camera->transform.position);
    _pitch = glm::degrees(glm::asin(glm::clamp(d.y, -1.0f, 1.0f)));
    _yaw = glm::degrees(glm::atan(d.z, d.x));
    _pitch = glm::clamp(_pitch, -89.0f, 89.0f);
    // Apply immediately so the first rendered frame is already aimed.
    glm::vec3 front = getFront();
    _camera->transform.lookAt(_camera->transform.position + front,
                              glm::vec3(0.0f, 1.0f, 0.0f));
}

void FpsCamera::resyncCursor(const Input& input) {
    _firstFrame = true;
    (void)input;
}

void FpsCamera::update(const Input& input, float dt, const std::vector<AABB>& obstacles,
                        const std::vector<GroundPatch>& ground, float baseGroundY) {
    // Mouse look from cursor delta.
    float dx = input.mouse.move.xNow - input.mouse.move.xOld;
    float dy = input.mouse.move.yNow - input.mouse.move.yOld;

    if (_firstFrame) {
        _firstFrame = false;
    } else {
        _yaw += dx * _sensitivity;
        _pitch -= dy * _sensitivity;
        _pitch = glm::clamp(_pitch, -89.0f, 89.0f);
    }

    glm::vec3 horizFront = glm::normalize(glm::vec3(
        cos(glm::radians(_yaw)), 0.0f, sin(glm::radians(_yaw))));
    glm::vec3 right = glm::normalize(glm::cross(horizFront, glm::vec3(0.0f, 1.0f, 0.0f)));

    auto held = [this](int key) {
        return glfwGetKey(_window, key) == GLFW_PRESS;
    };

    float speed = _moveSpeed * dt;
    if (held(GLFW_KEY_LEFT_SHIFT)) {
        speed *= 2.0f;
    }
    glm::vec3 delta(0.0f);
    if (held(GLFW_KEY_W)) {
        delta += horizFront * speed;
    }
    if (held(GLFW_KEY_S)) {
        delta -= horizFront * speed;
    }
    if (held(GLFW_KEY_D)) {
        delta += right * speed;
    }
    if (held(GLFW_KEY_A)) {
        delta -= right * speed;
    }

    // Resolve horizontal movement axis-by-axis. Collision uses a sphere whose
    // center sits at collideOffset above the ground (leg/torso height), so
    // waist-high railings block the player even though the eye is at 1.7m. For
    // each axis: (1) reject if the collide-sphere hits a blocker AABB; (2) reject
    // if the ground rises more than maxStep (ramps OK, platform face blocked).
    glm::vec3 pos = _camera->transform.position;
    float curGround = groundHeightAt(pos.x, pos.z, ground, baseGroundY);
    const float maxStep = 0.35f;

    auto collideCenter = [&](const glm::vec3& p, float gY) {
        return glm::vec3(p.x, gY + _collideOffset, p.z);
    };

    auto tryAxis = [&](int axis, float d) {
        if (d == 0.0f) {
            return;
        }
        glm::vec3 trial = pos;
        trial[axis] += d;
        float trialGround = groundHeightAt(trial.x, trial.z, ground, baseGroundY);
        glm::vec3 center = collideCenter(trial, trialGround);
        glm::vec3 closest;
        for (const auto& b : obstacles) {
            if (intersectSphereAABB(center, _radius, b, closest)) {
                return;  // blocked by a wall/railing/crate
            }
        }
        if (trialGround - curGround > maxStep) {
            return;  // too steep a step -> blocked
        }
        pos = trial;
        curGround = trialGround;
    };

    tryAxis(0, delta.x);  // X
    tryAxis(2, delta.z);  // Z

    // Eye height tracks the ground under the player (ramps/platform raise it).
    float groundY = groundHeightAt(pos.x, pos.z, ground, baseGroundY);
    pos.y = groundY + _eyeOffset;
    _camera->transform.position = pos;

    glm::vec3 front = getFront();
    _camera->transform.lookAt(pos + front, glm::vec3(0.0f, 1.0f, 0.0f));
}
