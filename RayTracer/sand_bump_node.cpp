//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//
//	File:    sand_bump_node.cpp
//	Purpose: Procedural sand micro-ripple bump shader.
//           Macro-scale dune geometry is handled by HeightFieldFloorNode;
//           this shader adds fine-scale wind ripples and grain noise on top.
//============================================================================

#include "RayTracer/sand_bump_node.hpp"

#include <cmath>

namespace cg
{

SandBumpNode::SandBumpNode(float ripple_freq, float ripple_amp,
                           float grain_freq,  float grain_amp,
                           float strength)
    : NormalMapNode(strength),
      ripple_freq_(ripple_freq),
      ripple_amp_(ripple_amp),
      grain_freq_(grain_freq),
      grain_amp_(grain_amp)
{
}

Vector3 SandBumpNode::sample(const Point2 & /*uv*/, const Point3 &world_pos, float cam_dist) const
{
    const float PI2 = 2.0f * static_cast<float>(M_PI);

    // Use world-space XZ so ripples are uniform across the floor regardless
    // of the sphere's UV parameterisation (which compresses near the pole).
    float u = world_pos.x;
    float v = world_pos.z;

    // ── Domain warp ────────────────────────────────────────────────────────
    // Distort (u,v) with a smooth low-frequency function before computing
    // ripples.  This curves the otherwise straight parallel lines into
    // meandering wind-formed ridges.  Two offset sine pairs give the warp
    // its own organic variation without repeating too soon.
    float warp_scale = 0.04f;   // spatial frequency of large wind-gust curves
    float warp_amp   = 4.0f;    // world-unit deflection — curves ripples over ~25u span
    float wu = u + warp_amp * (sinf(warp_scale * (u * 1.0f + v * 0.7f))
                              + 0.5f * sinf(warp_scale * 1.9f * (u * 0.6f - v * 1.0f)));
    float wv = v + warp_amp * (cosf(warp_scale * (u * 0.8f + v * 1.1f))
                              + 0.5f * cosf(warp_scale * 1.7f * (u * 1.0f + v * 0.5f)));

    // Derivatives of the warp (needed to correctly propagate slopes through).
    // dwu/du, dwu/dv, dwv/du, dwv/dv — computed analytically from the above.
    float ws = warp_scale, wa = warp_amp;
    float dwu_du = 1.0f + wa * ws * (cosf(ws*(u+0.7f*v))
                                    + 0.5f*1.9f*ws * cosf(ws*1.9f*(0.6f*u-v)) * 0.6f);
    float dwu_dv = wa * ws * (0.7f*cosf(ws*(u+0.7f*v))
                             + 0.5f*1.9f*ws * cosf(ws*1.9f*(0.6f*u-v)) * (-1.0f));
    float dwv_du = wa * ws * (-0.8f*sinf(ws*(0.8f*u+1.1f*v))
                             + 0.5f*1.7f*ws * (-sinf(ws*1.7f*(u+0.5f*v))) * 1.0f);
    float dwv_dv = 1.0f + wa * ws * (-1.1f*sinf(ws*(0.8f*u+1.1f*v))
                                    + 0.5f*1.7f*ws * (-sinf(ws*1.7f*(u+0.5f*v))) * 0.5f);

    // ── Distance LOD ──────────────────────────────────────────────────────
    // Grain fades out 5..25 units; ripples soften 25..65 units.
    auto smoothstep = [](float edge0, float edge1, float x) {
        float t = std::max(0.0f, std::min((x - edge0) / (edge1 - edge0), 1.0f));
        return t * t * (3.0f - 2.0f * t);
    };
    float grain_scale  = 1.0f - smoothstep( 5.0f, 25.0f, cam_dist);
    float ripple_scale = 1.0f - smoothstep(25.0f, 65.0f, cam_dist) * 0.85f;

    // ── Primary aeolian ripples (in warped space) ──────────────────────────
    // h = A * sin(2π * freq * wu)  →  slope in warped u/v, then chain-rule
    // back to world dx/dy.
    float f1 = ripple_freq_;
    float a1 = ripple_amp_ * ripple_scale;
    float dh_dwu = -a1 * cosf(PI2 * f1 * wu) * (PI2 * f1);
    float dh_dwv = -a1 * 0.20f * cosf(PI2 * f1 * 0.30f * wv) * (PI2 * f1 * 0.30f);
    float dx = dh_dwu * dwu_du + dh_dwv * dwv_du;
    float dy = dh_dwu * dwu_dv + dh_dwv * dwv_dv;

    // ── Secondary crossing ripples ─────────────────────────────────────────
    float f2 = f1 * 0.60f;
    float a2 = a1 * 0.40f;
    float wu2 =  0.85f * wu + 0.53f * wv;   // ~32° rotation in warped space
    float wv2 = -0.53f * wu + 0.85f * wv;
    float dh2_dwu2 = -a2 * cosf(PI2 * f2 * wu2) * (PI2 * f2);
    float dh2_dwv2 = -a2 * 0.12f * cosf(PI2 * f2 * 0.4f * wv2) * (PI2 * f2 * 0.4f);
    // chain rule: d(wu2)/du = 0.85*dwu_du - 0.53*dwv_du, etc.
    float dwu2_du = 0.85f * dwu_du - 0.53f * dwv_du;
    float dwu2_dv = 0.85f * dwu_dv - 0.53f * dwv_dv;
    float dwv2_du = 0.53f * dwu_du + 0.85f * dwv_du;
    float dwv2_dv = 0.53f * dwu_dv + 0.85f * dwv_dv;
    dx += dh2_dwu2 * dwu2_du + dh2_dwv2 * dwv2_du;
    dy += dh2_dwu2 * dwu2_dv + dh2_dwv2 * dwv2_dv;

    // ── Fine grain noise ───────────────────────────────────────────────────
    // Hash-based high-frequency perturbation mimics individual sand grains.
    // Use warped coords so the grain pattern aligns with the ripple curvature.
    auto fract = [](float x){ return x - std::floor(x); };
    float su = wu * grain_freq_;
    float sv = wv * grain_freq_;
    float hx = fract(fabsf(sinf(su * 12.9898f + sv * 78.233f)  * 43758.5453f));
    float hy = fract(fabsf(sinf(su *  4.1414f + sv *  2.7183f) * 31415.9265f));
    dx += (hx * 2.0f - 1.0f) * grain_amp_ * grain_scale;
    dy += (hy * 2.0f - 1.0f) * grain_amp_ * grain_scale;

    // ── Build and normalise the tangent-space normal ───────────────────────
    // A height-field with slopes (dx, dy) has the unnormalised normal (-dx, -dy, 1).
    float dz  = 1.0f;
    float len = sqrtf(dx * dx + dy * dy + dz * dz);
    Vector3 tn(dx / len, dy / len, dz / len);

    // Blend toward flat normal (0, 0, 1) by (1 - strength) so strength=0
    // leaves the surface unperturbed and strength=1 gives the full effect.
    float s = strength();
    if (s < 1.0f)
    {
        tn.x *= s;
        tn.y *= s;
        tn.z  = tn.z * s + (1.0f - s);
    }

    return tn;
}

} // namespace cg
