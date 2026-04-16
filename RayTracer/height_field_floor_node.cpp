//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//
//	File:    height_field_floor_node.cpp
//	Purpose: Procedural dune height field floor for the ray tracer.
//============================================================================

#include "RayTracer/height_field_floor_node.hpp"

#include <cmath>

// ── Dune height-field helpers (file-private) ──────────────────────────────
namespace
{

// Smooth 2D value noise — bilinear interpolation of a hashed grid.
// Quintic smooth-step gives C2 continuity; output in [0, 1].
static float value_noise_2d(float x, float y)
{
    auto fract_f = [](float v) { return v - std::floor(v); };
    auto hash_f  = [&](float a, float b) {
        return fract_f(fabsf(sinf(a * 127.1f + b * 311.7f) * 43758.5453f));
    };
    float ix = std::floor(x), iy = std::floor(y);
    float fx = x - ix,        fy = y - iy;
    float ux = fx * fx * fx * (fx * (fx * 6.0f - 15.0f) + 10.0f);
    float uy = fy * fy * fy * (fy * (fy * 6.0f - 15.0f) + 10.0f);
    float a = hash_f(ix,       iy      );
    float b = hash_f(ix + 1.f, iy      );
    float c = hash_f(ix,       iy + 1.f);
    float d = hash_f(ix + 1.f, iy + 1.f);
    return a + (b - a) * ux + (c - a) * uy + (a - b - c + d) * ux * uy;
}

// Asymmetric dune cross-section profile, t in [0, 1].
//   0 → peak_t : gentle windward slope (smooth-step rise)
//   peak_t → 1 : steep leeward slip face (quadratic drop)
static float dune_profile(float t)
{
    if (t <= 0.0f || t >= 1.0f) return 0.0f;
    const float peak = 0.62f;
    if (t < peak) {
        float s = t / peak;
        return s * s * (3.0f - 2.0f * s);       // windward: smooth-step 0→1
    } else {
        float s = (t - peak) / (1.0f - peak);
        return 1.0f - s * s;                      // leeward: steep quadratic drop
    }
}

// Single dune-ridge layer at world position (wx, wz).
//   period    — crest-to-crest distance in world units
//   rotation  — ridge orientation angle (radians)
//   noise_val — domain-warp value in [0, 1]; shifts ridge phase laterally
static float dune_layer(float wx, float wz,
                        float period, float rotation, float noise_val)
{
    const float gap_frac = 0.35f;               // 35% of each period is flat sand
    float dune_w = period * (1.0f - gap_frac);  // width of one dune profile

    float shift = noise_val * period * 0.70f;   // domain-warp phase offset

    float xb = wx * cosf(rotation) - wz * sinf(rotation);

    float raw = xb + shift;
    float cp  = raw - period * std::floor(raw / period); // position in [0, period)

    float t = cp / dune_w;
    return (t <= 1.0f) ? dune_profile(t) : 0.0f;
}

// Two-octave dune height at world position (wx, wz).
// Returns a value in roughly [0, 1.45].  Caller scales by height_scale_.
//
// Implements the article's formula: Dunes(x, y, width, depth) applied in two
// overlapping layers at different scales / orientations / domain warps.
static float dune_height_at(float wx, float wz)
{
    const float PI = 3.14159265f;

    // Domain-warp noise for each octave.
    // Scale 0.09 → grid cells ~11 wu, giving several variation cells across the
    // ~50 wu deep visible scene.  Offsets prevent the degenerate sin(0)=0 corner
    // of the hash from zeroing the warp near the world origin.
    float ns1 = value_noise_2d(wx * 0.09f + 1.7f, wz * 0.09f + 0.9f);
    float ns2 = value_noise_2d(wx * 0.13f + 5.1f, wz * 0.13f + 2.9f);

    // Octave 1 — large dunes (~18 wu crest-to-crest), ridges at -45°
    float h1 = dune_layer(wx, wz, 18.0f, -PI * 0.25f, ns1);

    // Octave 2 — smaller dunes (~8 wu), slightly different angle, half amplitude
    float h2 = dune_layer(wx, wz,  8.0f, -PI * 0.18f, ns2);

    return h1 + 0.45f * h2;
}

// Small constant for self-intersection avoidance (matches the rest of the tracer)
static constexpr float HFEPS = 1e-3f;

} // anonymous namespace

