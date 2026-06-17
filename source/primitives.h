#pragma once

#include "vertex.h"

#include <cstdint>
#include <utility>
#include <vector>

// A procedural mesh: deduplicated vertex array + triangle index array,
// matching the contract of Model's vector constructor.
using Mesh = std::pair<std::vector<Vertex>, std::vector<uint32_t>>;

Mesh createSphere(float radius = 0.5f, int segments = 24, int rings = 16);
Mesh createBox(float w = 1.0f, float h = 1.0f, float d = 1.0f);
Mesh createCylinder(float radius = 0.5f, float height = 1.0f, int segments = 24);
Mesh createCone(float radius = 0.5f, float height = 1.0f, int segments = 24);
Mesh createPrismFrustum(float rTop = 0.4f, float rBottom = 0.6f, float height = 1.0f,
                        int nSides = 6);
Mesh createPrism(float radius = 0.5f, float height = 1.0f, int nSides = 6);
