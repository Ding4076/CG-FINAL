#version 330 core
// Particle point sprite with per-particle size (location 2).
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in float aSize;

uniform mat4 uView;
uniform mat4 uProj;

out vec3 vColor;

void main() {
    vColor = aColor;
    vec4 viewPos = uView * vec4(aPos, 1.0);
    gl_Position = uProj * viewPos;
    // Per-particle point radius with distance falloff — large particles
    // read big and slow, small ones tight and fast, layered depth.
    gl_PointSize = aSize;
}
