#include "RayTracer/desert_rock_texture.hpp"

#include <cmath>
#include <algorithm>

namespace cg
{

DesertRockTexture::DesertRockTexture()
{
    node_type_ = SceneNodeType::PRESENTATION;
}

// -----------------------------------------------------------------------
//  hash1 — scalar hash in [0, 1].  Mirrors GLSL fract(sin(dot)*large).
// -----------------------------------------------------------------------
float DesertRockTexture::hash1(float x, float y, float z)
{
    float v = std::sin(x * 127.1f + y * 311.7f + z * 74.7f) * 43758.5453f;
    return v - std::floor(v);
}

// -----------------------------------------------------------------------
//  vnoise — smooth value noise via trilinear interpolation of hashed
//  lattice corners.  Equivalent to Blender's "smooth" noise texture.
// -----------------------------------------------------------------------
float DesertRockTexture::vnoise(float px, float py, float pz)
{
    float ix = std::floor(px), iy = std::floor(py), iz = std::floor(pz);
    float fx = px - ix,        fy = py - iy,        fz = pz - iz;

    // Hermite smoothstep
    float ux = fx * fx * (3.0f - 2.0f * fx);
    float uy = fy * fy * (3.0f - 2.0f * fy);
    float uz = fz * fz * (3.0f - 2.0f * fz);

    auto lerp = [](float a, float b, float t) { return a + t * (b - a); };

    float v000 = hash1(ix,     iy,     iz    );
    float v100 = hash1(ix+1.f, iy,     iz    );
    float v010 = hash1(ix,     iy+1.f, iz    );
    float v110 = hash1(ix+1.f, iy+1.f, iz    );
    float v001 = hash1(ix,     iy,     iz+1.f);
    float v101 = hash1(ix+1.f, iy,     iz+1.f);
    float v011 = hash1(ix,     iy+1.f, iz+1.f);
    float v111 = hash1(ix+1.f, iy+1.f, iz+1.f);

    return lerp(lerp(lerp(v000, v100, ux), lerp(v010, v110, ux), uy),
                lerp(lerp(v001, v101, ux), lerp(v011, v111, ux), uy), uz);
}

// -----------------------------------------------------------------------
//  fbm — fractional Brownian motion.
//  Mirrors Blender's Noise Texture (lacunarity=2, normalised output).
//  distortion > 0 warps the input coordinates before sampling (matches
//  the stripes noise texture which uses distortion=0.4 in the GLSL).
// -----------------------------------------------------------------------
float DesertRockTexture::fbm(float px, float py, float pz,
                              float detail, float roughness,
                              float lacunarity, float distortion)
{
    if (distortion > 0.0f)
    {
        float wx = vnoise(px+1.7f, py+9.2f, pz+5.4f) * 2.0f - 1.0f;
        float wy = vnoise(px+8.3f, py+2.8f, pz+1.2f) * 2.0f - 1.0f;
        float wz = vnoise(px+3.1f, py+6.5f, pz+7.9f) * 2.0f - 1.0f;
        px += wx * distortion;
        py += wy * distortion;
        pz += wz * distortion;
    }

    float value     = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float max_val   = 0.0f;
    int   octaves   = static_cast<int>(std::min(std::max(detail, 1.0f), 16.0f));

    for (int i = 0; i < octaves; ++i)
    {
        value    += vnoise(px * frequency, py * frequency, pz * frequency) * amplitude;
        max_val  += amplitude;
        amplitude *= roughness;
        frequency *= lacunarity;
    }

    // Fractional last octave
    float frac = detail - std::floor(detail);
    if (frac > 0.0f)
    {
        value   += vnoise(px * frequency, py * frequency, pz * frequency) * amplitude * frac;
        max_val += amplitude * frac;
    }

    return (max_val > 0.0f) ? (value / max_val) : 0.0f;
}

// -----------------------------------------------------------------------
//  get_color — world-space hit point → rock albedo.
//  Mirrors GLSL steps 1-6: coord mapping → two noise textures →
//  colour ramp (identity) → colour mix.
// -----------------------------------------------------------------------
Color3 DesertRockTexture::get_color(const Point3 &p)
{
    // Step 1-2: apply scale, then the two Mapping node stretches from the GLSL
    float ox = p.x * scale;
    float oy = p.y * scale;
    float oz = p.z * scale;

    // Mapping.001 (main): scale Z by 2
    float mx = ox, my = oy, mz = oz * 2.0f;
    // Mapping.002 (stripes): scale Z by 10
    float sx = ox, sy = oy, sz = oz * 10.0f;

    // Step 3: main noise (no distortion)
    float nr = std::min(std::max(texture_detail2, 0.0f), 1.0f);
    float noise_main = fbm(mx * noise_scale, my * noise_scale, mz * noise_scale,
                           texture_detail, nr, 2.0f, 0.0f);

    // Step 4: stripes noise (distortion=0.4)
    float noise_stripes = fbm(sx * stripes_scale, sy * stripes_scale, sz * stripes_scale,
                               texture_detail, 0.8f, 2.0f, 0.4f);

    // Step 5: ColorRamp (identity clamp — swap these out for custom ramp stops)
    float ramp_main    = std::min(std::max(noise_main,    0.0f), 1.0f);
    float ramp_stripes = std::min(std::max(noise_stripes, 0.0f), 1.0f);

    // Step 6: colour mixing
    // Mix.0: lerp(color1, color2, noise_main)
    Color3 result_ab(
        color1.r + noise_main * (color2.r - color1.r),
        color1.g + noise_main * (color2.g - color1.g),
        color1.b + noise_main * (color2.b - color1.b));

    // Mix.1: lerp(stripes_color, result_ab, ramp_stripes)
    Color3 base_color(
        stripes_color.r + ramp_stripes * (result_ab.r - stripes_color.r),
        stripes_color.g + ramp_stripes * (result_ab.g - stripes_color.g),
        stripes_color.b + ramp_stripes * (result_ab.b - stripes_color.b));

    return base_color;
}

} // namespace cg
