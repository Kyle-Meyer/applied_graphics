#include "RayTracer/image_texture.hpp"

#include <cmath>
#include <cstring>
#include <iostream>

namespace cg
{

ImageTexture::ImageTexture(const char *filename)
{
    ImageData im_data;
    load_image_data(im_data, filename, false);
    if(im_data.data == nullptr)
    {
        std::cout << "Error loading texture " << filename << '\n';
        return;
    }

    // Get the image dimensions
    width_ = im_data.w;
    height_ = im_data.h;

    // Crate pixel_map_ memory and copy image to pixel_map_
    const size_t pixmap_size = width_ * height_ * im_data.channels;
    pixel_map_ = new uint8_t[pixmap_size];
    std::memcpy(pixel_map_, im_data.data, pixmap_size);

    // Compute the difference between texels
    s_diff_ = (1.0f / static_cast<float>(width_ - 1));
    t_diff_ = (1.0f / static_cast<float>(height_ - 1));

    // Number of bytes per row of the image
    bytes_per_row_ = width_ * 3;

    // May want to set OpenGL texture properites

    node_type_ = SceneNodeType::IMAGE_TEXTURE;
}

ImageTexture::~ImageTexture() { delete[] pixel_map_; }

void ImageTexture::draw(SceneState &scene_state)
{
    // Draw children of this node
    SceneNode::draw(scene_state);
}

Color3 ImageTexture::get_color(const Point3 &) { return Color3(0.0f, 0.0f, 0.0f); }

Color3 ImageTexture::get_color(const TextureCoord2 &tex_coord)
{
    float s = tex_coord.s;
    float t = tex_coord.t;

    // Get the row and column positions
    int32_t r0 = static_cast<int32_t>((height_ - 1) * t);
    int32_t r1 = std::min<int32_t>(r0 + 1, height_ - 1);
    int32_t c0 = static_cast<int32_t>((width_ - 1) * s);
    int32_t c1 = std::min<int32_t>(c0 + 1, width_ - 1);

    // Compute the weighting factors
    float ds = s / s_diff_ - static_cast<float>(c0);
    float dt = t / t_diff_ - static_cast<float>(r0);
    float w00 = (1.0f - ds) * (1.0f - dt);
    float w10 = (1.0f - ds) * dt;
    float w01 = ds * (1.0f - dt);
    float w11 = ds * dt;

    // Get the 4 texel colors
    uint8_t r00, r10, r01, r11;
    uint8_t g00, g10, g01, g11;
    uint8_t b00, b10, b01, b11;
    get_texel(r0, c0, r00, g00, b00);
    get_texel(r1, c0, r10, g10, b10);
    get_texel(r0, c1, r01, g01, b01);
    get_texel(r1, c1, r11, g11, b11);

    // Calculate a linear, weighted average
    uint8_t r = static_cast<uint8_t>(w00 * r00 + w01 * r01 + w10 * r10 + w11 * r11);
    uint8_t g = static_cast<uint8_t>(w00 * g00 + w01 * g01 + w10 * g10 + w11 * g11);
    uint8_t b = static_cast<uint8_t>(w00 * b00 + w01 * b01 + w10 * b10 + w11 * b11);

    constexpr float COLOR_BYTE_SCALING = 1.0f / 255.0f;
    return Color3(r * COLOR_BYTE_SCALING, g * COLOR_BYTE_SCALING, b * COLOR_BYTE_SCALING);
}

void ImageTexture::get_texel(int32_t row, int32_t col, uint8_t &r, uint8_t &g, uint8_t &b)
{
    // Note - invert the y
    uint8_t *texptr = pixel_map_ + (row * bytes_per_row_) + (col * 3);
    r = *texptr++;
    g = *texptr++;
    b = *texptr;
}

} // namespace cg
