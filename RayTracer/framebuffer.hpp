//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    framebuffer.h
//	Purpose: Framebuffer class to support drawing pixels in the ray tracing
//           application.
//
//============================================================================

#ifndef __RAY_TRACER_FRAMEBUFFER_HPP__
#define __RAY_TRACER_FRAMEBUFFER_HPP__

#include "scene/color3.hpp"
#include "scene/graphics.hpp"
#include "scene/scene.hpp"

#include <vector>

namespace cg
{

extern "C"
{
    struct cColor3
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };
} // extern "C"

// pixel block size (for iterative ray tracing so partial solutions
// are displayed quickly)
const uint32_t FB_BLOCK_SIZE = 16;

/**
 * Framebuffer class to support drawing pixels in the ray tracing application
 */
class Framebuffer
{
  public:
    /**
     * Constructs local framebuffer support given an image width and height.
     */
    Framebuffer(uint32_t w, uint32_t h);

    /**
     * Destructor
     */
    ~Framebuffer();

    /**
     * Clears the "set" flags. Sets them to false.
     */
    void clear();

    /**
     * Sets a block of pixels to the specified color. Marks the lower
     * left pixel as being set so it does not need to be recalculated
     * on following iterations.
     * @param  x_LL        x component: lower left pixel of the block
     * @param  y_LL        y component: lower left pixel of the block
     * @param  color      Color to assign to the block
     * @param  block_size  Block size in pixels (x and y)
     */
    void set(int32_t x_LL, int32_t y_LL, Color3 &color, uint32_t block_size);

    /**
     * Tests whether the pixel position has been set. Only the lower left position
     * in each "block" of pixels has this flag set.
     * @param  x   x pixel value
     * @param  y   y pixel value
     */
    bool set(int32_t x, int32_t y);

    /**
     * Draws the framebuffer to the display using glDrawPixels (legacy OpenGL).
     */
    void render();

    /**
     * Perform simple anti-aliasing by pixel averaging. No additional rays
     * are traced.
     */
    void anti_alias();

  private:
    uint32_t          buffer_width_;
    uint32_t          buffer_height_;
    uint32_t          tex_width_;
    uint32_t          tex_height_;
    std::vector<bool> pixel_set_;
    cColor3          *pixels_;
    cColor3          *tex_pixels_;

    GLuint             texture_id_;
    GLSLVertexShader   vertex_shader_;
    GLSLFragmentShader fragment_shader_;
    GLSLShaderProgram  shader_program_;
    GLint              position_loc_;
    GLint              texture_loc_;
    GLuint             vao_;
    GLuint             vbo_;

    // Make default constructor private to force use of w,h constructor
    Framebuffer();

    int32_t get_pixel_index(int32_t x, int32_t y) const;

    bool init_shader_texture_buffers();
};

} // namespace cg

#endif
