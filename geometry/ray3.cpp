#include "geometry/ray3.hpp"

#include "geometry/geometry.hpp"

#include <cmath>

namespace cg
{

Ray3::Ray3() : o{0.0f, 0.0f, 0.0f}, d{1.0f, 0.0f, 0.0f} {}

Ray3::Ray3(const Point3 &p1, const Point3 &p2, bool normalize) : o(p1), d(p1, p2)
{
    if(normalize) d.normalize();
}

Ray3::Ray3(const Point3 &origin, const Vector3 &dir) : o(origin), d(dir) {}

Ray3::Ray3(const Point3 &origin, const Vector3 &dir, bool normalize)
{
    o = origin;
    d = dir;
    if(normalize) { d.normalize(); }
}

Ray3 Ray3::reflect(const Point3 &int_pt, const Vector3 &n) const
{
    // Required in 605.767
    return Ray3();
}

RayRefractionResult Ray3::refract(const Point3 &int_pt, Vector3 &n, float u1, float u2) const
{
    // Snell's law: n1 * sin(theta1) = n2 * sin(theta2)
    // Refracted direction: T = eta * I + (eta * cos_i - cos_t) * N
    // where eta = n1/n2

    float eta = u1 / u2;
    float n_dot_d = n.dot(d);
    float cos_i = -n_dot_d;

    // Ensure cos_i is positive (handle floating point precision issues)
    if (cos_i < 0.0f)
    {
        // Normal is pointing wrong way, flip it
        n = n * -1.0f;
        cos_i = -cos_i;
    }

    float sin_t2 = eta * eta * (1.0f - cos_i * cos_i);

    // Check for total internal reflection
    if (sin_t2 > 1.0f)
    {
        return RayRefractionResult{Ray3(), true};
    }

    float cos_t = std::sqrt(1.0f - sin_t2);

    // Compute refracted direction
    Vector3 refract_dir = d * eta + n * (eta * cos_i - cos_t);
    refract_dir.normalize();

    // Offset origin slightly in refracted direction to avoid self-intersection
    Point3 refract_origin = int_pt + refract_dir * EPSILON;

    return RayRefractionResult{Ray3(refract_origin, refract_dir), false};
}

Point3 Ray3::intersect(const float t) const { return o + d * t; }

RayObjectIntersectResult Ray3::intersect(const Plane &p) const
{
    // Get plane normal
    Vector3 n = p.get_normal();
    
    // Calculate denominator: n · d (normal dot ray direction)
    float denom = n.dot(d);
    
    // Check if ray is parallel to plane (or nearly parallel)
    if (std::abs(denom) < EPSILON)
    {
        return {false, 0.0f};
    }
    
    // Calculate t = (d - n · O) / (n · D)
    // where p.solve(o) = n · O - d
    float t = -p.solve(o) / denom;
    
    // Check if intersection is behind the ray origin
    if (t < 0.0f)
    {
        return {false, 0.0f};
    }
    
    return {true, t};
}
RayObjectIntersectResult Ray3::intersect(const BoundingSphere &sphere) const
{
    // Construct vector from ray origin to sphere center. Get squared length
    Vector3 l = sphere.center - o;
    float   l2 = l.norm_squared();

    // Component of l onto ray. Since the ray direction is unit length, the
    // component is the distance along the ray to the closest point to the
    // sphere (perp.)
    float s = l.dot(d);
    float r2 = sphere.radius * sphere.radius;
    if(s < 0.0f && l2 > r2)
    {
        // Early exit - no intersection. Sphere center is behind ray origin
        // AND ray origin is outside sphere
        return {false, 0.0f};
    }

    // Distance (squared) of closest point along the ray (perpendicular)
    float m2 = l2 - (s * s);
    if(m2 > r2)
    {
        // Ray passes outside the sphere
        return {false, 0.0f};
    }

    // Intersection occurs. Use Pythagorean theorem to find q - distance
    // from the nearest point along the ray to the intersection point
    if(l2 - r2 > EPSILON)
    {
        // Ray origin is outside sphere: nearest intersection is at
        // value t=s-q
        return {true, s - std::sqrt(r2 - m2)};
    }
    else
    {
        // Ray origin is inside sphere nearest intersection is at
        // value t=s+q
        return {true, s + std::sqrt(r2 - m2)};
    }
}

RayObjectIntersectResult Ray3::intersect(const AABB &box) const
{
    // Unrolled slab method — avoids the 3-iteration loop with its axis-selection
    // branches (9 branch comparisons → 3 direct slab computations).
    // t_min starts at 0 so it is clamped to [0, ∞); when the ray origin is inside
    // the box all slab entry values are ≤ 0 and t_min stays 0, causing the inside
    // branch below to fire and return t_max (the exit distance).
    const Point3 bmin = box.min_pt();
    const Point3 bmax = box.max_pt();

    float t_min = 0.0f;
    float t_max = std::numeric_limits<float>::max();

    // X slab
    if (std::abs(d.x) < EPSILON) {
        if (o.x < bmin.x || o.x > bmax.x) return {false, 0.0f};
    } else {
        float inv = 1.0f / d.x;
        float t1 = (bmin.x - o.x) * inv;
        float t2 = (bmax.x - o.x) * inv;
        if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
        t_min = std::max(t_min, t1);
        t_max = std::min(t_max, t2);
        if (t_min > t_max) return {false, 0.0f};
    }

    // Y slab
    if (std::abs(d.y) < EPSILON) {
        if (o.y < bmin.y || o.y > bmax.y) return {false, 0.0f};
    } else {
        float inv = 1.0f / d.y;
        float t1 = (bmin.y - o.y) * inv;
        float t2 = (bmax.y - o.y) * inv;
        if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
        t_min = std::max(t_min, t1);
        t_max = std::min(t_max, t2);
        if (t_min > t_max) return {false, 0.0f};
    }

    // Z slab
    if (std::abs(d.z) < EPSILON) {
        if (o.z < bmin.z || o.z > bmax.z) return {false, 0.0f};
    } else {
        float inv = 1.0f / d.z;
        float t1 = (bmin.z - o.z) * inv;
        float t2 = (bmax.z - o.z) * inv;
        if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
        t_min = std::max(t_min, t1);
        t_max = std::min(t_max, t2);
        if (t_min > t_max) return {false, 0.0f};
    }

    // t_min == 0 means origin is inside the box (all entry slabs were ≤ 0);
    // return t_max (exit distance) in that case.
    if (t_min == 0.0f)
        return (t_max >= 0.0f) ? RayObjectIntersectResult{true, t_max}
                               : RayObjectIntersectResult{false, 0.0f};

    return {true, t_min};
}

RayObjectIntersectResult Ray3::intersect(const std::vector<Point3> &polygon,
                                         const Vector3             &normal) const
{
    // Required in 605.767
    return {false, 0.0f};
}

RayTriangleIntersectResult
    Ray3::intersect(const Point3 &v0, const Point3 &v1, const Point3 &v2) const
{
   // Möller-Trumbore algorithm
    // Edge vectors
    Vector3 edge1(v0, v1);
    Vector3 edge2(v0, v2);
    
    // Begin calculating determinant - also used to calculate u parameter
    Vector3 pvec = d.cross(edge2);
    
    // If determinant is near zero, ray lies in plane of triangle
    float det = edge1.dot(pvec);

    // Double-sided M-T: handles both CCW (det>0) and CW (det<0) wound triangles.
    // Reject only degenerate/parallel triangles. u/v/t are multiplied by sign(det)
    // so the bounds checks remain sign-independent.
    float abs_det = std::abs(det);
    if (abs_det < 1e-8f)
    {
        return {false, 0.0f, 0.0f, 0.0f};
    }

    float sign_det = (det > 0.0f) ? 1.0f : -1.0f;
    float inv_abs_det = 1.0f / abs_det;

    // Calculate distance from v0 to ray origin
    Vector3 tvec(v0, o);

    // u = (tvec · pvec) * sign(det), checked against [0, abs_det]
    float u = tvec.dot(pvec) * sign_det;
    if (u < 0.0f || u > abs_det)
    {
        return {false, 0.0f, 0.0f, 0.0f};
    }

    Vector3 qvec = tvec.cross(edge1);

    // v = (d · qvec) * sign(det)
    float v = d.dot(qvec) * sign_det;
    if (v < 0.0f || u + v > abs_det)
    {
        return {false, 0.0f, 0.0f, 0.0f};
    }

    // Normalize to barycentric coords and compute t
    u *= inv_abs_det;
    v *= inv_abs_det;
    float t = edge2.dot(qvec) * sign_det * inv_abs_det;
    
    if (t < 0.0f)
    {
        return {false, 0.0f, 0.0f, 0.0f};
    }
    
    return {true, t, u, v};
}

bool Ray3::does_intersect_exist(const Point3 &v0, const Point3 &v1, const Point3 &v2) const
{
    // Required in 605.767
    return false;
}

RayMeshIntersectResult Ray3::intersect(const std::vector<Point3>   &vertex_list,
                                       const std::vector<uint16_t> &face_list,
                                       float                        t_min) const
{
    // Required in 605.767
    return {false, 0.0f, 0.0f, 0.0f, 0};
}

bool Ray3::does_intersect_exist(const std::vector<Point3>   &vertex_list,
                                const std::vector<uint16_t> &face_list,
                                float                        t_min) const
{
    // Required in 605.767
    return false;
}

bool Ray3::does_intersect_exist(const std::vector<VertexAndNormal> &vertex_list,
                                const std::vector<uint16_t>        &face_list,
                                float                               t_min) const
{
    // Required in 605.767
    return false;
}

} // namespace cg
