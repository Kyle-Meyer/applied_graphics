#include "RayTracer/rt_sphere_node.hpp"

#include "geometry/geometry.hpp"
#include "geometry/point2.hpp"

#include <cmath>

namespace cg
{

RTSphereNode::RTSphereNode(const Point3 &center, float radius)
{
    sphere_.center = center;
    sphere_.radius = radius;
}

RayObjectIntersectResult RTSphereNode::intersect(const Ray3 &ray) const
{
    return ray.intersect(sphere_);
}

Vector3 RTSphereNode::get_normal(const Point3 &int_pt)
{
    // For a sphere, the normal at any point is the vector from the center to that point
    Vector3 normal(sphere_.center, int_pt);
    normal.normalize();
    return normal;
}

Point2 RTSphereNode::get_texture_coord(const Point3 &int_pt)
{
    // Use spherical mapping to convert 3D point to texture coordinates
    // First get the normalized vector from center to intersection point
    Vector3 p(sphere_.center, int_pt);
    p.normalize();

    // Convert to spherical coordinates
    // theta (azimuthal angle): angle around the y-axis, range [0, 2*pi]
    // phi (polar angle): angle from the positive y-axis, range [0, pi]

    // Calculate theta using atan2(x, z) - gives angle in range [-pi, pi]
    float theta = std::atan2(p.x, p.z);

    // Calculate phi using acos(y) - gives angle in range [0, pi]
    float phi = std::acos(p.y);

    // Map to texture coordinates [0, 1]
    // s (horizontal): map theta from [-pi, pi] to [0, 1]
    float s = (theta + M_PI) / (2.0f * M_PI);

    // t (vertical): map phi from [0, pi] to [0, 1]
    float t = phi / M_PI;

    return Point2(s, t);
}

void RTSphereNode::find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest)
{
    // This is a leaf node - test for intersection with the sphere
    RayObjectIntersectResult result = intersect(ray);

    // Check if there's a valid intersection
    if(result.intersects && result.distance > EPSILON)
    {
        // Check if this intersection is closer than the current closest
        if(result.distance < closest.t_min)
        {
            // Update closest intersection information
            closest.t_min = result.distance;
            closest.geometry_node = this;
            closest.material_node = current_state.material_node;
            closest.texture_node = current_state.texture_node;

            // Copy transformation matrices if transforms are required
            if(current_state.transform_required)
            {
                closest.transform_required = true;
                closest.inverse_matrix = current_state.inverse_matrix;
                closest.normal_matrix = current_state.normal_matrix;
            }
        }
    }
}

bool RTSphereNode::does_intersect_exist(Ray3 ray, float d, SceneState &current_state)
{
    // This should be a leaf node - test for intersection

    // Spheres are convex - we can skip checking the current
    // object since convex objects cannot shadow themselves.
    if(this == current_state.geometry_node) { return false; }

    // Check if object is between ray start and light position
    RayObjectIntersectResult result = intersect(ray);

    // Return true if there's an intersection between the ray origin and the light
    // The intersection must be valid (> EPSILON) and closer than the light distance
    return (result.intersects && result.distance > EPSILON && result.distance < d);
}

bool RTSphereNode::is_convex(void) const { return true; }

} // namespace cg
