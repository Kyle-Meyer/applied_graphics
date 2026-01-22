#include "common/logging.hpp"
#include "geometry/geometry.hpp"

#include <algorithm>

namespace cg
{

void vector_test_module1()
{
    log_msg("Vector Test");

    // ------------------ 2D Point and Vector Tests --------------------/
    log_msg("\n2-D Point and Vector Operations\n");

    // Construct some 2D points
    Point2 a(5.0f, 0.0f);
    Point2 b(-2.0f, 4.0f);
    Point2 c(1.0f, -5.0f);
    log_msg("Point a (%.2f, %.2f)", a.x, a.y);
    log_msg("Point b (%.2f, %.2f)", b.x, b.y);
    log_msg("Point c (%.2f, %.2f)", c.x, c.y);

    // Midpoint between a and b
    Point2 m = a.mid_point(b);
    log_msg("Mid point between a and b is (%.2f, %.2f)", m.x, m.y);

    // Point 1/4 of the way from a to b
    Point2 q = a.affine_combination(0.75f, 0.25f, b);
    log_msg("Point q (1/4 between a and b) is (%.2f, %.2f)", q.x, q.y);

    // Construct a vector from point a to point b
    Vector2 v(a, b);
    log_msg("Vector v (%.2f, %.2f)", v.x, v.y);

    // Get the length of v
    log_msg("Norm of v = %.2f", v.norm());

    // Create a unit vector from v
    Vector2 u = v.normalize();
    log_msg("Unit vector of v = (%.2f, %.2f)", u.x, u.y);
    log_msg("Note: v is now a unit vector (result of normalize())");

    // Create a vector w from point b to point c
    Vector2 w = c - b;
    log_msg("Vector w (%.2f, %.2f)", w.x, w.y);
    w.normalize();
    log_msg("Unit vector of w = (%.2f, %.2f)", w.x, w.y);

    // Create vectors t0 and t1 that are in the opposite direction of w
    // and have a length of 4. (Tests both * operators)
    Vector2 t0 = w * -4.0f;
    log_msg("Vector t0 (%.2f, %.2f)", t0.x, t0.y);
    Vector2 t1 = -4.0f * w;
    log_msg("Vector t1 (%.2f, %.2f)", t1.x, t1.y);

    // Add vector v and vector w
    Vector2 wv = v + w;
    log_msg("Vector v + w = (%.2f, %.2f)", wv.x, wv.y);

    // Dot product of v and w
    float dot = v.dot(w);
    log_msg("Dot Product of v and w is %.4f", dot);

    // Find the angle in degrees between v and w
    Vector2 v2 = v * 2.0f;
    Vector2 w2 = w * 0.5f;
    log_msg("Angle between v2 and w2 = %.2f degrees", radians_to_degrees(v2.angle_between(w2)));

    // Add vector t0 to point c
    Point2 d = c + t0;
    log_msg("Point d: (%.2f %.2f)", d.x, d.y);

    // Find the projection of vector w onto vector v
    Vector2 wOnv = w2.projection(v2);
    log_msg("Projection of w2 onto v2 is (%.2f %.2f)", wOnv.x, wOnv.y);

    // Find perpendicular 2-D vectors
    Vector2 v2p1 = wOnv.get_perpendicular(true);
    log_msg("Clockwise perpendicular to wOnV is (%.2f %.2f)", v2p1.x, v2p1.y);
    Vector2 v2p2 = wOnv.get_perpendicular(false);
    log_msg("Counter-Clockwise perpendicular to wOnV is (%.2f %.2f)", v2p2.x, v2p2.y);

    // Find the reflection of vector t0 off the x axis
    Vector2 n1(0.0f, 1.0f);
    Vector2 r = t0.reflect(n1);
    log_msg("Reflection of t0 off the x-axis is %.4f %.4f", r.x, r.y);

    // ------------------ 3-D Point and Vector Tests --------------------/
    log_msg("\n3-D Point and Vector Operations\n");

    // Construct three points
    Point3 a3(0.0f, -2.0f, 0.0f);
    Point3 b3(-4.0f, 2.0f, 5.0f);
    Point3 c3(2.0f, -1.0f, -3.0f);
    log_msg("Point a3 (%.2f, %.2f, %.2f)", a3.x, a3.y, a3.z);
    log_msg("Point b3 (%.2f, %.2f, %.2f)", b3.x, b3.y, b3.z);
    log_msg("Point c3 (%.2f, %.2f, %.2f)", c3.x, c3.y, c3.z);

    // Midpoint between a3 and b3
    Point3 mid_pt = a3.mid_point(b3);
    log_msg("Mid point between a3 and b3 is (%.2f, %.2f, %.2f)", mid_pt.x, mid_pt.y, mid_pt.z);

    // Point 1/4 of the way from a3 to b3
    Point3 q3 = a3.affine_combination(0.75f, 0.25f, b3);
    log_msg("Point q3 (1/4 between a3 and b3) is (%.2f, %.2f, %.2f)", q3.x, q3.y, q3.z);

    // Construct a vector from point a3 to point b3
    Vector3 v3(a3, b3);
    log_msg("Vector v3 (%.2f, %.2f, %.2f)", v3.x, v3.y, v3.z);

    // Get the length of v3
    log_msg("Norm of v = %.2f", v3.norm());

    // Create a unit vector from v2
    Vector3 u3 = v3.normalize();
    log_msg("Unit vector of v3 = (%.2f, %.2f, %.2f)", u3.x, u3.y, u3.z);
    log_msg("Note: v3 is now a unit vector (result of normalize())");

    // Create a vector t that is in the opposite direction of u
    // and has a length of 10. (Tests both * operators)
    Vector3 t3 = u3 * -10.0f;
    log_msg("Vector t3 (%.2f, %.2f, %.2f)", t3.x, t3.y, t3.z);

    Vector3 t31 = -10.0f * u3;
    log_msg("Vector t31 (%.2f, %.2f, %.2f)", t31.x, t31.y, t31.z);

    // Create a vector from Point a3 to point c3
    Vector3 w3 = c3 - a3;
    log_msg("Vector w3 (%.2f, %.2f, %.2f)", w3.x, w3.y, w3.z);

    // Add vector v and vector w
    Vector3 wv3 = v3 + w3;
    log_msg("Vector v3 + w3 = (%.2f, %.2f, %.2f)", wv3.x, wv3.y, wv3.z);

    // Dot product of v3 and w3
    float dot3 = v3.dot(w3);
    log_msg("Dot Product of v3 and w3 is %.4f", dot3);

    // Find the angle in degrees between v and w
    Vector3 v4 = v3 * 2.0f;
    Vector3 w4 = w3 * 0.5f;
    log_msg("Angle between v4 and w4 = %.2f degrees", radians_to_degrees(v4.angle_between(w4)));

    // Add vector t to point c
    Point3 d3 = c3 + t3;
    log_msg("Point d3: (%.2f %.2f %.2f)", d3.x, d3.y, d3.z);

    // Find the projection of vector w onto vector v
    Vector3 w4Onv4 = w4.projection(v4);
    log_msg("Projection of w4 onto v4 is (%.2f %.2f %.2f)", w4Onv4.x, w4Onv4.y, w4Onv4.z);

    // ------------------- Line segment tests ------------------------//

    log_msg("\n2-D Line Segment Operations\n");

    // Construct a line segment from point a2 to point b2
    Point2       a2(5.0f, 3.0f);
    Point2       b2(-3.0f, 10.0f);
    LineSegment2 seg2(a2, b2);

    // Find the distance of point c2a from line segment seg2
    Point2 c2a(-5.0f, 12.0f);
    Point2 closest2;
    {
        auto result = seg2.distance(c2a);
        log_msg("Distance of point c2a from line segment ab is %.2f", result.distance);
        log_msg("Closest Point is (%.2f, %.2f)", result.closest_point.x, result.closest_point.y);
    }

    // Find the distance of point c2b from line segment seg2
    Point2 c2b(3.0f, 0.0f);
    {
        auto result = seg2.distance(c2b);
        log_msg("Distance of point c2b from line segment ab is %.2f", result.distance);
        log_msg("Closest Point is (%.2f, %.2f)", result.closest_point.x, result.closest_point.y);
    }

    // Find the distance of point c2c from line segment seg2
    Point2 c2c(12.0f, 1.0f);
    {
        auto result = seg2.distance(c2c);
        log_msg("Distance of point c2c from line segment ab is %.2f", result.distance);
        log_msg("Closest Point is (%.2f, %.2f)", result.closest_point.x, result.closest_point.y);
    }

    // Construct other 2-D line segments and test for intersection
    {
        LineSegment2 seg2b({-10.0f, 12.0f}, {4.0f, 11.0f});
        {
            auto result = seg2b.intersect(seg2);
            if(result.intersects)
            {
                log_msg("seg2b intersects seg2 at point: (%.2f, %.2f)",
                        result.intersect_point.x,
                        result.intersect_point.y);
            }
            else { log_msg("seg2b does not intersect seg2"); }
        }

        {
            auto result = seg2.intersect(seg2b);
            if(result.intersects)
            {
                log_msg("seg2 intersects seg2b at point: (%.2f, %.2f)",
                        result.intersect_point.x,
                        result.intersect_point.y);
            }
            else { log_msg("seg2 does not intersect seg2b"); }
        }
    }
    {
        LineSegment2 seg2c({-3.0f, 8.0f}, {5.0f, 1.0f});
        {
            auto result = seg2c.intersect(seg2);
            if(result.intersects)
            {
                log_msg("seg2c intersects seg2 at point: (%.2f, %.2f)",
                        result.intersect_point.x,
                        result.intersect_point.y);
            }
            else { log_msg("seg2c does not intersect seg2"); }
        }

        {
            auto result = seg2.intersect(seg2c);
            if(result.intersects)
            {
                log_msg("seg2 intersects seg2c at point: (%.2f, %.2f)",
                        result.intersect_point.x,
                        result.intersect_point.y);
            }
            else { log_msg("seg2 does not intersect seg2c"); }
        }
    }
    {
        LineSegment2 seg2d({0.0f, 0.0f}, {7.0f, 7.0f});
        {
            auto result = seg2d.intersect(seg2);
            if(result.intersects)
            {
                log_msg("seg2d intersects seg2 at point: (%.2f, %.2f)",
                        result.intersect_point.x,
                        result.intersect_point.y);
            }
            else { log_msg("seg2d does not intersect seg2"); }
        }
        {
            auto result = seg2.intersect(seg2d);
            if(result.intersects)
            {
                log_msg("seg2 intersects seg2d at point: (%.2f, %.2f)",
                        result.intersect_point.x,
                        result.intersect_point.y);
            }
            else { log_msg("seg2 does not intersect seg2d"); }
        }
    }

    // ------------ Line segment 3-D tests ---------------//
    log_msg("\n3-D Line Segment Operations\n");

    // Construct a line segment in 3-D
    LineSegment3 seg3({2.0f, 4.0f, -5.0f}, {-5.0f, 1.0f, 6.0f});

    // Find the distance of point c3a from line segment seg3
    Point3 c3a(12.0f, 5.0f, 10.0f);
    {
        auto result = seg3.distance(c3a);
        log_msg("Distance of point c3a from line segment seg3 is %.2f", result.distance);
        log_msg("Closest Point is (%.2f, %.2f, %.2f)",
                result.closest_point.x,
                result.closest_point.y,
                result.closest_point.z);
    }

    // Find the distance of point c3b from line segment seg3
    Point3 c3b(5.0f, 2.0f, 0.0f);
    {
        auto result = seg3.distance(c3b);
        log_msg("Distance of point c3b from line segment seg3 is %.2f", result.distance);
        log_msg("Closest Point is (%.2f, %.2f, %.2f)",
                result.closest_point.x,
                result.closest_point.y,
                result.closest_point.z);
    }

    // Find the distance of point c3c from line segment seg3
    Point3 c3c(3.0f, 5.0f, -6.0f);
    {
        auto result = seg3.distance(c3c);
        log_msg("Distance of point c3c from line segment seg3 is %.2f", result.distance);
        log_msg("Closest Point is (%.2f, %.2f, %.2f)",
                result.closest_point.x,
                result.closest_point.y,
                result.closest_point.z);
    }

    // Find the distance of point c3d from line segment seg3
    Point3 c3d(-7.0f, -1.0f, 8.0f);
    {
        auto result = seg3.distance(c3d);
        log_msg("Distance of point c3d from line segment seg3 is %.2f", result.distance);
        log_msg("Closest Point is (%.2f, %.2f, %.2f)",
                result.closest_point.x,
                result.closest_point.y,
                result.closest_point.z);
    }

    // -------------------------- Plane tests ------------------- //

    // Construct 3 points (not all in x,y plane!)
    Point3 m3(-5.0f, -3.0f, 5.0f);
    Point3 n3(3.0f, -5.0f, 1.0f);
    Point3 o3(4.0f, 4.0f, -3.0f);
    log_msg("\nPlane Test\n");
    log_msg("Point m3 (%.2f, %.2f, %.2f)", m3.x, m3.y, m3.z);
    log_msg("Point n3 (%.2f, %.2f, %.2f)", n3.x, n3.y, n3.z);
    log_msg("Point o3 (%.2f, %.2f, %.2f)", o3.x, o3.y, o3.z);

    // Get the cross product of vector mn and mo
    Vector3 perp = (n3 - m3).cross(o3 - m3);
    log_msg("Cross Product of mn and mo = %.2f %.2f %.2f", perp.x, perp.y, perp.z);

    // Construct a plane through m3,n1,o3
    Plane plane(m3, n3, o3);
    log_msg(
        "Plane through points m3,n3,o3 = %.2f %.2f %.2f %.2f", plane.a, plane.b, plane.c, plane.d);

    plane.normalize();
    log_msg("Normalized Plane = %.2f %.2f %.2f %.2f", plane.a, plane.b, plane.c, plane.d);

    // Construct plane with point m3 and perp
    Plane plane2(m3, perp);
    plane2.normalize();
    log_msg("Normalized Plane with point m and normal perp = %.2f %.2f %.2f %.2f",
            plane.a,
            plane.b,
            plane.c,
            plane.d);

    // Check if point n3 is on the plane
    float s = plane.solve(n3);
    if(s > EPSILON) log_msg("Point n3 is in front of the plane: s = %.2f", s);
    else if(s < -EPSILON) log_msg("Point n3 is behind the plane: s = %.2f", s);
    else log_msg("Point n3 is on the plane");

    // Check point k against the plane
    Point3 k(5.0, 2.0, 7.0f);
    log_msg("Point k (%.2f, %.2f, %.2f)", k.x, k.y, k.z);
    s = plane.solve(k);
    if(s > EPSILON) log_msg("Point k is in front of the plane: s = %.2f", s);
    else if(s < -EPSILON) log_msg("Point k is behind the plane: s = %.2f", s);
    else log_msg("Point k is on the plane");

    // Construct a vector from between 2 points
    Vector3 v3a({0.0f, -2.0f, 0.0f}, {-4.0f, 2.0f, 5.0f});
    log_msg("Vector v3a (%.2f, %.2f, %.2f)", v3a.x, v3a.y, v3a.z);

    // Reflect vector v3 off the plane
    Vector3 r3 = v3a.reflect(plane.get_normal());
    log_msg("Reflection of v3a off the plane = %.2f, %.2f, %.2f", r3.x, r3.y, r3.z);

    // --------------------- Student Written Expressions ------------------- //
    log_msg("\nStudent Written Expressions");

    // Student TODO:
    // Write an expression using the arithmetic operators (+,-,*,/) for points, vectors, and scalars
    // to find the point (m34) that is 3/4 of the distance along a line segment from m1 to m2.
    // Do not use affine_combination or mid_point
    Point3 m1(1.0f, 2.0f, 3.0f);
    Point3 m2(5.0f, -5.0f, 0.0f);
    Point3 m34 = m1 + (m2 - m1) * 0.75f;
    log_msg("m34 = (%.2f %.2f %.2f)", m34.x, m34.y, m34.z);
}

} // namespace cg
