#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

// Raw OBJ geometry (no materials). Faces are polygons (n-gons) kept as-is;
// triangulation happens in Model when building the Vertex/index buffers.
struct RawObj {
    std::vector<glm::vec3> positions; // 'v'
    std::vector<glm::vec2> texcoords; // 'vt'
    std::vector<glm::vec3> normals;   // 'vn'

    // Each face corner references pos/uv/normal by 1-based index (as in .obj);
    // -1 means that attribute is absent for this corner.
    struct Corner {
        int p = -1;
        int t = -1;
        int n = -1;
    };
    std::vector<std::vector<Corner>> faces; // one polygon per 'f' line
};

// Parse a .obj from an in-memory string.
RawObj parseObjString(const std::string& text);

// Parse a .obj file from disk. Throws std::runtime_error if it cannot be opened.
RawObj loadObj(const std::string& filepath);

// Serialize a mesh (parallel position/normal/texcoord arrays + triangle indices)
// to .obj text. Indices are triangles (length % 3 == 0). Assumes the three
// arrays are index-aligned (i.e. index k refers to positions[k], normals[k],
// texcoords[k] together) -- this matches the Model's deduplicated layout.
std::string writeObj(const std::vector<glm::vec3>& positions,
                     const std::vector<glm::vec3>& normals,
                     const std::vector<glm::vec2>& texcoords,
                     const std::vector<uint32_t>& indices);

// Convenience: write a mesh to a .obj file on disk.
void saveObj(const std::string& filepath,
             const std::vector<glm::vec3>& positions,
             const std::vector<glm::vec3>& normals,
             const std::vector<glm::vec2>& texcoords,
             const std::vector<uint32_t>& indices);
