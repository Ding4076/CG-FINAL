#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vLightSpacePos;
in vec4 vSpotSpacePos;
out vec4 FragColor;

// --- Material ---
uniform vec3  uKd;
uniform vec3  uKs;
uniform float uShininess;
uniform sampler2D uTexture;
uniform bool  uHasTexture;

// --- Camera ---
uniform vec3 uViewPos;

// --- Hemisphere ambient (sky/ground) ---
uniform vec3 uSkyColor;
uniform vec3 uGroundColor;

// --- Directional light ---
uniform vec3  uDirLightDir;     // direction TO the light
uniform vec3  uDirLightColor;
uniform float uDirLightIntensity;
uniform bool  uDirLightOn;

// --- Shadows (directional) ---
uniform sampler2D uShadowMap;
uniform bool      uShadowsOn;

// --- Shadows (spot light) ---
uniform sampler2D uSpotShadowMap;
uniform bool      uSpotShadowsOn;

// --- Point lights (up to 4) ---
struct PointLight {
    vec3  position;
    vec3  color;
    float intensity;
    float kc, kl, kq;          // attenuation: 1/(kc + kl*d + kq*d^2)
};
uniform int uPointCount;        // number actually enabled
uniform PointLight uPoints[4];

// --- Spot lights (up to 2) ---
struct SpotLight {
    vec3  position;
    vec3  direction;           // direction the spot points (from position outward)
    vec3  color;
    float intensity;
    float cosAngle;            // cosine of half the cone angle
    float kc, kl, kq;
};
uniform int uSpotCount;
uniform SpotLight uSpots[2];

// Blinn-Phong contribution from a light direction L (no attenuation).
vec3 blinnPhong(vec3 N, vec3 V, vec3 L, vec3 lightColor, float intensity,
                vec3 albedo) {
    vec3 H = normalize(L + V);
    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), uShininess);
    return intensity * lightColor * (diff * albedo + spec * uKs);
}

// PCF 3x3 shadow factor for the directional light. Returns 0 = full shadow,
// 1 = fully lit.
float shadowFactor() {
    // Perspective divide + remap to [0,1] texture coords.
    vec3 proj = vLightSpacePos.xyz / vLightSpacePos.w;
    proj = proj * 0.5 + 0.5;
    // Outside the shadow frustum -> not in shadow.
    if (proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0
        || proj.z > 1.0) {
        return 1.0;
    }
    float current = proj.z;
    float bias = 0.004;
    // 3x3 percentage-closer filtering for soft edges.
    vec2 texel = 1.0 / vec2(textureSize(uShadowMap, 0));
    float sum = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float closest = texture(uShadowMap, proj.xy + vec2(x, y) * texel).r;
            sum += (current - bias > closest) ? 0.0 : 1.0;
        }
    }
    return sum / 9.0;
}

// PCF shadow factor for the spot light (perspective depth map). Same idea as
// the directional one but from the spot's clip space (vSpotSpacePos).
float spotShadowFactor() {
    vec3 proj = vSpotSpacePos.xyz / vSpotSpacePos.w;
    proj = proj * 0.5 + 0.5;
    if (proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0
        || proj.z > 1.0) {
        return 1.0;
    }
    float current = proj.z;
    float bias = 0.002;
    vec2 texel = 1.0 / vec2(textureSize(uSpotShadowMap, 0));
    float sum = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float closest = texture(uSpotShadowMap, proj.xy + vec2(x, y) * texel).r;
            sum += (current - bias > closest) ? 0.0 : 1.0;
        }
    }
    return sum / 9.0;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 albedo = uHasTexture ? texture(uTexture, vTexCoord).rgb : uKd;

    // Hemisphere ambient: blend sky/ground by normal's up-ness.
    float upMix = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 ambient = 0.2 * mix(uGroundColor, uSkyColor, upMix) * albedo;

    vec3 result = ambient;

    // Directional (with shadow).
    if (uDirLightOn) {
        float shadow = (uShadowsOn) ? shadowFactor() : 1.0;
        result += shadow * blinnPhong(N, V, normalize(uDirLightDir), uDirLightColor,
                                      uDirLightIntensity, albedo);
    }

    // Point lights (with attenuation).
    for (int i = 0; i < uPointCount; ++i) {
        vec3 toLight = uPoints[i].position - vWorldPos;
        float dist = length(toLight);
        vec3 L = toLight / max(dist, 1e-4);
        float atten = 1.0 / (uPoints[i].kc + uPoints[i].kl * dist
                             + uPoints[i].kq * dist * dist);
        result += atten * blinnPhong(N, V, L, uPoints[i].color,
                                     uPoints[i].intensity, albedo);
    }

    // Spot lights (cone + attenuation). Spot 0 also casts a shadow.
    for (int i = 0; i < uSpotCount; ++i) {
        vec3 toLight = uSpots[i].position - vWorldPos;
        float dist = length(toLight);
        vec3 L = toLight / max(dist, 1e-4);
        float theta = dot(L, normalize(-uSpots[i].direction));
        // smooth cone edge
        float spot = clamp((theta - uSpots[i].cosAngle) / 0.05, 0.0, 1.0);
        if (spot > 0.0) {
            float atten = 1.0 / (uSpots[i].kc + uSpots[i].kl * dist
                                 + uSpots[i].kq * dist * dist);
            float shadow = (i == 0 && uSpotShadowsOn) ? spotShadowFactor() : 1.0;
            result += spot * atten * shadow * blinnPhong(N, V, L, uSpots[i].color,
                                                         uSpots[i].intensity, albedo);
        }
    }

    FragColor = vec4(result, 1.0);
}
