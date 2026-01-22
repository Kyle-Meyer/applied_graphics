#include "common/logging.hpp"
#include "geometry/geometry.hpp"

#include <algorithm>

namespace cg
{

void vector_test_module5()
{
    log_msg("Ray Tests");

    // ---------- Ray to Sphere Intersection Tests ----------------//

    // Create a bounding sphere centered at (2,2,0) with radius 4.0f
    Point3         origin(2.0f, 2.0f, 0.0f);
    BoundingSphere sphere(origin, 4.0f);

    {
        // Create a unit length ray starting at (-10, 10, 0) in the +x direction and test
        // intersection
        Ray3 ray1(Point3(-10.0f, 10.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f));
        auto result = ray1.intersect(sphere);
        if(result.intersects)
        {
            Point3 intersect = ray1.intersect(result.distance);
            log_msg("   Ray1 intersects sphere at %f %f %f, t = %f",
                    intersect.x,
                    intersect.y,
                    intersect.z,
                    result.distance);
        }
        else { log_msg("   Ray1 does not intersect sphere"); }
    }

    {
        // Create a unit length ray starting at (4, 8, 0) in the +x direction and test intersection
        Ray3 ray2(Point3(4.0f, 8.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f));
        auto result = ray2.intersect(sphere);
        if(result.intersects)
        {
            Point3 intersect = ray2.intersect(result.distance);
            log_msg("   Ray2 intersects sphere at %f %f %f, t = %f",
                    intersect.x,
                    intersect.y,
                    intersect.z,
                    result.distance);
        }
        else { log_msg("   Ray2 does not intersect sphere"); }
    }

    {
        // Create a unit length ray starting at (-10, 4, 0) in the +x direction and test
        // intersection
        Ray3 ray3(Point3(-10.0f, 4.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f));
        auto result = ray3.intersect(sphere);
        if(result.intersects)
        {
            Point3 intersect = ray3.intersect(result.distance);
            log_msg("   Ray3 intersects sphere at %f %f %f, t = %f",
                    intersect.x,
                    intersect.y,
                    intersect.z,
                    result.distance);
        }
        else { log_msg("   Ray3 does not intersect sphere"); }
    }

    {
        // Create a unit length ray starting at (4, 4, 0) in the +y direction and test intersection
        Ray3 ray4(Point3(4.0f, 4.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
        auto result = ray4.intersect(sphere);
        if(result.intersects)
        {
            Point3 intersect = ray4.intersect(result.distance);
            log_msg("   Ray4 intersects sphere at %f %f %f, t = %f",
                    intersect.x,
                    intersect.y,
                    intersect.z,
                    result.distance);
        }
        else { log_msg("   Ray4 does not intersect sphere"); }
    }

    {
        // Create a unit length ray starting at (4, 0, 0) in the +y direction and test intersection
        Ray3 ray5(Point3(4.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
        auto result = ray5.intersect(sphere);
        if(result.intersects)
        {
            Point3 intersect = ray5.intersect(result.distance);
            log_msg("   Ray5 intersects sphere at %f %f %f, t = %f",
                    intersect.x,
                    intersect.y,
                    intersect.z,
                    result.distance);
        }
        else { log_msg("   Ray5 does not intersect sphere"); }
    }

    {
        // Create a unit length ray starting at (2, 8, 0) in the +y direction and test intersection
        Ray3 ray6(Point3(2.0f, 8.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
        auto result = ray6.intersect(sphere);
        if(result.intersects)
        {
            Point3 intersect = ray6.intersect(result.distance);
            log_msg("   Ray6 intersects sphere at %f %f %f, t = %f",
                    intersect.x,
                    intersect.y,
                    intersect.z,
                    result.distance);
        }
        else { log_msg("   Ray6 does not intersect sphere"); }
    }
}

} // namespace cg
