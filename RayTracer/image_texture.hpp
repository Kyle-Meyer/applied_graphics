//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    image_texture.hpp
//	Purpose: Derived class for defining image based textures
//
//============================================================================

#ifndef __RAY_TRACER_IMAGE_TEXTURE_HPP__
#define __RAY_TRACER_IMAGE_TEXTURE_HPP__

#include "RayTracer/texture_node.hpp"

#include "scene/image_data.hpp"

namespace cg
{

/**
 * Image texture for use in ray tracing
 */
class ImageTexture : public TextureNode
{
  public:
    /**
     * Constructor. Loads the image from the specified file.
     * @param  filename  Image file to load.
     */
    ImageTexture(const char *filename);

    /**
     * Destructor. Deletes the pixmap memory.
     */
    ~ImageTexture();

    /**
     * Draw. May want to set texture properties so we can draw using OpenGL.
     */
    void draw(SceneState &scene_state) override;

    /**
     * Dummy function (only used by procedural textures)
     */
    Color3 get_color(const Point3 &) override;

    /**
     * Get the color based given the texture coordinates. Uses linear filtering
     * among the 4 nearest texels.
     * NOTE: does not currently support wrap and will cause issues if texture coords
     * are outside of [0-1] range.
     * @param   tex_coord Texture coordinate (s,t)
     * @return  Returns the color from the texture lookup
     */
    Color3 get_color(const TextureCoord2 &tex_coord) override;

  protected:
    uint8_t *pixel_map_;
    int32_t  width_;
    int32_t  height_;
    float    s_diff_;
    float    t_diff_;
    int32_t  bytes_per_row_;

    // Get a texel given the row and column
    void get_texel(int32_t row, int32_t col, uint8_t &r, uint8_t &g, uint8_t &b);
};

} // namespace cg

#endif
