#include "RayTracer/procedural_texture.hpp"

namespace cg
{

ProceduralTexture::ProceduralTexture() { node_type_ = SceneNodeType::PROCEDURAL_TEXTURE; }

void ProceduralTexture::find_closest_intersect(Ray3 ray, SceneState& current_state, SceneState& closest)
{
    // Save the previous texture
    SceneNode* prev_texture = current_state.texture_node;

    // Set this as the current texture
    current_state.texture_node = this;

    // Loop through children and check intersections
    for (auto& child : children_)
    {
        child->find_closest_intersect(ray, current_state, closest);
    }

    // Restore previous texture
    current_state.texture_node = prev_texture;
}

} // namespace cg
