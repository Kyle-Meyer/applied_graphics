//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//
//	File:    sand_bump_node.hpp
//	Purpose: Procedural sand bump shader — computes wind-ripple + grain
//           normals analytically from UV coordinates; no texture file needed.
//============================================================================

#ifndef __RAY_TRACER_SAND_BUMP_NODE_HPP__
#define __RAY_TRACER_SAND_BUMP_NODE_HPP__

#include "RayTracer/normal_map_node.hpp"
#include "geometry/point3.hpp"

namespace cg
{

/**
 * Procedural sand surface shader.
 *
 * Computes a tangent-space normal at each UV point by:
 *   1. Primary aeolian ripples  — low-frequency sine wave running along U
 *   2. Secondary crossing ripples — offset angle and scale for depth
 *   3. Fine grain noise overlay  — hash-based high-frequency perturbation
 *
 * Subclasses NormalMapNode so ray_tracer.cpp can use it without changes.
 */
class SandBumpNode : public NormalMapNode
{
public:
    /**
     * @param ripple_freq  Spatial frequency of primary wind ripples (cycles/UV unit)
     * @param ripple_amp   Slope amplitude of ripples (higher = deeper ridges)
     * @param grain_freq   Frequency of fine grain noise overlay
     * @param grain_amp    Amplitude of fine grain noise
     * @param strength     Overall perturbation blend in [0,1]
     */
    SandBumpNode(float ripple_freq = 0.30f,
                 float ripple_amp  = 0.10f,
                 float grain_freq  = 2.5f,
                 float grain_amp   = 0.23f,
                 float strength    = 0.40f);

    /**
     * Compute tangent-space normal from world-space position (ignores UV).
     * Using world XZ avoids the pole-compression artifact of spherical UV mapping.
     */
    Vector3 sample(const Point2 &uv, const Point3 &world_pos, float cam_dist = 0.0f) const override;

private:
    float ripple_freq_;
    float ripple_amp_;
    float grain_freq_;
    float grain_amp_;
};

} // namespace cg

#endif
