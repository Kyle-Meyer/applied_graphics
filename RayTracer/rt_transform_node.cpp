#include "RayTracer/rt_transform_node.hpp"

namespace cg
{

RTTransformNode::RTTransformNode() : TransformNode(), matrices_cached_(false) {}

RTTransformNode::~RTTransformNode() {}

void RTTransformNode::prepare_for_ray_tracing()
{
    ensure_matrices_cached();
}

void RTTransformNode::ensure_matrices_cached() const
{
    if (!matrices_cached_)
    {
        // Cast away const for lazy initialization (mutable pattern)
        RTTransformNode* self = const_cast<RTTransformNode*>(this);
        self->inverse_matrix_ = model_matrix_.get_inverse();
        self->normal_matrix_ = self->inverse_matrix_.get_transpose();
        self->matrices_cached_ = true;
    }
}

void RTTransformNode::find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest)
{
    // Ensure cached matrices are ready
    ensure_matrices_cached();

    // Transform the ray to object space using cached inverse
    Ray3 object_ray = inverse_matrix_ * ray;

    // Push current inverse matrix state
    current_state.inverse_matrix_stack.push_back(current_state.inverse_matrix);
    current_state.normal_transform_stack.push_back(current_state.normal_matrix);

    // Accumulate inverse matrices (right-multiply since we're going backwards)
    current_state.inverse_matrix = inverse_matrix_ * current_state.inverse_matrix;

    // Accumulate normal matrix
    current_state.normal_matrix = normal_matrix_ * current_state.normal_matrix;

    // Mark that transforms are required
    bool was_transform_required = current_state.transform_required;
    current_state.transform_required = true;

    // Test children with the transformed ray
    for (auto &child : children_)
    {
        child->find_closest_intersect(object_ray, current_state, closest);
    }

    // Pop inverse matrix state
    current_state.inverse_matrix = current_state.inverse_matrix_stack.back();
    current_state.inverse_matrix_stack.pop_back();
    current_state.normal_matrix = current_state.normal_transform_stack.back();
    current_state.normal_transform_stack.pop_back();
    current_state.transform_required = was_transform_required;
}

bool RTTransformNode::does_intersect_exist(Ray3 ray, float d, SceneState &current_state)
{
    // Ensure cached matrices are ready
    ensure_matrices_cached();

    // Transform the ray to object space using cached inverse
    Ray3 object_ray = inverse_matrix_ * ray;

    // The ray parameter t is preserved through the transform, so we use the same
    // distance value. This is because:
    // P_world = O_world + t * D_world
    // P_object = M^-1 * P_world = O_object + t * D_object
    // The t value is the same in both spaces.

    // Test children with the transformed ray
    for (auto &child : children_)
    {
        if (child->does_intersect_exist(object_ray, d, current_state))
        {
            return true;
        }
    }

    return false;
}

} // namespace cg
