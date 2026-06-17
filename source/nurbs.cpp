#include "nurbs.h"

#include <cassert>
#include <cmath>

void NurbsCurve::setClampedUniform(int degree, std::vector<glm::vec3> controlPoints) {
    assert(degree >= 1);
    assert(static_cast<int>(controlPoints.size()) > degree);
    _degree = degree;
    _mode = Mode::Clamped;
    _controlPoints = std::move(controlPoints);

    int n = static_cast<int>(_controlPoints.size()) - 1; // last control index
    int m = n + _degree + 1;                              // last knot index
    _knots.resize(m + 1);
    for (int i = 0; i <= _degree; ++i) {
        _knots[i] = 0.0f;
        _knots[m - i] = 1.0f;
    }
    int interior = (n + 1) - (_degree + 1);
    for (int i = 1; i <= interior; ++i) {
        _knots[_degree + i] = static_cast<float>(i) / (interior + 1);
    }
}

void NurbsCurve::setPeriodicUniform(int degree, std::vector<glm::vec3> controlPoints) {
    // Build a periodic (closed) uniform B-spline by duplicating the first
    // `degree` control points at the end and using a UNIFORM (non-clamped) knot
    // vector with integer spacing. The curve is then evaluated on the parameter
    // range [degree, N) (N = original control-point count), which is one full
    // seamless loop: the start and end positions match and the curve is
    // C^{degree-1} continuous across the seam.
    assert(degree >= 1);
    assert(static_cast<int>(controlPoints.size()) > degree);
    _degree = degree;
    _mode = Mode::Periodic;

    int n0 = static_cast<int>(controlPoints.size());   // original count
    // Extended control list = original + first `degree` wrapped.
    std::vector<glm::vec3> ext = controlPoints;
    for (int i = 0; i < degree; ++i) {
        ext.push_back(controlPoints[i]);
    }
    _controlPoints = std::move(ext);
    int n = static_cast<int>(_controlPoints.size()) - 1; // last ext index
    int m = n + _degree + 1;                             // last knot index
    _knots.resize(m + 1);
    for (int j = 0; j <= m; ++j) {
        _knots[j] = static_cast<float>(j);              // uniform integer knots
    }
    // The seamless loop is the parameter range [degree, n0). Length == n0.
    _period = static_cast<float>(n0);
    _loopStart = static_cast<float>(degree);
}

glm::vec3 NurbsCurve::evaluate(float u) const {
    if (_mode == Mode::Clamped) {
        float t = glm::clamp(u, 0.0f, 1.0f);
        if (t <= 0.0f) {
            return _controlPoints.front();
        }
        if (t >= 1.0f) {
            return _controlPoints.back();
        }
        int span = findSpan(t);
        glm::vec3 point(0.0f);
        for (int i = 0; i <= _degree; ++i) {
            int idx = span - _degree + i;
            if (idx < 0 || idx >= static_cast<int>(_controlPoints.size())) {
                continue;
            }
            point += basis(idx, _degree, t) * _controlPoints[idx];
        }
        return point;
    }

    // Periodic: wrap u into one loop [loopStart, loopStart + period).
    float t = _loopStart + std::fmod(u, _period);
    if (t < _loopStart) {
        t += _period;
    }
    int span = findSpan(t);
    glm::vec3 point(0.0f);
    for (int i = 0; i <= _degree; ++i) {
        int idx = span - _degree + i;
        if (idx < 0 || idx >= static_cast<int>(_controlPoints.size())) {
            continue;
        }
        point += basis(idx, _degree, t) * _controlPoints[idx];
    }
    return point;
}

int NurbsCurve::findSpan(float t) const {
    int n = static_cast<int>(_controlPoints.size()) - 1;
    if (t >= _knots[n + 1]) {
        return n;
    }
    if (t <= _knots[_degree]) {
        return _degree;
    }
    int low = _degree;
    int high = n + 1;
    int mid = (low + high) / 2;
    while (t < _knots[mid] || t >= _knots[mid + 1]) {
        if (t < _knots[mid]) {
            high = mid;
        } else {
            low = mid;
        }
        mid = (low + high) / 2;
    }
    return mid;
}

float NurbsCurve::basis(int i, int p, float t) const {
    // Standard Cox-de Boor on the (possibly uniform) knot vector, with
    // zero-division guards.
    if (p == 0) {
        return (t >= _knots[i] && t < _knots[i + 1]) ? 1.0f : 0.0f;
    }
    float leftDenom = _knots[i + p] - _knots[i];
    float left = (leftDenom != 0.0f) ? (t - _knots[i]) / leftDenom * basis(i, p - 1, t)
                                     : 0.0f;
    float rightDenom = _knots[i + p + 1] - _knots[i + 1];
    float right = (rightDenom != 0.0f)
                      ? (_knots[i + p + 1] - t) / rightDenom * basis(i + 1, p - 1, t)
                      : 0.0f;
    return left + right;
}
