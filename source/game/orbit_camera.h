#pragma once

#include "bounding_box.h"
#include "camera.h"
#include "input.h"

// Orbit inspector camera (like pbr_viewer's CameraController):
//   right-drag  -> rotate (spherical orbit around target)
//   wheel       -> zoom (dolly toward/away from target)
//   middle-drag -> pan (move target in the screen plane)
//   zoomToFit() -> frame a scene bounding sphere.
class OrbitCamera {
public:
    OrbitCamera(PerspectiveCamera* camera);

    void update(const Input& input, float dt);

    // Reset to the default framing (the view shown right after switching to
    // Orbit). The bbox argument is ignored by default; the framing is fixed by
    // the defaults passed to setDefaultFraming().
    void zoomToFit(const BoundingBox& box);

    // Configure the "home" framing used by zoomToFit. Call once after building
    // the scene so reset returns to a known-good view.
    void setDefaultFraming(const glm::vec3& target, float radius, float yaw,
                           float pitch);

    glm::vec3 getTarget() const { return _target; }
    void setTarget(const glm::vec3& t) { _target = t; }

private:
    PerspectiveCamera* _camera;
    glm::vec3 _target{0.0f};
    float _radius = 9.0f;       // distance eye<->target
    float _yaw = 90.0f;         // degrees, around Y (eye at +Z looking -Z)
    float _pitch = 15.0f;       // degrees, above horizon
    float _rotateSpeed = 0.25f;
    float _panSpeed = 0.01f;

    // Home framing used by zoomToFit.
    glm::vec3 _homeTarget{0.75f, 0.8f, 0.0f};
    float _homeRadius = 9.0f;
    float _homeYaw = 90.0f;
    float _homePitch = 15.0f;

    void applyTransform();
};
