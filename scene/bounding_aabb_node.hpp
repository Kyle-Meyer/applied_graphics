//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    bounding_aabb_node.hpp
//	Purpose: Scene graph axis aligned bounding box node.
//
//============================================================================

#ifndef __SCENE_BOUNDING_AABB_NODE_HPP__
#define __SCENE_BOUNDING_AABB_NODE_HPP__

#include "geometry/aabb.hpp"
#include "geometry/point3.hpp"
#include "geometry/ray3.hpp"
#include "scene/bounding_node.hpp"

namespace cg
{

/**
 * AABB bounding volume node.
 */
class AABBNode : public BoundingNode
{
  public:
    /**
     * Constructor
     */
    AABBNode();

    /**
     * Destructor
     */
    ~AABBNode();

    /**
     * Sets the bounding box
     * @param   min_pt  Minimum point
     * @param   max_pt  Maximum point
     */
    void set(const Point3 &min_pt, const Point3 &max_pt);

    /**
     * Draw this bounding volume node and its children.
     */
    void draw(SceneState &scene_state) override;

    /**
     * Update this bounding volume node and its children.
     */
    void update(SceneState &scene_state) override;

    void find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest) override;

    bool does_intersect_exist(Ray3 ray, float d, SceneState &current_state) override;

  protected:
    AABB box_;
};

} // namespace cg

#endif
