//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//
//	File:    height_field_floor_node.hpp
//	Purpose: Ray-traced floor whose surface is a procedural dune height field.
//           Unlike a bump-mapped flat plane, this node ray-marches against the
//           actual displaced surface, so dune silhouettes, cast shadows, and
//           parallax are all geometrically correct.
//============================================================================

#ifndef __RAY_TRACER_HEIGHT_FIELD_FLOOR_NODE_HPP__
#define __RAY_TRACER_HEIGHT_FIELD_FLOOR_NODE_HPP__

#include "geometry/point2.hpp"
#include "geometry/point3.hpp"
#include "geometry/ray3.hpp"
#include "geometry/vector3.hpp"
#include "scene/geometry_node.hpp"
#include "scene/scene_state.hpp"

namespace cg
{

/**
 * Floor geometry node whose surface elevation is a procedural two-octave dune
 * height field (the Dunes() formula layered with domain-warped value noise).
 *
 * Ray intersection is found by adaptive ray marching along the ray until the
 * ray dips below the local surface, followed by a bisection refinement step.
 * All geometry methods (get_normal, get_tangent) derive their values
 * analytically from the same height function via finite differences, so a
 * NormalMapNode (SandBumpNode) can still be applied on top to add micro-ripples.
 *
 * Usage: drop-in replacement for the large-sphere floor trick.
 *   auto floor = std::make_shared<HeightFieldFloorNode>();
 *   floor_mat->add_child(floor_nm);  // SandBumpNode wraps this
 *   floor_nm->add_child(floor);
 */
class HeightFieldFloorNode : public GeometryNode
{
  public:
    /**
     * @param base_y       Y coordinate of the flat base plane (sand level).
     * @param height_scale Multiplier on the raw dune height value.
     *                     1.0 → dune crests reach ~1.45 wu above base_y.
     *                     0.7 → gentler dunes, ~1.0 wu above base_y.
     */
    explicit HeightFieldFloorNode(float base_y = -1.0f, float height_scale = 0.7f);

    /** Traverse — find closest intersection via ray marching. */
    void find_closest_intersect(Ray3 ray, SceneState &current_state,
                                SceneState &closest) override;

    /** Shadow test — return true if height field blocks the ray before distance d. */
    bool does_intersect_exist(Ray3 ray, float d, SceneState &current_state) override;

    /** World-space geometric normal derived from height-field gradients. */
    Vector3 get_normal(const Point3 &hit_pt) override;

    /** Planar UV: (x / uv_scale, z / uv_scale) in [0,1] across the visible scene. */
    Point2 get_texture_coord(const Point3 &hit_pt) override;

    /** Surface tangent dF/dx = (1, dh/dx, 0) for TBN normal-mapping support. */
    Vector3 get_tangent(const Point3 &hit_pt) override;

  private:
    float base_y_;        ///< flat-plane Y
    float height_scale_;  ///< world-unit scale applied to dune_height_at()
    float max_height_;    ///< = height_scale_ * 1.5f  (ceiling of the marching volume)

    /** Evaluate the displaced surface Y at world position (x, z). */
    float surface_y(float x, float z) const;
};

} // namespace cg

#endif
