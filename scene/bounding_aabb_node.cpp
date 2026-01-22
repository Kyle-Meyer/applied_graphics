#include "scene/bounding_aabb_node.hpp"

namespace cg
{

AABBNode::AABBNode() : BoundingNode() {}

AABBNode::~AABBNode() {}

void AABBNode::set(const Point3 &min_pt, const Point3 &max_pt) { box_.update(min_pt, max_pt); }

void AABBNode::draw(SceneState &scene_state)
{
    // TBD - add culling logic!

    // Draw children of this node
    SceneNode::draw(scene_state);
}

void AABBNode::update(SceneState &scene_state)
{
    // Update children of this node
    SceneNode::update(scene_state);
}

void AABBNode::find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest)
{
    // Complete in 605.767 - ray tracing project
}

bool AABBNode::does_intersect_exist(Ray3 ray, float d, SceneState &current_state) { return false; }

} // namespace cg
