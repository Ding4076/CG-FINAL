#version 330 core
// Particle point sprite. Each particle is a GL_POINT with a per-vertex color;
// the fragment shader draws a soft disc.
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 uView;
uniform mat4 uProj;

out vec3 vColor;

void main() {
    vColor = aColor;
    vec4 viewPos = uView * vec4(aPos, 1.0);
    gl_Position = uProj * viewPos;
    // Size shrinks with distance for a basic perspective effect.
    gl_PointSize = clamp(300.0 / -viewPos.z, 2.0, 40.0);
}
