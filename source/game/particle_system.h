#pragma once

#include "glsl_program.h"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

// Simple CPU particle pool for balloon-pop bursts and muzzle flashes.
// Rendered as additive point sprites via a dedicated shader (particle.vert/frag).
class ParticleSystem {
public:
    struct Particle {
        glm::vec3 pos;
        glm::vec3 vel;
        float life;       // seconds remaining
        float maxLife;
        glm::vec3 color;
        float size;
    };

    void burst(const glm::vec3& pos, const glm::vec3& color, int count,
               float speed, float life, float size);
    void update(float dt);

    // Draw with the given view/proj. Loads the particle shader from the asset
    // path on first use.
    void draw(const glm::mat4& view, const glm::mat4& proj,
              const std::string& shaderDir);

private:
    std::vector<Particle> _particles;
    std::unique_ptr<GLSLProgram> _shader;
    GLuint _vao = 0;
    GLuint _vbo = 0;
    bool _inited = false;
    void initGL();
};
