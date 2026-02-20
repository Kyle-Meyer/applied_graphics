#include "scene/view_frustum.hpp"

#include <cmath>

namespace cg
{

ViewFrustum::ViewFrustum() {}

void ViewFrustum::construct(const Matrix4x4 &vp)
{
    // Gribb-Hartmann method: extract frustum planes from VP matrix rows.
    // For a point p inside the frustum, clip coords satisfy -w <= x,y,z <= w.
    // Each plane is derived from combinations of row3 +/- row_i.
    //
    // Matrix accessors: mij() where i=row, j=col.
    // Plane equation: ax + by + cz + d >= 0 for inside.
    // Our Plane::solve(p) returns ax+by+cz - stored_d, so stored_d = -d.

    // Left: row3 + row0
    Plane &left = planes_[static_cast<int>(FrustumPlane::LEFT)];
    left.a = vp.m30() + vp.m00();
    left.b = vp.m31() + vp.m01();
    left.c = vp.m32() + vp.m02();
    left.d = -(vp.m33() + vp.m03());
    left.normalize();

    // Right: row3 - row0
    Plane &right = planes_[static_cast<int>(FrustumPlane::RIGHT)];
    right.a = vp.m30() - vp.m00();
    right.b = vp.m31() - vp.m01();
    right.c = vp.m32() - vp.m02();
    right.d = -(vp.m33() - vp.m03());
    right.normalize();

    // Bottom: row3 + row1
    Plane &bottom = planes_[static_cast<int>(FrustumPlane::BOTTOM)];
    bottom.a = vp.m30() + vp.m10();
    bottom.b = vp.m31() + vp.m11();
    bottom.c = vp.m32() + vp.m12();
    bottom.d = -(vp.m33() + vp.m13());
    bottom.normalize();

    // Top: row3 - row1
    Plane &top = planes_[static_cast<int>(FrustumPlane::TOP)];
    top.a = vp.m30() - vp.m10();
    top.b = vp.m31() - vp.m11();
    top.c = vp.m32() - vp.m12();
    top.d = -(vp.m33() - vp.m13());
    top.normalize();

    // Near: row3 + row2
    Plane &near_p = planes_[static_cast<int>(FrustumPlane::NEAR_PLANE)];
    near_p.a = vp.m30() + vp.m20();
    near_p.b = vp.m31() + vp.m21();
    near_p.c = vp.m32() + vp.m22();
    near_p.d = -(vp.m33() + vp.m23());
    near_p.normalize();

    // Far: row3 - row2
    Plane &far_p = planes_[static_cast<int>(FrustumPlane::FAR_PLANE)];
    far_p.a = vp.m30() - vp.m20();
    far_p.b = vp.m31() - vp.m21();
    far_p.c = vp.m32() - vp.m22();
    far_p.d = -(vp.m33() - vp.m23());
    far_p.normalize();
}

FrustumIntersectType ViewFrustum::intersect(const BoundingSphere &sphere) const
{
    bool all_inside = true;

    for (int i = 0; i < NUM_PLANES; ++i)
    {
        float dist = planes_[i].solve(sphere.center);

        if (dist < -sphere.radius)
        {
            return FrustumIntersectType::OUTSIDE;
        }

        if (dist < sphere.radius)
        {
            all_inside = false;
        }
    }

    return all_inside ? FrustumIntersectType::INSIDE : FrustumIntersectType::INTERSECT;
}

FrustumIntersectType ViewFrustum::intersect(const AABB &box) const
{
    Point3 min_pt = box.min_pt();
    Point3 max_pt = box.max_pt();
    bool all_inside = true;

    for (int i = 0; i < NUM_PLANES; ++i)
    {
        const Plane &plane = planes_[i];
        Vector3 normal = plane.get_normal();

        // P-vertex: the corner most in the direction of the plane normal
        Point3 p_vertex(
            normal.x >= 0.0f ? max_pt.x : min_pt.x,
            normal.y >= 0.0f ? max_pt.y : min_pt.y,
            normal.z >= 0.0f ? max_pt.z : min_pt.z);

        // N-vertex: the opposite corner
        Point3 n_vertex(
            normal.x >= 0.0f ? min_pt.x : max_pt.x,
            normal.y >= 0.0f ? min_pt.y : max_pt.y,
            normal.z >= 0.0f ? min_pt.z : max_pt.z);

        // If the p-vertex is outside, the entire box is outside
        if (plane.solve(p_vertex) < 0.0f)
        {
            return FrustumIntersectType::OUTSIDE;
        }

        // If the n-vertex is outside, the box straddles this plane
        if (plane.solve(n_vertex) < 0.0f)
        {
            all_inside = false;
        }
    }

    return all_inside ? FrustumIntersectType::INSIDE : FrustumIntersectType::INTERSECT;
}

} // namespace cg
