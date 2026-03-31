#include "scene/bounding_sphere_node.hpp"

#include <iostream>

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
    if (!scene_state.fully_inside_frustum)
    {
        FrustumIntersectType result = scene_state.frustum.intersect(bounding_sphere_);
        if (result == FrustumIntersectType::OUTSIDE)
        {
            std::cout << "CULLED (Sphere): " << name_ << std::endl;
            return;
        }
        if (result == FrustumIntersectType::INSIDE)
        {
            // Entire subtree is inside — descendants can skip frustum tests
            scene_state.fully_inside_frustum = true;
            SceneNode::draw(scene_state);
            scene_state.fully_inside_frustum = false;
            return;
        }
    }

    // INTERSECT (or already fully inside): draw children, which will test their own BVs
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
