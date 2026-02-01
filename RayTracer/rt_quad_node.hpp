//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//
//	File:    rt_quad_node.hpp
//	Purpose: Quadrilateral (planar surface) for use in ray tracing.
//============================================================================

#ifndef __RAY_TRACER_RT_QUAD_NODE_HPP__
#define __RAY_TRACER_RT_QUAD_NODE_HPP__

#include "geometry/point3.hpp"
#include "geometry/vector3.hpp"
#include "geometry/ray3.hpp"
#include "scene/geometry_node.hpp"

namespace cg
{

/**
 * Quadrilateral for ray tracing. Defined by 4 corner points in CCW order.
 */
class RTQuadNode : public GeometryNode
{
  public:
    /**
     * Constructor for a ray traced quad
     * @param  v0  First vertex (corner)
     * @param  v1  Second vertex (corner)
     * @param  v2  Third vertex (corner)
     * @param  v3  Fourth vertex (corner)
     * Vertices should be in counter-clockwise order when viewed from front.
     */
    RTQuadNode(const Point3 &v0, const Point3 &v1, const Point3 &v2, const Point3 &v3);

    /**
     * Gets the normal to the quad (same everywhere since it's planar)
     * @param   int_pt  Intersection point (not used, normal is constant)
     * @return  Returns a unit length normal.
     */
    Vector3 get_normal(const Point3 &int_pt) override;

    /**
     * Get the texture coordinate at an intersection point.
     * Uses bilinear interpolation based on position within quad.
     * @param  int_pt  Intersection point on the quad surface
     * @return Returns the texture coordinate (s, t) at the intersection point
     */
    Point2 get_texture_coord(const Point3 &int_pt) override;

    /**
     * Ray tracing intersect method - finds closest intersection
     */
    void find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest) override;

    /**
     * Ray tracing intersect method - checks if intersection exists within distance d
     */
    bool does_intersect_exist(Ray3 ray, float d, SceneState &current_state) override;

    /**
     * Quads are convex.
     */
    bool is_convex(void) const;

  private:
    Point3 v0_, v1_, v2_, v3_;  // Corner vertices
    Vector3 normal_;             // Pre-computed normal
    Vector3 edge1_, edge2_;      // Edges for intersection testing

    /**
     * Test ray intersection with the quad (splits into 2 triangles)
     * @param ray  The ray to test
     * @param t    Output: distance to intersection
     * @return true if intersection found
     */
    bool intersect(const Ray3 &ray, float &t) const;
};

} // namespace cg

#endif
