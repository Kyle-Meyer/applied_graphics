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
    // Determine if ray is entering or exiting the material
    // If dot(d, normal) < 0, ray is entering (normal points outward)
    // If dot(d, normal) > 0, ray is exiting (we're inside)
    float cos_i = d.dot(normal);
    float n1, n2;
    Vector3 adjusted_normal = normal;

    if (cos_i < 0.0f)
    {
        // Ray is entering the material (from air/vacuum)
        n1 = 1.0f;  // Index of refraction for air
        n2 = mat_node->get_index_of_refraction();
    }
    else
    {
        // Ray is exiting the material (going back to air)
        n1 = mat_node->get_index_of_refraction();
        n2 = 1.0f;  // Index of refraction for air
        adjusted_normal = normal * -1.0f;  // Flip normal to point in correct direction
    }

    // Use the base Ray3 refract method
    RayRefractionResult result = refract(int_pt, adjusted_normal, n1, n2);
    total_internal_reflection = result.total_internal_refraction;

    Ray refracted_ray(result.refracted_ray, recursion_level_ - 1, threshold_);
    return refracted_ray;
}

bool Ray::below_threshold()
{
    // Complete in 605.767 - ray tracing project.
    return recursion_level_ <= 0;
}

} // namespace cg
