#ifndef __RAY_TRACER_STONE_NORMAL_NODE_HPP__
#define __RAY_TRACER_STONE_NORMAL_NODE_HPP__

#include "RayTracer/normal_map_node.hpp"
#include "geometry/point3.hpp"

namespace cg
{

/**
 * Procedural granite/stone bump shader.
 *
 * Derives a tangent-space normal from a multi-octave fBm height field sampled
 * at the world-space hit position using finite differences.  No texture file
 * needed — each face gets a unique pattern driven by its world-space location.
 *
 * Subclasses NormalMapNode so ray_tracer.cpp uses it without any changes.
 */
class StoneNormalNode : public NormalMapNode
{
public:
    /**
     * @param scale     Spatial frequency of the noise (larger = finer grain)
     * @param octaves   fBm octave count (4–6 gives good granite detail)
     * @param strength  Overall perturbation blend in [0,1]
     */
    explicit StoneNormalNode(float scale    = 4.0f,
                             int   octaves  = 5,
                             float strength = 0.4f);

    Vector3 sample(const Point2 &uv, const Point3 &world_pos, float cam_dist = 0.0f) const override;

private:
    float scale_;
    int   octaves_;

    static float hash(float x, float y, float z);
    static float vnoise(float x, float y, float z);
    float fbm(float x, float y, float z) const;
};

} // namespace cg

#endif
