#include "RayTracer/texture_node.hpp"

#include "scene/image_data.hpp"

#include <iostream>

namespace cg
{

TextureNode::TextureNode() {}

TextureNode::TextureNode(
    char *filename, GLuint mode, GLuint s_wrap, GLuint t_wrap, GLuint min_filter, GLuint mag_filter)
{
    updated_ = true;
    texture_id_ = 0;
    node_type_ = SceneNodeType::PRESENTATION;

    ImageData im_data;
    load_image_data(im_data, filename);
    if(im_data.data == nullptr)
    {
        std::cout << "Error loading texture " << filename << '\n';
        return;
    }
}

void TextureNode::draw(SceneState &scene_state)
{
    // Draw children of this node
    SceneNode::draw(scene_state);
}

void TextureNode::find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest)
{
    // Push texture onto stack within the current state
    current_state.push_texture();

    // Set this as the current texture
    current_state.texture_node = this;

    // Loop through the list and check intersections with children
    for(auto c : children_) { c->find_closest_intersect(ray, current_state, closest); }

    // Revert back to prior texture state
    current_state.pop_texture();
}

Color3 TextureNode::get_color(const TextureCoord2 &tex_coord) { return Color3(0.0f, 0.0f, 0.0f); }

} // namespace cg
