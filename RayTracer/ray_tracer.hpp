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

    /**
     * Enable or disable soft shadows (area light disc sampling).
     * When enabled, N shadow rays are cast toward random points on the moon disc.
     * When disabled, a single ray is cast toward the disc centre (hard shadows).
     * @param  enable  True for soft shadows, false for hard.
     */
    void set_soft_shadows(bool enable);

  private:
    Lighting                   lighting_;
    std::shared_ptr<SceneNode> scene_root_;
    std::vector<LightNode *>   lights_;

    bool  soft_shadows_enabled_;
    int   shadow_samples_;   // N rays per hit point when soft shadows are on
    float disc_radius_;      // radius of the moon disc (world units)

    /**
     * Computes the fraction of the light disc visible from int_pt (0 = fully
     * shadowed, 1 = fully lit).  With soft_shadows_enabled_ = false this
     * degenerates to a binary 0/1 hard shadow test.
     * @param   int_pt       Intersection point
     * @param   light_pos    Light position (or directional-light anchor point)
     * @param   current_obj  Current object, used to skip self-intersection.
     * @return  Shadow factor in [0, 1].
     */
    float shadow_factor(const Point3 &int_pt, Point3 &light_pos, SceneNode *current_obj);
};

} // namespace cg

#endif
