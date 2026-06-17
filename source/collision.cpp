#include "collision.h"

#include <algorithm>

bool intersectSphereAABB(const glm::vec3& c, float r, const AABB& b, glm::vec3& outClosest) {
    // Closest point on the box to the sphere center: clamp each axis.
    outClosest.x = std::clamp(c.x, b.min.x, b.max.x);
    outClosest.y = std::clamp(c.y, b.min.y, b.max.y);
    outClosest.z = std::clamp(c.z, b.min.z, b.max.z);
    glm::vec3 d = c - outClosest;
    return glm::dot(d, d) < r * r;
}

glm::vec3 resolvePlayer(const glm::vec3& pos, float r, const std::vector<AABB>& boxes,
                        const glm::vec3& delta) {
    // Move axis-by-axis (X then Z) so we slide along walls. Y is locked
    // elsewhere (horizontal-only movement), so we only resolve X and Z here.
    glm::vec3 p = pos;

    // X axis.
    glm::vec3 tryX = p + glm::vec3(delta.x, 0.0f, 0.0f);
    glm::vec3 closest;
    for (const auto& b : boxes) {
        if (intersectSphereAABB(tryX, r, b, closest)) {
            tryX = p;  // blocked on X -> revert just X
            break;
        }
    }
    p = tryX;

    // Z axis.
    glm::vec3 tryZ = p + glm::vec3(0.0f, 0.0f, delta.z);
    for (const auto& b : boxes) {
        if (intersectSphereAABB(tryZ, r, b, closest)) {
            tryZ = p;  // blocked on Z -> revert just Z
            break;
        }
    }
    p = tryZ;

    return p;
}

bool intersectSphereSphere(const glm::vec3& a, float ra, const glm::vec3& b, float rb,
                           glm::vec3& outNormal) {
    glm::vec3 d = a - b;
    float dist2 = glm::dot(d, d);
    float rr = ra + rb;
    if (dist2 >= rr * rr || dist2 == 0.0f) {
        return false;
    }
    float dist = std::sqrt(dist2);
    outNormal = d / dist;
    return true;
}

float groundHeightAt(float x, float z, const std::vector<GroundPatch>& patches, float baseY) {
    float best = baseY;
    for (const auto& p : patches) {
        if (p.contains(x, z)) {
            best = std::max(best, p.surfaceY(x, z));
        }
    }
    return best;
}
