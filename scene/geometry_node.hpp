//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:	 David W. Nesbitt
//	File:    geometry_node.hpp
//	Purpose: Scene graph geometry node.
//
//============================================================================

#ifndef __SCENE_GEOMETRY_NODE_HPP__
#define __SCENE_GEOMETRY_NODE_HPP__

#include "scene/scene_node.hpp"

namespace cg
{

/**
 * Geometry node base class. Stores and draws geometry.
 */
class GeometryNode : public SceneNode
{
  public:
    /**
     * Constructor
     */
    GeometryNode();

    /**
     * Destructor
     */
    virtual ~GeometryNode();

    /**
     * Draw this geometry node. Geometry nodes are leaf nodes and have no children.
     * @param  scene_state  Current scene state
     */
    virtual void draw(SceneState &scene_state) override;

    /**
     * Get the surface normal at a point on the geometry.
     * Override this method in ray tracing geometry nodes.
     * @param  int_pt  Intersection point on the surface
     * @return Returns the unit length surface normal at the intersection point
     */
    virtual Vector3 get_normal(const Point3 &int_pt);

    /**
     * Get the texture coordinate at an intersection point.
     * For spheres, use spherical mapping. For meshes, interpolate using barycentric coordinates.
     * Override this method in ray tracing geometry nodes.
     * @param  int_pt  Intersection point on the surface
     * @return Returns the texture coordinate (s, t) at the intersection point
     */
    virtual Point2 get_texture_coord(const Point3 &int_pt);
};

} // namespace cg

#endif
