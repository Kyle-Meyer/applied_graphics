//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    texture_node.hpp
//	Purpose: Scene graph texture presentation node. Reads an image file and
//           binds the texture. Supports changing texture properties.
//
//============================================================================

#ifndef __RAY_TRACER_TEXTURE_NODE_HPP__
#define __RAY_TRACER_TEXTURE_NODE_HPP__

#include "scene/color3.hpp"
#include "scene/scene_node.hpp"

namespace cg
{

/**
 * Texture presentation node.
 */
class TextureNode : public SceneNode
{
  public:
    /**
     * Constructor. Builds an individual texture with the specified
     * from an image file specified by filename.
     */
    TextureNode(char  *filename,
                GLuint mode,
                GLuint s_wrap,
                GLuint t_wrap,
                GLuint min_filter,
                GLuint mag_filter);

    /**
     * Draw. Not used for ray tracing.
     */
    void draw(SceneState &scene_state) override;

    /**
     * Ray tracing intersect method. Finds the closest intersection (sets
     * properties in SceneState.
     * @param ray  Ray being traced.
     * @param current_state Current scene state information.
     * @param closest Scene state carrying information about closest intersection.
     */
    void find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest) override;

    // All procedural textures must contain the GetColor method
    virtual Color3 get_color(const Point3 &v) = 0;

    // Get a color given a texture coordinate
    virtual Color3 get_color(const TextureCoord2 &tex_coord);

  protected:
    bool   updated_;    // Has this texture been updated?
    GLuint texture_id_; // Texture ID

    // Force direct users of TextureNode to use public constructor
    TextureNode();
};

} // namespace cg

#endif
