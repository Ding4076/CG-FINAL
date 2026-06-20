#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uDiffuse;
uniform sampler2D uSpecular;
uniform vec3 uViewPos;
uniform vec3 uLightDir;   // direction TO the light
uniform vec3 uLightColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 L = normalize(uLightDir);
    vec3 H = normalize(L + V);

    vec3 diff = texture(uDiffuse,  vTexCoord).rgb;
    vec3 spec = texture(uSpecular, vTexCoord).rgb;

    vec3 ambient  = 0.25 * diff;
    vec3 diffuse  = max(dot(N, L), 0.0) * diff * uLightColor;
    vec3 specular = pow(max(dot(N, H), 0.0), 64.0) * spec * uLightColor;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
