#include "scene/bezier_curve.hpp"

namespace cg
{

// ---------------------------------------------------------------------------
// Bernstein basis B_i^3(t)
// ---------------------------------------------------------------------------

static float bernstein3(int i, float t)
{
    float u = 1.0f - t;
    switch(i)
    {
        case 0: return u * u * u;
        case 1: return 3.0f * t * u * u;
        case 2: return 3.0f * t * t * u;
        case 3: return t * t * t;
        default: return 0.0f;
    }
}

// ---------------------------------------------------------------------------
// Direct evaluation using Bernstein basis
// ---------------------------------------------------------------------------

Point3 BezierCurve::evaluate(float t,
                              const Point3 &p0, const Point3 &p1,
                              const Point3 &p2, const Point3 &p3)
{
    const Point3 *pts[4] = {&p0, &p1, &p2, &p3};
    Point3        result(0.0f, 0.0f, 0.0f);
    for(int i = 0; i < 4; ++i)
    {
        float w = bernstein3(i, t);
        result.x += w * pts[i]->x;
        result.y += w * pts[i]->y;
        result.z += w * pts[i]->z;
    }
    return result;
}

// ---------------------------------------------------------------------------
// Constructor and forward-difference initialisation
// ---------------------------------------------------------------------------

BezierCurve::BezierCurve(const Point3 &p0, const Point3 &p1,
                          const Point3 &p2, const Point3 &p3,
                          int steps)
    : steps_(steps), current_step_(0)
{
    cp_[0] = p0;
    cp_[1] = p1;
    cp_[2] = p2;
    cp_[3] = p3;
    init_differences();
}

/**
 * Build the forward-difference table from the control points.
 *
 * Sample the curve at t = 0, h, 2h, 3h (where h = 1/steps) and compute
 * the Newton forward-difference coefficients:
 *
 *   d0 = q0
 *   d1 = q1 - q0
 *   d2 = q2 - 2*q1 + q0
 *   d3 = q3 - 3*q2 + 3*q1 - q0
 */
void BezierCurve::init_differences()
{
    const float h = 1.0f / static_cast<float>(steps_);

    Point3 q0 = evaluate(0.0f,        cp_[0], cp_[1], cp_[2], cp_[3]);
    Point3 q1 = evaluate(h,           cp_[0], cp_[1], cp_[2], cp_[3]);
    Point3 q2 = evaluate(2.0f * h,    cp_[0], cp_[1], cp_[2], cp_[3]);
    Point3 q3 = evaluate(3.0f * h,    cp_[0], cp_[1], cp_[2], cp_[3]);

    // X component
    d_[0][0] = q0.x;
    d_[1][0] = q1.x - q0.x;
    d_[2][0] = q2.x - 2.0f * q1.x + q0.x;
    d_[3][0] = q3.x - 3.0f * q2.x + 3.0f * q1.x - q0.x;

    // Y component
    d_[0][1] = q0.y;
    d_[1][1] = q1.y - q0.y;
    d_[2][1] = q2.y - 2.0f * q1.y + q0.y;
    d_[3][1] = q3.y - 3.0f * q2.y + 3.0f * q1.y - q0.y;

    // Z component
    d_[0][2] = q0.z;
    d_[1][2] = q1.z - q0.z;
    d_[2][2] = q2.z - 2.0f * q1.z + q0.z;
    d_[3][2] = q3.z - 3.0f * q2.z + 3.0f * q1.z - q0.z;

    current_step_ = 0;
}

void BezierCurve::reset()
{
    init_differences();
}

// ---------------------------------------------------------------------------
// Stepping via forward differences
// ---------------------------------------------------------------------------

void BezierCurve::step()
{
    if(current_step_ >= steps_) return;

    // Forward-difference recurrence (O(1) additions, no polynomial eval)
    for(int c = 0; c < 3; ++c)
    {
        d_[0][c] += d_[1][c];
        d_[1][c] += d_[2][c];
        d_[2][c] += d_[3][c];
        // d_[3][c] is constant for a cubic curve — never updated
    }

    ++current_step_;
}

Point3 BezierCurve::current_point() const
{
    return Point3(d_[0][0], d_[0][1], d_[0][2]);
}

bool BezierCurve::is_done() const
{
    return current_step_ >= steps_;
}

} // namespace cg
