//============================================================================
//  RayTracer/desert_rock_texture.hpp
//  Procedural desert rock albedo — ported from textures/desert_rock.glsl.
//
//  Implements only the base-color path (GLSL steps 1-6: fBm noise → color
//  mix).  Lighting is handled by the ray tracer's Cook-Torrance pipeline;
//  the bump / PBR sections of the GLSL are omitted here.
//
//  Uses world-space hit position as texture coordinates.  Defaults are tuned
//  for world-scale rocks (scale/noise_scale reduced vs. the object-space
//  GLSL originals).
//============================================================================

#ifndef __DESERT_ROCK_TEXTURE_HPP__
#define __DESERT_ROCK_TEXTURE_HPP__

#include "RayTracer/texture_node.hpp"
#include "scene/color3.hpp"

namespace cg
{

class DesertRockTexture : public TextureNode
{
  public:
    DesertRockTexture();

    Color3 get_color(const Point3 &p) override;

    // ------------------------------------------------------------------
    // Tunable parameters — match the GLSL uniform names.
    // Defaults are scaled for world-space coordinates (rocks span several
    // world units, so noise_scale and stripes_scale are lower than the
    // object-space originals).
    // ------------------------------------------------------------------
    float  scale          = 0.5f;
    Color3 color1         = Color3(0.35f, 0.40f, 0.55f);   // moonlit blue-grey highlight
    Color3 color2         = Color3(0.10f, 0.12f, 0.20f);   // darker blue base
    Color3 stripes_color  = Color3(0.05f, 0.06f, 0.12f);   // deep blue-black stripe
    float  noise_scale    = 6.0f;   // GLSL default 16 (object-space)
    float  stripes_scale  = 5.0f;   // GLSL default 13
    float  texture_detail = 8.0f;   // fBm octave count; GLSL default 15
    float  texture_detail2= 0.5f;   // fBm roughness (noise-roughness path)

  private:
    static float hash1(float x, float y, float z);
    static float vnoise(float x, float y, float z);
    static float fbm(float x, float y, float z,
                     float detail, float roughness,
                     float lacunarity, float distortion);
};

} // namespace cg

#endif
