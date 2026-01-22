#include "scene/bounding_sphere_node.hpp"

namespace cg
{

BoundingSphereNode::BoundingSphereNode() : BoundingNode() {}

BoundingSphereNode::~BoundingSphereNode() {}

void BoundingSphereNode::set_bounding_sphere(const BoundingSphere &sphere)
{
    bounding_sphere_ = sphere;
}

void BoundingSphereNode::merge_bounding_sphere(const BoundingSphere &sphere)
{
    bounding_sphere_.merge_with(sphere);
}

void BoundingSphereNode::draw(SceneState &scene_state)
{
    // TBD - culling

    // Draw children of this node
    SceneNode::draw(scene_state);
}

void BoundingSphereNode::update(SceneState &scene_state)
{
    // Update children of this node
    SceneNode::update(scene_state);
}

void BoundingSphereNode::find_closest_intersect(Ray3        ray,
                                                SceneState &current_state,
                                                SceneState &closest)
{
    // Complete in 605.767 - ray tracing project
}

bool BoundingSphereNode::does_intersect_exist(Ray3 ray, float d, SceneState &current_state)
{
    return false;
}

} // namespace cg
