#version 330 core
// Draws solid black everywhere EXCEPT inside the scope circle (those pixels are
// discarded so the scene shows through).
in vec2 vScreen;
out vec4 FragColor;
uniform vec2  uCenter;   // circle center in [0,1] screen coords
uniform float uRadius;   // circle radius in [0,1] of the SHORTER axis
uniform float uAspect;   // framebuffer width / height
void main() {
    // Work in pixel-space aspect-corrected coords so the mask is a true circle.
    // Scale x by aspect (assume uRadius is relative to the shorter axis = height).
    vec2 d = vScreen - uCenter;
    d.x *= uAspect;          // undo the horizontal stretch
    float dist = dot(d, d);
    if (dist < uRadius * uRadius) {
        discard;   // inside the scope: keep the rendered scene
    }
    FragColor = vec4(0.0, 0.0, 0.0, 1.0);   // outside: black
}
