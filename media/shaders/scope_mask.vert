#version 330 core
// Fullscreen triangle/quad: gl_Position directly in clip space.
// Expects a fullscreen quad with positions in [-1,1]; we also derive the
// screen pixel coordinate from the position.
layout (location = 0) in vec2 aPos;
out vec2 vScreen;   // pixel coords [0,1]x[0,1]
uniform vec2 uScreen;  // full framebuffer size in pixels
void main() {
    vScreen = aPos * 0.5 + 0.5;   // [-1,1] -> [0,1]
    gl_Position = vec4(aPos, 0.0, 1.0);
}
