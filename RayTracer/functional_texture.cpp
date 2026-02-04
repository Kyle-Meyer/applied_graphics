#include "RayTracer/functional_texture.hpp"

namespace cg
{

FunctionalTexture::FunctionalTexture(PointPatternFn point_pattern)
    : point_pattern_(point_pattern), uv_pattern_(nullptr)
{
}

FunctionalTexture::FunctionalTexture(PointPatternFn point_pattern, UVPatternFn uv_pattern)
    : point_pattern_(point_pattern), uv_pattern_(uv_pattern)
{
}

Color3 FunctionalTexture::get_color(const Point3 &p)
{
    if (point_pattern_)
    {
        return point_pattern_(p);
    }
    return Color3(0.0f, 0.0f, 0.0f);
}

Color3 FunctionalTexture::get_color(const TextureCoord2 &tex_coord)
{
    if (uv_pattern_)
    {
        return uv_pattern_(tex_coord);
    }
    return Color3(0.0f, 0.0f, 0.0f);
}

// ========== Static factory methods ==========

std::shared_ptr<FunctionalTexture> FunctionalTexture::checkerboard(
    const Color3 &color1, const Color3 &color2, float scale)
{
    // World-space checkerboard
    auto point_fn = [color1, color2, scale](const Point3 &p) -> Color3 {
        int x = static_cast<int>(std::floor(p.x * scale));
        int y = static_cast<int>(std::floor(p.y * scale));
        int z = static_cast<int>(std::floor(p.z * scale));
        // Handle negative coordinates properly
        int sum = ((x % 2) + 2) % 2 + ((y % 2) + 2) % 2 + ((z % 2) + 2) % 2;
        return (sum % 2 == 0) ? color1 : color2;
    };

    // UV-space checkerboard
    auto uv_fn = [color1, color2, scale](const TextureCoord2 &tc) -> Color3 {
        int s = static_cast<int>(std::floor(tc.s * scale * 10.0f));
        int t = static_cast<int>(std::floor(tc.t * scale * 10.0f));
        return ((s + t) % 2 == 0) ? color1 : color2;
    };

    return std::make_shared<FunctionalTexture>(point_fn, uv_fn);
}

std::shared_ptr<FunctionalTexture> FunctionalTexture::stripes(
    const Color3 &color1, const Color3 &color2, float scale, int axis)
{
    auto point_fn = [color1, color2, scale, axis](const Point3 &p) -> Color3 {
        float coord = (axis == 0) ? p.x : (axis == 1) ? p.y : p.z;
        int stripe = static_cast<int>(std::floor(coord * scale));
        return ((stripe % 2 + 2) % 2 == 0) ? color1 : color2;
    };

    auto uv_fn = [color1, color2, scale, axis](const TextureCoord2 &tc) -> Color3 {
        float coord = (axis == 0) ? tc.s : tc.t;
        int stripe = static_cast<int>(std::floor(coord * scale * 10.0f));
        return (stripe % 2 == 0) ? color1 : color2;
    };

    return std::make_shared<FunctionalTexture>(point_fn, uv_fn);
}

std::shared_ptr<FunctionalTexture> FunctionalTexture::gradient(
    const Color3 &bottom_color, const Color3 &top_color)
{
    auto point_fn = [bottom_color, top_color](const Point3 &p) -> Color3 {
        // Normalize y to [0, 1] range (assuming y typically in [-1, 1] or similar)
        float t = (p.y + 1.0f) * 0.5f;
        t = std::max(0.0f, std::min(1.0f, t));
        return Color3(
            bottom_color.r + t * (top_color.r - bottom_color.r),
            bottom_color.g + t * (top_color.g - bottom_color.g),
            bottom_color.b + t * (top_color.b - bottom_color.b));
    };

    auto uv_fn = [bottom_color, top_color](const TextureCoord2 &tc) -> Color3 {
        float t = tc.t;
        return Color3(
            bottom_color.r + t * (top_color.r - bottom_color.r),
            bottom_color.g + t * (top_color.g - bottom_color.g),
            bottom_color.b + t * (top_color.b - bottom_color.b));
    };

    return std::make_shared<FunctionalTexture>(point_fn, uv_fn);
}

} // namespace cg
