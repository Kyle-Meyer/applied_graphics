#ifndef __SCENE_LOD_NODE_HPP__
#define __SCENE_LOD_NODE_HPP__

#include "scene/scene_node.hpp"
#include "geometry/point3.hpp"
#include "geometry/ray3.hpp"

#include <vector>

namespace cg
{

/**
 * Level of Detail (LOD) node. Selects which child geometry to render based
 * on the distance from the camera to the object's world-space origin.
 *
 * Levels are added from most detailed (smallest max_distance) to least
 * detailed (largest max_distance). The last level is used as a fallback
 * when the distance exceeds all thresholds.
 *
 * Usage:
 *   auto lod = std::make_shared<LODNode>("my_object");
 *   lod->add_level(20.0f,  high_detail_node);   // distance <= 20
 *   lod->add_level(60.0f,  mid_detail_node);    // distance <= 60
 *   lod->add_level(FLT_MAX, low_detail_node);   // everything beyond
 */
class LODNode : public SceneNode
{
  public:
    struct LODLevel
    {
        float                      max_distance; // Use this level when distance <= max_distance
        std::shared_ptr<SceneNode> node;
        std::string                note;         // Optional label printed when this level is selected
    };

    explicit LODNode(const std::string &name = "LODNode");

    /**
     * Add a detail level. Levels must be added in ascending max_distance order.
     * @param max_distance  Distance threshold; this level is chosen when the
     *                      camera is closer than max_distance.
     * @param node          Geometry (or subtree) to draw at this level.
     * @param note          Optional label printed when this level is selected.
     */
    void add_level(float max_distance, std::shared_ptr<SceneNode> node, const std::string &note = "");

    /**
     * Set the world-space reference position used for ray-tracing LOD distance.
     * Should match the object's world-space position after transforms.
     */
    void set_position(const Point3 &pos);

    /**
     * Select and draw the appropriate detail level based on camera distance.
     */
    void draw(SceneState &scene_state) override;

    /**
     * Ray tracing: select LOD level by distance from ray origin to position_,
     * then delegate intersection test to that level's geometry.
     */
    void find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest) override;

    /**
     * Ray tracing shadow test: same LOD selection, early-exit version.
     */
    bool does_intersect_exist(Ray3 ray, float d, SceneState &current_state) override;

    /**
     * Reset the per-render print throttle. Call once at the start of each render.
     */
    static void reset_rt_print_count();

  private:
    std::vector<LODLevel> levels_;
    Point3                position_;  // World-space reference for RT distance

    // Compute distance from camera to this node's world-space origin (draw path).
    float compute_distance(const SceneState &state) const;

    // Return the index into levels_ appropriate for the given distance.
    int select_level(float distance) const;
};

} // namespace cg

#endif
