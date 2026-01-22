#include "RayTracer/material_node.hpp"

namespace cg
{

MaterialNode::MaterialNode()
{
    node_type_ = SceneNodeType::PRESENTATION;
    shininess_ = 1.0f;
    reflective_ = false;
    transparent_ = false;
    index_of_refraction_ = 1.0f;

    // Note: color constructors default rgb to 0 and alpha to 1
}

void MaterialNode::set_ambient(const Color4 &c) { ambient_ = c; }

const Color4 &MaterialNode::get_ambient() const { return ambient_; }

void MaterialNode::set_diffuse(const Color4 &c) { diffuse_ = c; }

const Color4 &MaterialNode::get_diffuse() const { return diffuse_; }

void MaterialNode::set_ambient_and_diffuse(const Color4 &c)
{
    ambient_ = c;
    diffuse_ = c;
}

void MaterialNode::set_specular(const Color4 &c) { specular_ = c; }

const Color4 &MaterialNode::get_specular() const { return specular_; }

void MaterialNode::set_emission(const Color4 &c) { emission_ = c; }

const Color4 &MaterialNode::get_emission() const { return emission_; }

void MaterialNode::set_shininess(float s) { shininess_ = s; }

float MaterialNode::get_shininess() const { return shininess_; }

void MaterialNode::set_global_reflectivity(float r, float g, float b)
{
    global_reflection_.set(r, g, b);
    reflective_ = (r > 0.0f || g > 0.0f || b > 0.0f);
}

const Color3 &MaterialNode::get_global_reflectivity() const { return global_reflection_; }

void MaterialNode::set_global_transmission(float r, float g, float b)
{
    global_refraction_.set(r, g, b);
    transparent_ = (r > 0.0f || g > 0.0f || b > 0.0f);
}

const Color3 &MaterialNode::get_global_transmission(void) const { return global_refraction_; }

void MaterialNode::set_index_of_refraction(float r) { index_of_refraction_ = r; }

float MaterialNode::get_index_of_refraction() const { return index_of_refraction_; }

bool MaterialNode::is_reflective() const { return reflective_; }

bool MaterialNode::is_transparent() const { return transparent_; }

void MaterialNode::draw(SceneState &scene_state)
{
    // Not useful for regular OpenGL drawing...

    // Draw children of this node
    SceneNode::draw(scene_state);
}

void MaterialNode::find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest)
{
    // Push material onto stack within the current state
    current_state.push_material();

    // Set this as the current material
    current_state.material_node = this;

    // Loop through the list and check intersections with children
    for(auto c : children_) { c->find_closest_intersect(ray, current_state, closest); }

    // Revert back to prior material state
    current_state.pop_material();
}

} // namespace cg