// ── HeightFieldFloorNode ──────────────────────────────────────────────────
namespace cg
{

HeightFieldFloorNode::HeightFieldFloorNode(float base_y, float height_scale)
    : base_y_(base_y),
      height_scale_(height_scale),
      max_height_(height_scale * 1.5f)
{
}

float HeightFieldFloorNode::surface_y(float x, float z) const
{
    return base_y_ + height_scale_ * dune_height_at(x, z);
}

// ── Ray marching ──────────────────────────────────────────────────────────
//
// Strategy:
//   1. Quick reject if the ray cannot reach the height-field volume.
//   2. Advance t to where the ray first enters the ceiling (base_y + max_height).
//   3. March in steps sized so we advance at most XZ_STEP world units horizontally
//      per iteration — small enough to not skip any feature of the 8 wu dunes.
//   4. When the ray dips below the surface (gap flips negative), bisect the
//      bracketing [t_prev, t_curr] interval for a precise hit.
//   5. Accept only if t < closest.t_min.

static constexpr float XZ_STEP  = 0.30f;  // max horizontal step (world units)
static constexpr float MAX_T    = 250.0f; // give up after this distance
static constexpr int   BISECT_N = 12;     // bisection refinement iterations

void HeightFieldFloorNode::find_closest_intersect(Ray3         ray,
                                                   SceneState  &current_state,
                                                   SceneState  &closest)
{
    float ceil_y = base_y_ + max_height_;

    // ── Quick reject ─────────────────────────────────────────────────────
    // Ray going upward and already above the ceiling → can't hit.
    if (ray.d.y >= 0.0f && ray.o.y >= ceil_y)
        return;

    // ── Find march entry t ───────────────────────────────────────────────
    float t_start = HFEPS;
    if (ray.o.y > ceil_y)
    {
        // Descend to the ceiling plane before marching
        if (ray.d.y >= 0.0f) return;   // going up from above ceiling
        t_start = (ray.o.y - ceil_y) / (-ray.d.y);
    }

    // ── Compute step size ─────────────────────────────────────────────────
    // dt is chosen so we advance at most XZ_STEP in the XZ plane per step.
    // For near-vertical rays the horizontal speed is tiny → cap dt.
    float xz_speed = sqrtf(ray.d.x * ray.d.x + ray.d.z * ray.d.z);
    float dt = (xz_speed > 0.02f) ? (XZ_STEP / xz_speed) : 0.15f;

    // ── March ─────────────────────────────────────────────────────────────
    float t_prev  = t_start;
    float gap_prev = (ray.o.y + t_prev * ray.d.y)
                   - surface_y(ray.o.x + t_prev * ray.d.x,
                                ray.o.z + t_prev * ray.d.z);

    for (float t = t_start + dt; t < MAX_T; t += dt)
    {
        float px  = ray.o.x + t * ray.d.x;
        float py  = ray.o.y + t * ray.d.y;
        float pz  = ray.o.z + t * ray.d.z;
        float gap = py - surface_y(px, pz);

        if (gap < 0.0f)
        {
            // Ray crossed the surface between t_prev and t — bisect.
            float lo = t_prev, hi = t;
            for (int i = 0; i < BISECT_N; ++i)
            {
                float mid  = 0.5f * (lo + hi);
                float mx   = ray.o.x + mid * ray.d.x;
                float my   = ray.o.y + mid * ray.d.y;
                float mz   = ray.o.z + mid * ray.d.z;
                float mgap = my - surface_y(mx, mz);
                if (mgap < 0.0f) hi = mid;
                else             lo = mid;
            }

            float t_hit = 0.5f * (lo + hi);
            if (t_hit > HFEPS && t_hit < closest.t_min)
            {
                closest.t_min         = t_hit;
                closest.geometry_node = this;
                closest.material_node = current_state.material_node;
                closest.texture_node  = current_state.texture_node;
                closest.normal_map_node = current_state.normal_map_node;
                if (current_state.transform_required)
                {
                    closest.transform_required = true;
                    closest.inverse_matrix     = current_state.inverse_matrix;
                    closest.normal_matrix      = current_state.normal_matrix;
                }
            }
            return;
        }

        // Adaptive step: shrink when close to the surface.
        if (gap < dt * 2.0f)
            dt = std::max(gap * 0.4f, XZ_STEP * 0.1f / std::max(xz_speed, 0.02f));

        t_prev   = t;
        gap_prev = gap;
        (void)gap_prev; // suppress unused warning
    }
    // No intersection found within MAX_T
}

bool HeightFieldFloorNode::does_intersect_exist(Ray3        ray,
                                                 float       d,
                                                 SceneState &current_state)
{
    // Don't shadow-test against ourselves
    if (this == current_state.geometry_node)
        return false;

    float ceil_y = base_y_ + max_height_;

    // Shadow ray going up from above the floor → can't hit
    if (ray.d.y >= 0.0f && ray.o.y >= ceil_y)
        return false;

    float t_start = HFEPS;
    if (ray.o.y > ceil_y)
    {
        if (ray.d.y >= 0.0f) return false;
        t_start = (ray.o.y - ceil_y) / (-ray.d.y);
    }

    float xz_speed = sqrtf(ray.d.x * ray.d.x + ray.d.z * ray.d.z);
    float dt = (xz_speed > 0.02f) ? (XZ_STEP / xz_speed) : 0.15f;

    float t_prev = t_start;
    for (float t = t_start + dt; t < d && t < MAX_T; t += dt)
    {
        float px  = ray.o.x + t * ray.d.x;
        float py  = ray.o.y + t * ray.d.y;
        float pz  = ray.o.z + t * ray.d.z;

        if (py < surface_y(px, pz))
            return true;

        t_prev = t;
        (void)t_prev;
    }
    return false;
}

// ── Surface properties ────────────────────────────────────────────────────

Vector3 HeightFieldFloorNode::get_normal(const Point3 &hit_pt)
{
    const float eps = 0.15f;
    float h0 = surface_y(hit_pt.x,       hit_pt.z      );
    float hx = surface_y(hit_pt.x + eps, hit_pt.z      );
    float hz = surface_y(hit_pt.x,       hit_pt.z + eps);

    // Tangent vectors along the surface:
    //   Tu = (eps, hx-h0, 0)    Tv = (0, hz-h0, eps)
    // Normal = Tu × Tv = ( -(hx-h0), eps², -(hz-h0) ), simplified to:
    Vector3 n(-(hx - h0) / eps, 1.0f, -(hz - h0) / eps);
    n.normalize();
    return n;
}

Point2 HeightFieldFloorNode::get_texture_coord(const Point3 &hit_pt)
{
    // Simple planar mapping: scale XZ to a [0,1] range over the visible scene
    const float uv_scale = 40.0f;
    return Point2(hit_pt.x / uv_scale + 0.5f,
                  hit_pt.z / uv_scale + 0.5f);
}

Vector3 HeightFieldFloorNode::get_tangent(const Point3 &hit_pt)
{
    // Tangent = dF/dx = (1, dh/dx, 0) normalised.
    // Use a finite difference to match get_normal's eps exactly.
    const float eps = 0.15f;
    float dh_dx = (surface_y(hit_pt.x + eps, hit_pt.z) -
                   surface_y(hit_pt.x,        hit_pt.z)) / eps;
    Vector3 t(1.0f, dh_dx, 0.0f);
    t.normalize();
    return t;
}

} // namespace cg
