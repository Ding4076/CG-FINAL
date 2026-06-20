#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main() {
    vec4 world = uModel * vec4(aPosition, 1.0);
    vWorldPos  = world.xyz;
    vNormal    = mat3(transpose(inverse(uModel))) * aNormal;
    vTexCoord  = aTexCoord;
    gl_Position = uProj * uView * world;
}
