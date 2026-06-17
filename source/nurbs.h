#pragma once

#include <glm/glm.hpp>
#include <vector>

// Hand-written B-spline (NURBS with uniform weights) curve. No GLU NURBS API.
// Two modes:
//   - Clamped (open): curve passes through the first/last control points.
//     Used for open paths.
//   - Periodic (closed): uniform integer knots + parameter wrapping, so the
//     curve is C^{degree-1} continuous across the seam and loops seamlessly.
//     Used for the moving target trajectory.
class NurbsCurve {
public:
    enum class Mode { Clamped, Periodic };

    // Clamped uniform curve. degree >= 1 and < controlPoints.size().
    void setClampedUniform(int degree, std::vector<glm::vec3> controlPoints);

    // Periodic (closed) uniform curve. The curve wraps so evaluate(0) ==
    // evaluate(period) and is smooth across the seam. degree >= 1 and
    // < controlPoints.size(). No need to duplicate control points at the end.
    void setPeriodicUniform(int degree, std::vector<glm::vec3> controlPoints);

    // Evaluate the curve at parameter u. For Periodic mode u is taken modulo
    // the period, so any u works. For Clamped mode u is clamped to [0,1].
    glm::vec3 evaluate(float u) const;

    int degree() const { return _degree; }
    Mode mode() const { return _mode; }
    const std::vector<glm::vec3>& controlPoints() const { return _controlPoints; }

private:
    int _degree = 3;
    Mode _mode = Mode::Clamped;
    std::vector<glm::vec3> _controlPoints;
    std::vector<float> _knots;   // clamped: clamped knots; periodic: uniform integer
    float _period = 1.0f;        // periodic: loop length in parameter units
    float _loopStart = 0.0f;     // periodic: parameter where the seamless loop begins

    // Shared helpers (work for both modes via the _knots vector).
    int findSpan(float t) const;
    float basis(int i, int p, float t) const;
};
