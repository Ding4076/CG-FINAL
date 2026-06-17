#pragma once

#include <glm/glm.hpp>
#include <vector>

// Axis-aligned bounding box (world space). Used for vertical blockers
// (walls, railing, crates) that stop the player horizontally.
struct AABB {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
};

// A flat or sloped walkable patch of ground. Covers an xz rectangle; groundY()
// gives the surface height at (x,z) inside it (constant for a platform, linear
// for a ramp). The player's feet follow these.
struct GroundPatch {
    glm::vec2 min;        // xz min
    glm::vec2 max;        // xz max
    float yLow = 0.0f;    // surface height at the min-z edge
    float yHigh = 0.0f;   // surface height at the max-z edge (ramp); == yLow for flat

    bool contains(float x, float z) const {
        return x >= min.x && x <= max.x && z >= min.y && z <= max.y;
    }
    // Surface height at (x,z); linearly interpolated along z between yLow/yHigh.
    float surfaceY(float x, float z) const {
        (void)x;
        if (max.y == min.y) {
            return yLow;
        }
        float t = (z - min.y) / (max.y - min.y);
        return yLow + t * (yHigh - yLow);
    }
};

// True if the sphere (center c, radius r) overlaps the box b.
bool intersectSphereAABB(const glm::vec3& c, float r, const AABB& b, glm::vec3& outClosest);

// Resolve a proposed horizontal movement (pos -> pos + delta) against a list of
// blocker AABBs, axis-by-axis so the player slides along walls. Only X and Z are
// resolved; Y is set separately from the ground height. Returns corrected XZ/Y
// (Y of the input pos is preserved).
glm::vec3 resolvePlayer(const glm::vec3& pos, float r, const std::vector<AABB>& boxes,
                        const glm::vec3& delta);

// Highest ground-patch surface height under (x,z), or baseY if none.
float groundHeightAt(float x, float z, const std::vector<GroundPatch>& patches, float baseY);

// True if two spheres overlap; writes the unit separation normal (from b to a).
bool intersectSphereSphere(const glm::vec3& a, float ra, const glm::vec3& b, float rb,
                           glm::vec3& outNormal);
