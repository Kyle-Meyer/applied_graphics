#include "scene/lod_node.hpp"

#include <atomic>
#include <cmath>
#include <iostream>

namespace cg
{

namespace
{
std::atomic<int> s_rt_lod_prints{0};
constexpr int    MAX_RT_LOD_PRINTS = 10;
} // namespace

LODNode::LODNode(const std::string &name)
{
    name_ = name;
}

void LODNode::add_level(float max_distance, std::shared_ptr<SceneNode> node)
{
    levels_.push_back({max_distance, node});
}

void LODNode::set_position(const Point3 &pos)
{
    position_ = pos;
}

void LODNode::reset_rt_print_count()
{
    s_rt_lod_prints.store(0, std::memory_order_relaxed);
}

float LODNode::compute_distance(const SceneState &state) const
{
    // Extract the world-space origin of this node from the accumulated model matrix.
    // The translation column (col 3) gives the object's world position.
    Point3 world_pos(
        state.model_matrix.m03(),
        state.model_matrix.m13(),
        state.model_matrix.m23());

    Vector3 delta(world_pos, state.camera_position);
    return delta.norm();
}

int LODNode::select_level(float distance) const
{
    for (int i = 0; i < static_cast<int>(levels_.size()); ++i)
    {
        if (distance <= levels_[i].max_distance)
        {
            return i;
        }
    }
    // Beyond all thresholds — use the lowest-detail level
    return static_cast<int>(levels_.size()) - 1;
}

void LODNode::draw(SceneState &scene_state)
{
    if (levels_.empty())
    {
        return;
    }

    float distance = compute_distance(scene_state);
    int   level    = select_level(distance);

    std::cout << "LOD: " << name_
              << " level=" << level
              << " distance=" << distance << std::endl;

    levels_[level].node->draw(scene_state);
}

void LODNode::find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest)
{
    if (levels_.empty())
        return;

    // Use the ray's origin as the camera position for LOD distance.
    // For primary rays this is the exact camera position; shadow rays will
    // use a slightly different origin but the level selection stays consistent.
    Vector3 delta(ray.o, position_);
    float   distance = delta.norm();
    int     level    = select_level(distance);

    int n = s_rt_lod_prints.fetch_add(1, std::memory_order_relaxed);
    if (n < MAX_RT_LOD_PRINTS)
    {
        std::cout << "LOD RT: " << name_
                  << "  level=" << level
                  << "  dist=" << distance
                  << std::endl;
    }
    else if (n == MAX_RT_LOD_PRINTS)
    {
        std::cout << "(LOD RT print limit reached - suppressing further output)\n";
    }

    levels_[level].node->find_closest_intersect(ray, current_state, closest);
}

bool LODNode::does_intersect_exist(Ray3 ray, float d, SceneState &current_state)
{
    if (levels_.empty())
        return false;

    Vector3 delta(ray.o, position_);
    float   distance = delta.norm();
    int     level    = select_level(distance);

    return levels_[level].node->does_intersect_exist(ray, d, current_state);
}

} // namespace cg
