//============================================================================
//	Johns Hopkins University Whiting School of Engineering
//	605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:	 David W. Nesbitt
//	File:    ray_tracer.hpp
//	Purpose: Recursive ray tracer.
//
//============================================================================

#ifndef __RAY_TRACER_RAY_TRACER_HPP__
#define __RAY_TRACER_RAY_TRACER_HPP__

#include "RayTracer/lighting.hpp"
#include "RayTracer/procedural_texture.hpp"
#include "RayTracer/ray.hpp"
#include "scene/geometry_node.hpp"
#include "scene/light_node.hpp"

#include <vector>

namespace cg
{

/**
 * Ray tracer class. Performs recursive ray tracing.
 */
class RayTracer
{
  public:
    /**
     * Constructor.
     */
    RayTracer(std::shared_ptr<SceneNode> scene_root);

    ~RayTracer();

    /**
     * Initial call to trace a ray.
     */
    Color3 trace_ray(Ray3 &initial_ray, int depth, float adaptive_threshold);

    /**
     * Recursive method to trace a ray
     */
    Color3 trace_ray(Ray &ray);

    /**
     * Set the view position (for lighting).
     */
    void set_view_position(const Point3 &pos);

    /**
     * Add a light to the ray tracer.
     * @param light  Pointer to the light node
     */
    void add_light(LightNode *light);

  private:
    Lighting                   lighting_;
    std::shared_ptr<SceneNode> scene_root_;
    std::vector<LightNode *>   lights_;

    /**
     * Tests if the intersect point is in shadow with respect to the
     * specified light position.
     * @param   int_pt       Intersection point
     * @param   light_pos    Light position
     * @param   current_obj  Current object. If this object is convex we
     *                      can avoid a shadow computation.
     * @return  Returns true if there is an occluding object between
     *          the light and the intersect point, false if not.
     **/
    bool in_shadow(const Point3 &int_pt, Point3 &light_pos, SceneNode *current_obj);
};

} // namespace cg

#endif
