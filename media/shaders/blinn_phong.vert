#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vTexCoord;
out vec4 vLightSpacePos;   // position in the directional light's clip space
out vec4 vSpotSpacePos;    // position in the spot light's clip space

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uLightSpace;
uniform mat4 uSpotSpace;

void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vTexCoord = aTexCoord;
    vLightSpacePos = uLightSpace * world;
    vSpotSpacePos = uSpotSpace * world;
    gl_Position = uProj * uView * world;
}
