#include "RayTracer/normal_map_node.hpp"

#include "scene/image_data.hpp"

// Declared in main.cpp — toggled by 'm' key
extern bool g_normal_map_enabled;

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>

namespace cg
{

NormalMapNode::NormalMapNode(float strength)
    : pixel_map_(nullptr), width_(0), height_(0), strength_(strength)
{
    node_type_ = SceneNodeType::PRESENTATION;
}

NormalMapNode::NormalMapNode(const char *filename, float strength)
    : pixel_map_(nullptr), width_(0), height_(0), strength_(strength)
{
    node_type_ = SceneNodeType::PRESENTATION;

    ImageData im;
    load_image_data(im, filename, false);  // RGB only — no alpha needed
    if (!im.data)
    {
        std::cerr << "NormalMapNode: could not load '" << filename << "'\n";
        return;
    }

    width_  = im.w;
    height_ = im.h;

    const size_t n = static_cast<size_t>(width_) * height_ * 3;
    pixel_map_ = new uint8_t[n];
    std::memcpy(pixel_map_, im.data, n);
    free_image_data(im);
}

NormalMapNode::~NormalMapNode()
{
    delete[] pixel_map_;
}

void NormalMapNode::draw(SceneState &scene_state)
{
    SceneNode::draw(scene_state);
}

void NormalMapNode::find_closest_intersect(Ray3 ray,
                                           SceneState &current_state,
                                           SceneState &closest)
{
    if (g_normal_map_enabled)
    {
        current_state.push_normal_map();
        current_state.normal_map_node = this;
    }

    for (auto c : children_)
        c->find_closest_intersect(ray, current_state, closest);

    if (g_normal_map_enabled)
        current_state.pop_normal_map();
}

void NormalMapNode::fetch_texel(int32_t row, int32_t col,
                                float &r, float &g, float &b) const
{
    // Clamp to valid range
    row = std::max(0, std::min(row, height_ - 1));
    col = std::max(0, std::min(col, width_  - 1));

    const uint8_t *p = pixel_map_ + (row * width_ + col) * 3;
    constexpr float INV255 = 1.0f / 255.0f;
    r = p[0] * INV255;
    g = p[1] * INV255;
    b = p[2] * INV255;
}

Vector3 NormalMapNode::sample(const Point2 &uv, const Point3 & /*world_pos*/, float /*cam_dist*/) const
{
    if (!pixel_map_)
        return Vector3(0.0f, 0.0f, 1.0f);  // Neutral: no perturbation

    // Wrap UVs into [0,1) to handle tiling
    float s = uv.x - std::floor(uv.x);
    float t = uv.y - std::floor(uv.y);

    // Map to texel space
    float fx = s * static_cast<float>(width_  - 1);
    float fy = t * static_cast<float>(height_ - 1);

    int32_t c0 = static_cast<int32_t>(fx);
    int32_t r0 = static_cast<int32_t>(fy);
    int32_t c1 = c0 + 1;
    int32_t r1 = r0 + 1;

    float ds = fx - static_cast<float>(c0);
    float dt = fy - static_cast<float>(r0);

    // Bilinear interpolation
    float r00, g00, b00, r10, g10, b10, r01, g01, b01, r11, g11, b11;
    fetch_texel(r0, c0, r00, g00, b00);
    fetch_texel(r1, c0, r10, g10, b10);
    fetch_texel(r0, c1, r01, g01, b01);
    fetch_texel(r1, c1, r11, g11, b11);

    float w00 = (1.0f - ds) * (1.0f - dt);
    float w10 = (1.0f - ds) * dt;
    float w01 = ds * (1.0f - dt);
    float w11 = ds * dt;

    float r_f = w00 * r00 + w01 * r01 + w10 * r10 + w11 * r11;
    float g_f = w00 * g00 + w01 * g01 + w10 * g10 + w11 * g11;
    float b_f = w00 * b00 + w01 * b01 + w10 * b10 + w11 * b11;

    // Remap [0,1] -> [-1,1] for x and y; b stays in [0,1] -> [0,1] (z is always positive)
    Vector3 tn(r_f * 2.0f - 1.0f,
               g_f * 2.0f - 1.0f,
               b_f);

    // Blend toward flat normal (0,0,1) based on strength
    if (strength_ < 1.0f)
    {
        tn.x *= strength_;
        tn.y *= strength_;
        tn.z  = tn.z * strength_ + (1.0f - strength_);
    }

    return tn;
}

} // namespace cg
