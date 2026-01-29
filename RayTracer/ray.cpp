#include "RayTracer/ray.hpp"

namespace cg
{

Ray::Ray(Ray3 &initial_ray, int32_t depth, float t)
{
    o = initial_ray.o;
    d = initial_ray.d;
    recursion_level_ = depth;
    threshold_ = t;
}

Ray Ray::get_refracted_ray(const Point3       &int_pt,
                           const Vector3      &normal,
                           const MaterialNode *mat_node,
                           bool               &total_internal_reflection)
{
    // Complete in 605.767
    Ray3 r = Ray3();

    Ray refracted_ray(r, recursion_level_ - 1, threshold_);
    return refracted_ray;
}

bool Ray::below_threshold()
{
    // Complete in 605.767 - ray tracing project.
    return recursion_level_ <= 0;
}

} // namespace cg
