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
    // Required in 605.767
    return RayRefractionResult{};
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
 // Slab method for ray-AABB intersection
    Point3 box_min = box.min_pt();
    Point3 box_max = box.max_pt();
    
    float t_min = 0.0f;
    float t_max = std::numeric_limits<float>::max();
    
    // Check each axis (x, y, z)
    for (int i = 0; i < 3; i++)
    {
        float ray_origin, ray_dir, box_min_i, box_max_i;
        
        if (i == 0) // X axis
        {
            ray_origin = o.x;
            ray_dir = d.x;
            box_min_i = box_min.x;
            box_max_i = box_max.x;
        }
        else if (i == 1) // Y axis
        {
            ray_origin = o.y;
            ray_dir = d.y;
            box_min_i = box_min.y;
            box_max_i = box_max.y;
        }
        else // Z axis
        {
            ray_origin = o.z;
            ray_dir = d.z;
            box_min_i = box_min.z;
            box_max_i = box_max.z;
        }
        
        if (std::abs(ray_dir) < EPSILON)
        {
            // Ray is parallel to slab
            // No intersection if origin is not within slab
            if (ray_origin < box_min_i || ray_origin > box_max_i)
            {
                return {false, 0.0f};
            }
        }
        else
        {
            // Compute intersection t values of ray with near and far plane of slab
            float inv_d = 1.0f / ray_dir;
            float t1 = (box_min_i - ray_origin) * inv_d;
            float t2 = (box_max_i - ray_origin) * inv_d;
            
            // Make t1 the intersection with near plane, t2 with far plane
            if (t1 > t2)
            {
                std::swap(t1, t2);
            }
            
            // Compute intersection of slab intersection intervals
            t_min = std::max(t_min, t1);
            t_max = std::min(t_max, t2);
            
            // Exit with no intersection as soon as slab intersection becomes empty
            if (t_min > t_max)
            {
                return {false, 0.0f};
            }
        }
    }
    
    // Ray intersects all 3 slabs
    // Return the nearest intersection (t_min)
    // But only if it's in front of the ray
    if (t_min < 0.0f)
    {
        // Ray origin is inside box, return t_max if positive
        if (t_max >= 0.0f)
        {
            return {true, t_max};
        }
        return {false, 0.0f};
    }
    
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
    
    // Use backface culling
    if (det < EPSILON)
    {
        return {false, 0.0f, 0.0f, 0.0f};
    }
    
    float inv_det = 1.0f / det;
    
    // Calculate distance from v0 to ray origin
    Vector3 tvec(v0, o);
    
    // Calculate u parameter and test bounds
    float u = tvec.dot(pvec) * inv_det;
    if (u < 0.0f || u > 1.0f)
    {
        return {false, 0.0f, 0.0f, 0.0f};
    }
    
    // Prepare to test v parameter
    Vector3 qvec = tvec.cross(edge1);
    
    // Calculate v parameter and test bounds
    float v = d.dot(qvec) * inv_det;
    if (v < 0.0f || u + v > 1.0f)
    {
        return {false, 0.0f, 0.0f, 0.0f};
    }
    
    // Calculate t - ray intersection distance
    float t = edge2.dot(qvec) * inv_det;
    
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
