#version 330 core
in vec3 vColor;
out vec4 FragColor;

uniform float uAlpha;   // global fade multiplier for the whole burst

void main() {
    // Soft circular point: distance from the point's center (gl_PointCoord).
    vec2 c = gl_PointCoord - vec2(0.5);
    float d = dot(c, c);          // 0 at center, 0.25 at edge
    float a = smoothstep(0.25, 0.0, d) * uAlpha;
    if (a <= 0.0) {
        discard;
    }
    FragColor = vec4(vColor, a);
}
