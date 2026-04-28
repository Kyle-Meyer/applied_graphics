#include "RayTracer/stone_normal_node.hpp"

#include <cmath>

namespace cg
{

StoneNormalNode::StoneNormalNode(float scale, int octaves, float strength)
    : NormalMapNode(strength), scale_(scale), octaves_(octaves)
{
}

float StoneNormalNode::hash(float x, float y, float z)
{
    float v = std::sin(x * 127.1f + y * 311.7f + z * 74.7f) * 43758.5453f;
    return v - std::floor(v);
}

float StoneNormalNode::vnoise(float x, float y, float z)
{
    float ix = std::floor(x), iy = std::floor(y), iz = std::floor(z);
    float fx = x - ix,        fy = y - iy,        fz = z - iz;

    // Hermite smoothstep
    float ux = fx * fx * (3.0f - 2.0f * fx);
    float uy = fy * fy * (3.0f - 2.0f * fy);
    float uz = fz * fz * (3.0f - 2.0f * fz);

    auto lerp = [](float a, float b, float t){ return a + t * (b - a); };

    float v000 = hash(ix,     iy,     iz    );
    float v100 = hash(ix+1.f, iy,     iz    );
    float v010 = hash(ix,     iy+1.f, iz    );
    float v110 = hash(ix+1.f, iy+1.f, iz    );
    float v001 = hash(ix,     iy,     iz+1.f);
    float v101 = hash(ix+1.f, iy,     iz+1.f);
    float v011 = hash(ix,     iy+1.f, iz+1.f);
    float v111 = hash(ix+1.f, iy+1.f, iz+1.f);

    return lerp(lerp(lerp(v000, v100, ux), lerp(v010, v110, ux), uy),
                lerp(lerp(v001, v101, ux), lerp(v011, v111, ux), uy), uz);
}

float StoneNormalNode::fbm(float x, float y, float z) const
{
    float value = 0.0f, amplitude = 1.0f, total = 0.0f;
    float freq  = 1.0f;
    for (int i = 0; i < octaves_; ++i)
    {
        value    += vnoise(x * freq, y * freq, z * freq) * amplitude;
        total    += amplitude;
        amplitude *= 0.5f;
        freq      *= 2.0f;
    }
    return (total > 0.0f) ? (value / total) : 0.0f;
}

Vector3 StoneNormalNode::sample(const Point2 & /*uv*/, const Point3 &world_pos, float /*cam_dist*/) const
{
    const float eps = 0.04f;   // finite-difference step in world units
    float sx = world_pos.x * scale_;
    float sy = world_pos.y * scale_;
    float sz = world_pos.z * scale_;

    // Central differences across all three axes give smoother gradients than
    // one-sided differences, important for the visible flat shaft faces.
    float h_px = fbm(sx + eps, sy,       sz      );
    float h_nx = fbm(sx - eps, sy,       sz      );
    float h_py = fbm(sx,       sy + eps, sz      );
    float h_ny = fbm(sx,       sy - eps, sz      );

    float dx = (h_px - h_nx) / (2.0f * eps);
    float dy = (h_py - h_ny) / (2.0f * eps);

    // Tangent-space normal from height-field gradient (-dx, -dy, 1), normalised.
    float dz  = 1.0f;
    float len = std::sqrt(dx * dx + dy * dy + dz * dz);
    Vector3 tn(-dx / len, -dy / len, dz / len);

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
