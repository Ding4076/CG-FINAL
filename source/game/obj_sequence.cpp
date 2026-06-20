#include "obj_sequence.h"

#include "obj_loader.h"
#include "primitives.h"
#include "vertex.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <vector>

namespace {
// Generate one frame of the deforming sphere: two-layer ripple + global pulse,
// giving a more organic, lively deformation than a single ripple.
Mesh makeDeformingSphere(float phase) {
    const int segments = 32;
    const int rings    = 22;
    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;
    verts.reserve((segments + 1) * (rings + 1));

    const float PI = 3.14159265358979323846f;
    for (int r = 0; r <= rings; ++r) {
        float theta = PI * r / rings;
        float st = std::sin(theta), ct = std::cos(theta);
        for (int s = 0; s <= segments; ++s) {
            float phi = 2.0f * PI * s / segments;
            float sp = std::sin(phi), cp = std::cos(phi);
            float p = phase * 2.0f * PI;
            // Global pulse
            float pulse   = 0.18f * std::sin(p);
            // Two ripple layers travelling in opposite directions
            float ripple1 = 0.12f * std::sin(4.0f * phi + p * 1.5f) * st;
            float ripple2 = 0.08f * std::sin(6.0f * theta + p * 2.3f);
            float radius  = 1.0f + pulse + ripple1 + ripple2;
            glm::vec3 n(cp * st, ct, sp * st);
            verts.push_back(Vertex{n * radius, n,
                {s / (float)segments, r / (float)rings}});
        }
    }
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < segments; ++s) {
            uint32_t a = r * (segments + 1) + s;
            uint32_t b = a + segments + 1;
            idx.push_back(a);   idx.push_back(b);     idx.push_back(a + 1);
            idx.push_back(a + 1); idx.push_back(b); idx.push_back(b + 1);
        }
    }
    return {verts, idx};
}

std::string framePath(const std::string& dir, int i) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "frame_%03d.obj", i);
    return (std::filesystem::path(dir) / buf).string();
}

// Split the deforming-sphere mesh into parallel pos/norm/uv arrays (the format
// saveObj expects) so the written .obj has matching v/vt/vn indexed 1:1.
void flattenForObj(const Mesh& mesh, std::vector<glm::vec3>& P,
                   std::vector<glm::vec3>& N, std::vector<glm::vec2>& T,
                   std::vector<uint32_t>& I) {
    const auto& v = mesh.first;
    P.resize(v.size());
    N.resize(v.size());
    T.resize(v.size());
    for (size_t i = 0; i < v.size(); ++i) {
        P[i] = v[i].position;
        N[i] = v[i].normal;
        T[i] = v[i].texCoord;
    }
    I = mesh.second;
}
}  // namespace

bool ObjSequence::loadOrGenerate(const std::string& dir, int count, int fps) {
    namespace fs = std::filesystem;
    _frames.clear();
    _index = 0;
    _accum = 0.0f;
    _done = false;
    _frameTime = fps > 0 ? 1.0f / static_cast<float>(fps) : (1.0f / 12.0f);
    if (count <= 0) {
        return false;
    }

    try {
        fs::create_directories(dir);

        // Generate the frame files on first run (self-contained demo) so the
        // loader always has real .obj files to read.
        if (!fs::exists(framePath(dir, 0))) {
            for (int i = 0; i < count; ++i) {
                float phase = static_cast<float>(i) / static_cast<float>(count);
                Mesh mesh = makeDeformingSphere(phase);
                std::vector<glm::vec3> P, N;
                std::vector<glm::vec2> T;
                std::vector<uint32_t> I;
                flattenForObj(mesh, P, N, T, I);
                saveObj(framePath(dir, i), P, N, T, I);
            }
        }

        // Read every frame back from disk -- this is the multi-frame OBJ load
        // the spec requires (each frame re-parses an .obj into a fresh Model).
        _frames.reserve(count);
        for (int i = 0; i < count; ++i) {
            _frames.push_back(std::make_unique<Model>(framePath(dir, i)));
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "ObjSequence loadOrGenerate FAILED: %s\n", e.what());
        return false;
    }
    return !_frames.empty();
}

void ObjSequence::update(float dt) {
    if (_frames.empty()) {
        return;
    }
    _accum += dt * _speed;
    while (_accum >= _frameTime) {
        _accum -= _frameTime;
        ++_index;
        if (_index >= static_cast<int>(_frames.size())) {
            if (_looping) {
                _index = 0;
            } else {
                _index = static_cast<int>(_frames.size()) - 1;
                _done = true;
                break;
            }
        }
    }
}

void ObjSequence::rewind() {
    _index = 0;
    _accum = 0.0f;
    _done = false;
}

Model* ObjSequence::currentModel() const {
    if (_frames.empty()) {
        return nullptr;
    }
    return _frames[_index].get();
}
