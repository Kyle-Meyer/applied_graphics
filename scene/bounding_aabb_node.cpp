#include "scene/bounding_aabb_node.hpp"

#include <atomic>
#include <iostream>

namespace cg
{

namespace
{
std::atomic<int> s_rt_cull_prints{0};
constexpr int    MAX_RT_CULL_PRINTS = 10;
} // namespace

AABBNode::AABBNode() : BoundingNode() {}

AABBNode::~AABBNode() {}

void AABBNode::set(const Point3 &min_pt, const Point3 &max_pt) { box_.update(min_pt, max_pt); }

void AABBNode::reset_rt_print_count()
{
    s_rt_cull_prints.store(0, std::memory_order_relaxed);
}

void AABBNode::draw(SceneState &scene_state)
{
    if (!scene_state.fully_inside_frustum)
    {
        FrustumIntersectType result = scene_state.frustum.intersect(box_);
        if (result == FrustumIntersectType::OUTSIDE)
        {
            std::cout << "CULLED (AABB): " << name_ << std::endl;
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

void AABBNode::update(SceneState &scene_state)
{
    // Update children of this node
    SceneNode::update(scene_state);
}

void AABBNode::find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest)
{
    // Test ray against bounding box - if miss, skip all children
    RayObjectIntersectResult result = ray.intersect(box_);
    if (!result.intersects || result.distance > closest.t_min)
    {
        if (!result.intersects)
        {
            // Ray completely missed this bounding volume - print culling event (throttled)
            int n = s_rt_cull_prints.fetch_add(1, std::memory_order_relaxed);
            if (n < MAX_RT_CULL_PRINTS)
            {
                std::cout << "RT CULLED (AABB): " << name_ << std::endl;
            }
            else if (n == MAX_RT_CULL_PRINTS)
            {
                std::cout << "(AABB RT cull print limit reached - suppressing further output)\n";
            }
        }
        return;
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
