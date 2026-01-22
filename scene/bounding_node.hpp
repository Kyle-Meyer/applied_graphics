//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    bounding_node.hpp
//	Purpose: Scene graph bounding volume node.
//
//============================================================================

#ifndef __SCENE_BOUNDING_NODE_HPP__
#define __SCENE_BOUNDING_NODE_HPP__

#include "scene/scene_node.hpp"

namespace cg
{

/**
 * Bounding node base class. Derived classes have bounding volumes and perform
 * view frustum culling.
 */
class BoundingNode : public SceneNode
{
  public:
    /**
     * Constructor
     */
    BoundingNode();

    /**
     * Destructor
     */
    virtual ~BoundingNode();

    /**
     * Draw this bounding volume node and its children. Derived classes will
     * generally support view frustum culling and occlusion culling.
     *
     */
    virtual void draw(SceneState &scene_state) = 0;

    /**
     * Update this bounding volume node and its children.
     */
    virtual void update(SceneState &scene_state) = 0;

  protected:
};

} // namespace cg

#endif
