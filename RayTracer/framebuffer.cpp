//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    Framebuffer.h
//	Purpose: Framebuffer class to support drawing pixels in the ray tracing
//           application.
//
//============================================================================

#include "RayTracer/framebuffer.hpp"
#include "RayTracer/shader_src.hpp"

#include <cstring>
#include <iostream>

namespace cg
{

Framebuffer::Framebuffer() {}

Framebuffer::Framebuffer(uint32_t w, uint32_t h)
{
    // Round up width and height to BLOCK_SIZE increment
    buffer_width_ = ((w / FB_BLOCK_SIZE) + 1) * FB_BLOCK_SIZE;
    buffer_height_ = ((h / FB_BLOCK_SIZE) + 1) * FB_BLOCK_SIZE;

    tex_width_ = w;
    tex_height_ = h;

    // Size the vectors
    pixel_set_.resize(buffer_width_ * buffer_height_);

    // Allocate pixel storage
    pixels_ = new cColor3[buffer_width_ * buffer_height_];
    tex_pixels_ = new cColor3[tex_width_ * tex_height_];

    if(!init_shader_texture_buffers()) { exit(-1); }
}

Framebuffer::~Framebuffer()
{
    delete[] pixels_;
    delete[] tex_pixels_;
}

void Framebuffer::clear()
{
    // Clear all the set flags
    for(auto s : pixel_set_) { s = false; }
}

void Framebuffer::set(int32_t x_LL, int32_t y_LL, Color3 &color, uint32_t block_size)
{
    // Mark this pixel as being set so it doesn't get calculated again
    pixel_set_[get_pixel_index(x_LL, y_LL)] = true;

    cColor3 c_color = {color.r_byte(), color.g_byte(), color.b_byte()};

    // Set pixels within the "block"
    for(uint32_t y = y_LL; y < y_LL + block_size; ++y)
    {
        // Get pointer to pixel at the left of the block for this row
        cColor3 *p = pixels_ + (get_pixel_index(x_LL, y));
        for(uint32_t x = x_LL; x < x_LL + block_size; ++x, ++p) { *p = c_color; }
    }
}

bool Framebuffer::set(int32_t x, int32_t y) { return pixel_set_[get_pixel_index(x, y)]; }

void Framebuffer::render()
{
    for(uint32_t row = 0; row < tex_height_; ++row)
    {
        cColor3 *pixel_ptr = pixels_ + buffer_width_ * row;
        cColor3 *tex_pixel_ptr = tex_pixels_ + tex_width_ * row;
        std::memcpy(tex_pixel_ptr, pixel_ptr, tex_width_ * sizeof(cColor3));
    }

    shader_program_.use();

    glUniform1i(texture_loc_, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexSubImage2D(
        GL_TEXTURE_2D, 0, 0, 0, tex_width_, tex_height_, GL_RGB, GL_UNSIGNED_BYTE, tex_pixels_);

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Framebuffer::anti_alias()
{
    {
        int32_t r, g, b;
        int32_t lo_idx, up_idx;

        for(uint32_t x = 0; x < buffer_width_ - 1; x++)
        {
            for(uint32_t y = 0; y < buffer_height_ - 1; y++)
            {
                // Get the total RGB for a 4 pixel square
                lo_idx = get_pixel_index(x, y);
                up_idx = get_pixel_index(x, y + 1);

                cColor3       &p00 = pixels_[lo_idx];
                const cColor3 &p10 = pixels_[lo_idx + 1];
                const cColor3 &p01 = pixels_[up_idx];
                const cColor3 &p11 = pixels_[up_idx + 1];

                r = (static_cast<int32_t>(p00.r) + static_cast<int32_t>(p10.r) +
                     static_cast<int32_t>(p01.r) + static_cast<int32_t>(p11.r));

                g = (static_cast<int32_t>(p00.g) + static_cast<int32_t>(p10.g) +
                     static_cast<int32_t>(p01.g) + static_cast<int32_t>(p11.g));

                b = (static_cast<int32_t>(p00.b) + static_cast<int32_t>(p10.b) +
                     static_cast<int32_t>(p01.b) + static_cast<int32_t>(p11.b));

                p00.r = static_cast<uint8_t>(std::min<int32_t>(std::max<int32_t>(r / 4, 0), 255));
                p00.g = static_cast<uint8_t>(std::min<int32_t>(std::max<int32_t>(g / 4, 0), 255));
                p00.b = static_cast<uint8_t>(std::min<int32_t>(std::max<int32_t>(b / 4, 0), 255));
            }
        }
    }

    render();
}

int32_t Framebuffer::get_pixel_index(int32_t x, int32_t y) const { return y * buffer_width_ + x; }

bool Framebuffer::init_shader_texture_buffers()
{
    // Create and compile the vertex shader
    if(!vertex_shader_.create_from_source(tex_vert))
    {
        std::cout << "Vertex Shader compile failed\n";
        return false;
    }

    // Create and compile the fragment shader
    if(!fragment_shader_.create_from_source(tex_frag))
    {
        std::cout << "Fragment Shader compile failed\n";
        return false;
    }

    shader_program_.create();
    if(!shader_program_.attach_shaders(vertex_shader_.get(), fragment_shader_.get()))
    {
        std::cout << "Shader program link failed\n";
        return false;
    }

    shader_program_.use();

    position_loc_ = glGetAttribLocation(shader_program_.get_program(), "vtx_position");
    if(position_loc_ < 0)
    {
        std::cout << "Error getting vtx_position location" << std::endl;
        return false;
    }

    texture_loc_ = glGetAttribLocation(shader_program_.get_program(), "vtx_texture");
    if(texture_loc_ < 0)
    {
        std::cout << "Error getting vertex texture location\n";
        return false;
    }

    /*
     * Setup Texture
     */
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    // Load image data and generate mipmaps
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 tex_width_,
                 tex_height_,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 tex_pixels_);

    // Set texture filters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Bind null texture
    glBindTexture(GL_TEXTURE_2D, 0);

    /*
     * Setup Vertex Buffers
     */
    float vtx_tex[4][4] = {
        {-1.0f, -1.0f, 0.0f, 0.0f}, // bottom-left
        {1.0f, -1.0f, 1.0f, 0.0f},  // bottom-right
        {-1.0f, 1.0f, 0.0f, 1.0f},  // top-left
        {1.0f, 1.0f, 1.0f, 1.0f}    // top-right
    };

    glGenBuffers(1, &vbo_);

    // Bind the vertex list to the vertex buffer object
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, (void *)vtx_tex, GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glVertexAttribPointer(position_loc_, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)0);
    glVertexAttribPointer(
        texture_loc_, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)(sizeof(float) * 2));
    glEnableVertexAttribArray(position_loc_);
    glEnableVertexAttribArray(texture_loc_);
    glBindVertexArray(0);

    return true;
}

} // namespace cg
