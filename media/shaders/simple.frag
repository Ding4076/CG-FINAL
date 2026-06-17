#version 330 core
in vec3 vNormal;
in vec3 vFragPos;
out vec4 FragColor;

uniform vec3 uLightDir;   // direction TO the light

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    float diff = max(dot(N, L), 0.0);
    vec3 base = vec3(0.8, 0.85, 0.9);
    vec3 ambient = 0.2 * base;
    vec3 diffuse = diff * base;
    FragColor = vec4(ambient + diffuse, 1.0);
}
