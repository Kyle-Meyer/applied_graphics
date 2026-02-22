#include "scene/lod_node.hpp"

#include <cmath>
#include <iostream>

namespace cg
{

LODNode::LODNode(const std::string &name)
{
    name_ = name;
}

void LODNode::add_level(float max_distance, std::shared_ptr<SceneNode> node)
{
    levels_.push_back({max_distance, node});
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

} // namespace cg
