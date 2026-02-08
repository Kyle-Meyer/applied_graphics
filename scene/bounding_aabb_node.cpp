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
    // Test ray against bounding box - if miss, skip all children
    RayObjectIntersectResult result = ray.intersect(box_);
    if (!result.intersects)
    {
        return;  // Ray misses bounding box, skip entire subtree
    }

    // Ray hits bounding box - traverse children to find actual intersections
    for (auto &child : children_)
    {
        child->find_closest_intersect(ray, current_state, closest);
    }
}

bool AABBNode::does_intersect_exist(Ray3 ray, float d, SceneState &current_state)
{
    // Test ray against bounding box
    RayObjectIntersectResult result = ray.intersect(box_);

    // If ray misses box, or box is beyond the distance we care about, skip children
    if (!result.intersects || result.distance > d)
    {
        return false;
    }

    // Ray hits bounding box within distance - check children
    for (auto &child : children_)
    {
        if (child->does_intersect_exist(ray, d, current_state))
        {
            return true;  // Found an intersection, early exit
        }
    }

    return false;
}

} // namespace cg
