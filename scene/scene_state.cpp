#include "scene/scene_state.hpp"

namespace cg
{

void SceneState::init()
{
    max_enabled_light = 0;
    model_matrix.set_identity();
    model_matrix_stack.clear();
}

void SceneState::push_transforms() { model_matrix_stack.push_back(model_matrix); }

void SceneState::pop_transforms()
{
    // If there are any matrices on the stack, retrieve the last one and
    // remove it from the stack
    if(model_matrix_stack.size() > 0)
    {
        model_matrix = model_matrix_stack.back();
        model_matrix_stack.pop_back();
    }
    else model_matrix.set_identity();
}

void SceneState::push_material() { material_stack.push_back(material_node); }

void SceneState::pop_material()
{
    material_stack.pop_back();
    material_node = (material_stack.size() > 0) ? material_stack.back() : nullptr;
}

void SceneState::push_texture() { texture_stack.push_back(texture_node); }

void SceneState::pop_texture()
{
    texture_stack.pop_back();
    texture_node = (texture_stack.size() > 0) ? texture_stack.back() : nullptr;
}

} // namespace cg
