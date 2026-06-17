#include "orbit_camera.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

OrbitCamera::OrbitCamera(PerspectiveCamera* camera)
    : _camera(camera) {
    // Do NOT applyTransform() here: it would overwrite the FPS spawn position.
    // The orbit view is applied on demand (update / setDefaultFraming).
}

void OrbitCamera::applyTransform() {
    // Eye position on a sphere around the target.
    float cp = cos(glm::radians(_pitch));
    glm::vec3 offset(
        _radius * cp * cos(glm::radians(_yaw)),
        _radius * sin(glm::radians(_pitch)),
        _radius * cp * sin(glm::radians(_yaw)));
    glm::vec3 eye = _target + offset;
    _camera->transform.position = eye;
    _camera->transform.lookAt(_target, glm::vec3(0.0f, 1.0f, 0.0f));
}

void OrbitCamera::update(const Input& input, float dt) {
    (void)dt;

    // Rotate: right button drag.
    if (input.mouse.press.right) {
        float dx = input.mouse.move.xNow - input.mouse.move.xOld;
        float dy = input.mouse.move.yNow - input.mouse.move.yOld;
        _yaw += dx * _rotateSpeed;
        _pitch += dy * _rotateSpeed;
        _pitch = glm::clamp(_pitch, -89.0f, 89.0f);
    }

    // Zoom: mouse wheel.
    if (input.mouse.scroll.yOffset != 0.0f) {
        _radius *= (1.0f - input.mouse.scroll.yOffset * 0.1f);
        _radius = std::max(_radius, 0.5f);
    }

    // Pan: middle button drag (move both target and eye in the screen plane).
    if (input.mouse.press.middle) {
        float dx = input.mouse.move.xNow - input.mouse.move.xOld;
        float dy = input.mouse.move.yNow - input.mouse.move.yOld;
        glm::vec3 front = glm::normalize(_target - _camera->transform.position);
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up = glm::normalize(glm::cross(right, front));
        glm::vec3 pan = (-right * dx + up * dy) * _panSpeed * _radius;
        _target += pan;
    }

    applyTransform();
}

void OrbitCamera::setDefaultFraming(const glm::vec3& target, float radius, float yaw,
                                    float pitch) {
    _homeTarget = target;
    _homeRadius = radius;
    _homeYaw = yaw;
    _homePitch = pitch;
    // Apply it immediately so the orbit starts at the home view.
    _target = target;
    _radius = radius;
    _yaw = yaw;
    _pitch = pitch;
    applyTransform();
}

void OrbitCamera::zoomToFit(const BoundingBox& box) {
    // Just return to the home framing (the view shown right after switching to
    // Orbit). Computing from the bbox was unreliable; a fixed known-good view
    // is simpler and always shows the scene.
    (void)box;
    _target = _homeTarget;
    _radius = _homeRadius;
    _yaw = _homeYaw;
    _pitch = _homePitch;
    applyTransform();
}
