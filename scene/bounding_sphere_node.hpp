//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    bounding_sphere_node.hpp
//	Purpose: Scene graph bounding sphere node.
//
//============================================================================

#ifndef __SCENE_BOUNDING_SPHERE_NODE_HPP__
#define __SCENE_BOUNDING_SPHERE_NODE_HPP__

#include "geometry/bounding_sphere.hpp"
#include "scene/bounding_node.hpp"

namespace cg
{

/**
 * Bounding sphere node.
 */
class BoundingSphereNode : public BoundingNode
{
  public:
    /**
     * Constructor
     */
    BoundingSphereNode();

    /**
     * Destructor
     */
    virtual ~BoundingSphereNode();

    /**
     * Sets the bounding sphere
     * @param  sphere   Bounding sphere
     */
    void set_bounding_sphere(const BoundingSphere &sphere);

    /**
     * Merges the incoming bounding sphere with the current bounding sphere.
     * @param  sphere   Bounding sphere
     */
    void merge_bounding_sphere(const BoundingSphere &sphere);

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
    BoundingSphere bounding_sphere_;
};

} // namespace cg

#endif
