#include "primitives.h"

#include <cmath>

Mesh createSphere(float radius, int segments, int rings) {
    std::vector<Vertex> v;
    std::vector<uint32_t> i;
    for (int r = 0; r <= rings; ++r) {
        float theta = static_cast<float>(M_PI) * r / rings;
        float st = std::sin(theta), ct = std::cos(theta);
        for (int s = 0; s <= segments; ++s) {
            float phi = 2.0f * static_cast<float>(M_PI) * s / segments;
            float sp = std::sin(phi), cp = std::cos(phi);
            glm::vec3 n(cp * st, ct, sp * st);
            v.push_back(Vertex(n * radius, n, glm::vec2(s / static_cast<float>(segments),
                                                        r / static_cast<float>(rings))));
        }
    }
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < segments; ++s) {
            uint32_t a = static_cast<uint32_t>(r * (segments + 1) + s);
            uint32_t b = a + segments + 1;
            i.push_back(a);
            i.push_back(b);
            i.push_back(a + 1);
            i.push_back(a + 1);
            i.push_back(b);
            i.push_back(b + 1);
        }
    }
    return {v, i};
}

namespace {
void addRing(std::vector<glm::vec3>& ring, float r, float y, int nSides) {
    for (int k = 0; k < nSides; ++k) {
        float a = 2.0f * static_cast<float>(M_PI) * k / nSides;
        ring.emplace_back(r * std::cos(a), y, r * std::sin(a));
    }
}
} // namespace

Mesh createPrismFrustum(float rTop, float rBottom, float height, int nSides) {
    std::vector<Vertex> v;
    std::vector<uint32_t> i;

    std::vector<glm::vec3> bot, top;
    addRing(bot, rBottom, -height / 2, nSides);
    addRing(top, rTop, height / 2, nSides);

    // Side faces (one quad -> two tris per side), outward horizontal normals.
    for (int k = 0; k < nSides; ++k) {
        int kn = (k + 1) % nSides;
        glm::vec3 along = bot[kn] - bot[k];
        glm::vec3 edge = top[k] - bot[k];
        glm::vec3 n = glm::normalize(glm::cross(edge, along));
        uint32_t base = static_cast<uint32_t>(v.size());
        v.push_back({bot[k], n, {0, 0}});
        v.push_back({bot[kn], n, {1, 0}});
        v.push_back({top[kn], n, {1, 1}});
        v.push_back({top[k], n, {0, 1}});
        i.push_back(base);
        i.push_back(base + 1);
        i.push_back(base + 2);
        i.push_back(base);
        i.push_back(base + 2);
        i.push_back(base + 3);
    }

    // Bottom cap (normal -Y).
    uint32_t cBot = static_cast<uint32_t>(v.size());
    v.push_back({{0, -height / 2, 0}, {0, -1, 0}, {0, 0}});
    for (int k = 0; k < nSides; ++k) {
        int kn = (k + 1) % nSides;
        v.push_back({bot[k], {0, -1, 0}, {0, 0}});
        v.push_back({bot[kn], {0, -1, 0}, {0, 0}});
    }
    for (int k = 0; k < nSides; ++k) {
        uint32_t b = cBot + 1 + static_cast<uint32_t>(k) * 2;
        i.push_back(cBot);
        i.push_back(b);
        i.push_back(b + 1);
    }

    // Top cap (normal +Y).
    uint32_t cTop = static_cast<uint32_t>(v.size());
    v.push_back({{0, height / 2, 0}, {0, 1, 0}, {0, 0}});
    for (int k = 0; k < nSides; ++k) {
        int kn = (k + 1) % nSides;
        v.push_back({top[k], {0, 1, 0}, {0, 0}});
        v.push_back({top[kn], {0, 1, 0}, {0, 0}});
    }
    for (int k = 0; k < nSides; ++k) {
        uint32_t b = cTop + 1 + static_cast<uint32_t>(k) * 2;
        i.push_back(cTop);
        i.push_back(b + 1);
        i.push_back(b);
    }

    return {v, i};
}

Mesh createPrism(float radius, float height, int nSides) {
    return createPrismFrustum(radius, radius, height, nSides);
}

Mesh createBox(float w, float h, float d) {
    std::vector<Vertex> v;
    std::vector<uint32_t> i;
    float x = w / 2, y = h / 2, z = d / 2;
    struct Face {
        glm::vec3 n;
        glm::vec3 p[4];
    };
    // 6 faces, 4 distinct verts each (flat normals, correct for lighting).
    Face faces[6] = {
        {{0, 0, 1}, {{-x, -y, z}, {x, -y, z}, {x, y, z}, {-x, y, z}}},
        {{0, 0, -1}, {{x, -y, -z}, {-x, -y, -z}, {-x, y, -z}, {x, y, -z}}},
        {{0, 1, 0}, {{-x, y, z}, {x, y, z}, {x, y, -z}, {-x, y, -z}}},
        {{0, -1, 0}, {{-x, -y, -z}, {x, -y, -z}, {x, -y, z}, {-x, -y, z}}},
        {{1, 0, 0}, {{x, -y, z}, {x, -y, -z}, {x, y, -z}, {x, y, z}}},
        {{-1, 0, 0}, {{-x, -y, -z}, {-x, -y, z}, {-x, y, z}, {-x, y, -z}}},
    };
    for (const auto& f : faces) {
        uint32_t base = static_cast<uint32_t>(v.size());
        v.push_back({f.p[0], f.n, {0, 0}});
        v.push_back({f.p[1], f.n, {1, 0}});
        v.push_back({f.p[2], f.n, {1, 1}});
        v.push_back({f.p[3], f.n, {0, 1}});
        i.push_back(base);
        i.push_back(base + 1);
        i.push_back(base + 2);
        i.push_back(base);
        i.push_back(base + 2);
        i.push_back(base + 3);
    }
    return {v, i};
}

Mesh createCylinder(float radius, float height, int segments) {
    return createPrismFrustum(radius, radius, height, segments);
}

Mesh createCone(float radius, float height, int segments) {
    return createPrismFrustum(0.0001f, radius, height, segments);
}
