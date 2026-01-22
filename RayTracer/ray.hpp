//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    ray.hpp
//	Purpose: Ray for ray tracing applications. Extends Ray3 to provide
//           additional information required for ray tracing.
//
//============================================================================

#ifndef __RAYT_RACER_RAY_HPP__
#define __RAYT_RACER_RAY_HPP__

#include "RayTracer/material_node.hpp"
#include "geometry/point3.hpp"
#include "geometry/ray3.hpp"
#include "geometry/vector3.hpp"

namespace cg
{

/**
 * Ray class. Extends the geometric ray definition to add information
 * needed by RayTracer.
 */
class Ray : public Ray3
{
  public:
    int32_t recursion_level_;
    float   threshold_;

    /**
     * Construct a ray. Provide origin and direction (Ray3) as well
     * as the depth in the tree.
     * @param initial_ray  Initial ray from camera through the image plane.
     * @param depth       Maximum depth (recursions) to trace the ray.
     * @param t           Threshold (attenuation where ray tracing stops)
     */
    Ray(Ray3 &initial_ray, int32_t depth, float t);

    /**
     * Get the refracted ray given the intersection point, normal, and material.
     * Checks if total internal reflection occurs.
     * @param int_pt  Intersection point where refracted ray begins.
     * @param normal  Normal to the surface at the intersection point.
     * @param mat_node      Material that is intersected.
     * @param total_internal_reflection (OUT) - true if ray is internally reflected
     *                and false if not.
     * @return Returns the refracted ray.
     */
    Ray get_refracted_ray(const Point3       &int_pt,
                          const Vector3      &normal,
                          const MaterialNode *mat_node,
                          bool               &total_internal_reflection);

    /**
     * Check if below the adaptive depth testing threshold.
     * @return  Returns true if below the adaptive depth test threshold.
     */
    bool below_threshold();
};

} // namespace cg

#endif
