//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//
//	File:    rt_transform_node.hpp
//	Purpose: Transform node for ray tracing with inverse matrix support.
//============================================================================

#ifndef __RAY_TRACER_RT_TRANSFORM_NODE_HPP__
#define __RAY_TRACER_RT_TRANSFORM_NODE_HPP__

#include "scene/transform_node.hpp"

namespace cg
{

/**
 * Transform node for ray tracing.
 * Transforms rays to object space before intersection tests,
 * and stores the inverse/normal matrices for transforming results back.
 * Caches inverse and normal matrices for performance.
 */
class RTTransformNode : public TransformNode
{
  public:
    /**
     * Constructor.
     */
    RTTransformNode();

    /**
     * Destructor.
     */
    virtual ~RTTransformNode();

    /**
     * Precompute and cache the inverse and normal matrices.
     * Call this after all transforms have been applied and before ray tracing.
     */
    void prepare_for_ray_tracing();

    /**
     * Find closest intersection. Transforms the ray to object space,
     * tests children, and stores inverse matrices for later normal transformation.
     * @param ray            The ray (in world space)
     * @param current_state  Current traversal state
     * @param closest        State for closest intersection found
     */
    void find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest) override;

    /**
     * Check if an intersection exists within distance d.
     * Transforms the ray to object space before testing children.
     * @param ray            The ray (in world space)
     * @param d              Maximum distance to check
     * @param current_state  Current traversal state
     * @return Returns true if an intersection exists within distance d
     */
    bool does_intersect_exist(Ray3 ray, float d, SceneState &current_state) override;

  private:
    Matrix4x4 inverse_matrix_;       // Cached inverse of model_matrix_
    Matrix4x4 normal_matrix_;        // Cached transpose of inverse (for normal transform)
    mutable bool matrices_cached_;   // Whether matrices have been computed

    /**
     * Ensure cached matrices are up to date.
     */
    void ensure_matrices_cached() const;
};

} // namespace cg

#endif
