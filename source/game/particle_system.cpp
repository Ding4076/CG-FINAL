#include "particle_system.h"

#include "gl_utility.h"

#include <algorithm>
#include <glm/gtc/random.hpp>

void ParticleSystem::burst(const glm::vec3& pos, const glm::vec3& color, int count,
                           float speed, float life, float size) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = pos;
        p.vel = glm::sphericalRand(speed);
        p.life = life * (0.5f + 0.5f * glm::linearRand(0.0f, 1.0f));
        p.maxLife = p.life;
        p.color = color;
        p.size = size;
        _particles.push_back(p);
    }
}

void ParticleSystem::update(float dt) {
    const glm::vec3 gravity(0.0f, -3.0f, 0.0f);
    for (auto& p : _particles) {
        p.vel += gravity * dt;
        p.pos += p.vel * dt;
        p.life -= dt;
    }
    _particles.erase(
        std::remove_if(_particles.begin(), _particles.end(),
                       [](const Particle& p) { return p.life <= 0.0f; }),
        _particles.end());
}

void ParticleSystem::initGL() {
    if (_inited) {
        return;
    }
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    // position (vec3) + color (vec3) per particle
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6,
                          (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    _inited = true;
}

void ParticleSystem::draw(const glm::mat4& view, const glm::mat4& proj,
                          const std::string& shaderDir) {
    if (_particles.empty()) {
        return;
    }
    initGL();
    if (!_shader) {
        _shader = std::make_unique<GLSLProgram>();
        _shader->attachVertexShaderFromFile(shaderDir + "particle.vert");
        _shader->attachFragmentShaderFromFile(shaderDir + "particle.frag");
        _shader->link();
    }

    // Build the interleaved vertex data, fading alpha by remaining life.
    std::vector<float> data;
    data.reserve(_particles.size() * 6);
    float alpha = 1.0f;  // per-frame global alpha is 1; per-particle via color scale
    for (const auto& p : _particles) {
        float f = p.life / p.maxLife;   // 0..1
        data.push_back(p.pos.x);
        data.push_back(p.pos.y);
        data.push_back(p.pos.z);
        data.push_back(p.color.r * f);
        data.push_back(p.color.g * f);
        data.push_back(p.color.b * f);
    }

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(),
                 GL_DYNAMIC_DRAW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // additive
    glDepthMask(GL_FALSE);               // don't write depth (particles are overlay)

    _shader->use();
    _shader->setUniformMat4("uView", view);
    _shader->setUniformMat4("uProj", proj);
    _shader->setUniformFloat("uAlpha", alpha);
    glBindVertexArray(_vao);
    glDrawArrays(GL_POINTS, 0, (GLsizei)_particles.size());
    glBindVertexArray(0);
    _shader->unuse();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
