#ifndef __SCENE_BEZIER_CURVE_HPP__
#define __SCENE_BEZIER_CURVE_HPP__

#include "geometry/point3.hpp"

namespace cg
{

/**
 * Cubic Bezier curve with forward differencing.
 *
 * Stores 4 control points P0..P3 and steps through the curve in `steps`
 * equal increments using the forward-difference recurrence:
 *
 *   d0 += d1    (current position advances by first difference)
 *   d1 += d2    (first  difference advances by second difference)
 *   d2 += d3    (second difference advances by third  difference)
 *   d3          (third  difference is constant for a cubic curve)
 *
 * The table is initialised by sampling the curve at t = 0, h, 2h, 3h
 * where h = 1/steps and computing the Newton forward differences.
 *
 * Call reset() to restart from t = 0 without rebuilding the table.
 */
class BezierCurve
{
  public:
    /**
     * Construct a cubic Bezier curve stepped in `steps` equal increments.
     * @param p0..p3  Control points (degree-3 Bezier)
     * @param steps   Total steps across t in [0, 1]
     */
    BezierCurve(const Point3 &p0, const Point3 &p1,
                const Point3 &p2, const Point3 &p3,
                int steps);

    /** Reset to t = 0 (reinitialises the forward-difference table). */
    void reset();

    /** Advance one step using the forward-difference recurrence. */
    void step();

    /** Return the current position P(t) on the curve. */
    Point3 current_point() const;

    /** Return true once all `steps` steps have been taken (t reached 1). */
    bool is_done() const;

    /** Total number of steps. */
    int total_steps() const { return steps_; }

    /**
     * Direct evaluation at an arbitrary t in [0, 1] using Bernstein basis.
     * Used to pre-sample the curve for polyline rendering.
     */
    static Point3 evaluate(float t,
                           const Point3 &p0, const Point3 &p1,
                           const Point3 &p2, const Point3 &p3);

  private:
    void init_differences();

    Point3 cp_[4];       // Control points P0..P3
    int    steps_;       // Total steps
    int    current_step_;

    // Forward-difference table: d_[k][c] for degree k in {0..3}, component c in {0=x,1=y,2=z}.
    // d_[0] holds the current position; d_[1..3] are the successive differences.
    float d_[4][3];
};

} // namespace cg

#endif
