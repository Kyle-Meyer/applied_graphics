#include "RayTracer/rt_sphere_node.hpp"

#include "geometry/geometry.hpp"

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
    // Complete in 605.767
    return Vector3(0.0f, 0.0f, 0.0f);
}

void RTSphereNode::find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest)
{
    // Complete in 605.767
    // This should be a leaf node.
    // TODO - test for intersection (set t value) and see if closer than tMin
    //     float t = 0.0f;
    //     if(t > EPSILON) { closest.set_slosest(current_state, this, t); }
}

bool RTSphereNode::does_intersect_exist(Ray3 ray, float d, SceneState &current_state)
{
    // This should be a leaf node - test for intersection

    // Spheres are convex - we can skip checking the current
    // object since convex objects cannot shadow themselves.
    if(this == current_state.geometry_node) { return false; }

    // Check if object is between ray start and light position
    // Complete in 605.767
    return false;
}

bool RTSphereNode::is_convex(void) const { return true; }

} // namespace cg
