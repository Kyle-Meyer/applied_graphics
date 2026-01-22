//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:	 David W. Nesbitt
//	File:    scene_node.hpp
//	Purpose: Scene graph node. All scene node classes inherit from this
//           base class.
//
//============================================================================

#ifndef __SCENE_SCENE_NODE_HPP__
#define __SCENE_SCENE_NODE_HPP__

#include "scene/graphics.hpp"
#include "scene/scene_state.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace cg
{

enum class SceneNodeType
{
    BASE,
    PRESENTATION,       // Generic presentation node
    SHADER,             // GLSL Shader node
    TRANSFORM,          // Transform node
    GEOMETRY,           // Geometry node
    LIGHT,              // Light source
    CAMERA,             // Camera
    BOUNDING,           // Bounding volume node
    PROCEDURAL_TEXTURE, // Procedural texture
    IMAGE_TEXTURE       // Image texture
};

std::ostream &operator<<(std::ostream &out, const SceneNodeType &type);

/**
 * Scene graph node: base class
 */
class SceneNode
{
  public:
    /**
     * Constructor. Set the reference count to 0.
     */
    SceneNode();

    /**
     * Destructor
     */
    virtual ~SceneNode();

    /**
     * Draw the scene node and its children. The base class just draws the
     * children. Derived classes can use this (SceneNode::draw()) to draw
     * all children without having to duplicate this code.
     * @param  scene_state  Current scene state
     */
    virtual void draw(SceneState &scene_state);

    /**
     * Update the scene node and its children
     * @param  scene_state  Current scene state
     */
    virtual void update(SceneState &scene_state);

    /**
     * Destroy all the children
     */
    void destroy();

    /**
     * Add a child to this node. Increment the reference count of the child.
     * @param  node  Add a child node to this scene node.
     */
    void add_child(std::shared_ptr<SceneNode> node);

    /**
     * Get the type of scene node
     * @return  Returns the type of hte scene node.
     */
    SceneNodeType node_type() const;

    /**
     * Set the name for this scene node.
     * @param  nm  Name for this scene node.
     */
    void set_name(const char *nm);

    /**
     * Get the name for this scene node.
     * @return  Returns the name of this scene node.
     */
    const std::string &get_name() const;

    /**
     * Intersection method (for picking or ray tracing)
     * @param  ray   Ray to intersect with geometry. Note that this is not
     *               passed by reference, as transformation to the ray can
     *               occur at certain levels of the tree
     * @param  current_state Current state, updated as the scene is traversed
     * @param  closest       Closest object information
     */
    virtual void find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest);

    /**
     * Intersection method - checks if an object in the scene graph
     * intersects the ray along the specified distance. Can return true if any
     * object intersects. (Used for shadow rays in ray tracing).
     * @param  ray  Ray to test intersections.
     * @param  d    Maximum distance - intersections must be closer than d.
     * @param  current_state Current state (transformations, etc.)
     * @return  Returns true if an intersection is found closer than distance d, false
     *          if no intersections occur.
     */
    virtual bool does_intersect_exist(Ray3 ray, float d, SceneState &current_state);

    void print_graph(std::ostream &out = std::cout, int32_t level = 0) const;

  protected:
    std::string                             name_;
    SceneNodeType                           node_type_;
    std::vector<std::shared_ptr<SceneNode>> children_;
};

/**
 * Convenience method to add a chain of children to a parent.
 * @param  parent   Parent scene node.
 * @param  child_1  Direct child of parent.
 * @param  child_2  Direct child of child_1.
 */
void add_sub_tree(std::shared_ptr<cg::SceneNode> parent,
                  std::shared_ptr<cg::SceneNode> child_1,
                  std::shared_ptr<cg::SceneNode> child_2);

/**
 * Convenience method to add a chain of children to a parent.
 * For example, to add a material, then a transform, then a
 * geometry node as a child to a specified parent node.
 * @param  parent   Parent scene node.
 * @param  child_1  Direct child of parent (e.g. Presentation node).
 * @param  child_2  Direct child of child_1 (e.g. Transformation node).
 * @param  child_3  Direct child of child_2 (e.g. Geometry node).
 */
void add_sub_tree(std::shared_ptr<cg::SceneNode> parent,
                  std::shared_ptr<cg::SceneNode> child_1,
                  std::shared_ptr<cg::SceneNode> child_2,
                  std::shared_ptr<cg::SceneNode> child_3);

} // namespace cg

#endif
