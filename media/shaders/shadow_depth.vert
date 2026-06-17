#version 330 core
// Depth-only pass: write clip-space depth from the light's point of view.
layout (location = 0) in vec3 aPos;

uniform mat4 uLightSpace;   // projection * view of the directional light
uniform mat4 uModel;

void main() {
    gl_Position = uLightSpace * uModel * vec4(aPos, 1.0);
}
