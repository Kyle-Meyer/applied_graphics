//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    rt_sphere_node.hpp
//	Purpose: Sphere for use in ray tracing.
//
//============================================================================

#ifndef __RAY_TRACER_RT_SPHERE_NODE_HPP__
#define __RAY_TRACER_RT_SPHERE_NODE_HPP__

#include "geometry/bounding_sphere.hpp"
#include "geometry/point3.hpp"
#include "geometry/ray3.hpp"

#include "scene/geometry_node.hpp"

namespace cg
{

/**
 * Sphere for ray tracing.
 */
class RTSphereNode : public GeometryNode
{
  public:
    /**
     * Constructor for a ray traced sphere
     * @param  center  Sphere center
     * @param  radius  Sphere radius
     */
    RTSphereNode(const Point3 &center, float radius);

    /**
     * Calculates the intersect of a ray and a sphere.
     * @param   ray  Ray to intersect with this sphere
     * @return  Returns whether or not there is an intersection, and the distance
     *          at which the intersection occurs (0.0 if no intersection).
     */
    RayObjectIntersectResult intersect(const Ray3 &ray) const;

    /**
     * Gets the normal to the sphere at the intersect point
     * @param   int_pt  Intersection point with the sphere
     * @return  Returns a unit length normal at the intersection point.
     */
    Vector3 get_normal(const Point3 &int_pt) override;

    /**
     * Get the texture coordinate at an intersection point using spherical mapping.
     * @param  int_pt  Intersection point on the sphere surface
     * @return Returns the texture coordinate (s, t) at the intersection point
     */
    Point2 get_texture_coord(const Point3 &int_pt) override;

    /**
     * Ray tracing intersect method
     */
    void find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest) override;

    /**
     * Ray tracing intersect method - checks if an object in the scene graph
     * intersects the ray along the specified distance. Can return true if any
     * object intersects.
     */
    bool does_intersect_exist(Ray3 ray, float d, SceneState &current_state) override;

    /**
     * Spheres are always convex.
     */
    bool is_convex(void) const;

  private:
    BoundingSphere sphere_;
};

} // namespace cg

#endif
