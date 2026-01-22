#include "PolygonMesh/geodesic_dome.hpp"

#include "geometry/geometry.hpp"

namespace cg
{

namespace
{
// Convenience method to form a vertex list from a list of triangle vertices
std::vector<Point3> form_triangle(const Point3 &v0, const Point3 &v1, const Point3 &v2)
{
    return {v0, v1, v2};
}
} // namespace

GeodesicDome::GeodesicDome(int32_t position_loc, int32_t normal_loc)
{
    // Vertices
    Point3 v0(0.0f, 1.0f, PHI_INV);
    Point3 v1(0.0f, 1.0f, -PHI_INV);
    Point3 v2(1.0f, PHI_INV, 0.0f);
    Point3 v3(1.0f, -PHI_INV, 0.0f);
    Point3 v4(0.0f, -1.0f, -PHI_INV);
    Point3 v5(0.0f, -1.0f, PHI_INV);
    Point3 v6(PHI_INV, 0.0f, 1.0f);
    Point3 v7(-PHI_INV, 0.0f, 1.0f);
    Point3 v8(PHI_INV, 0.0f, -1.0f);
    Point3 v9(-PHI_INV, 0.0f, -1.0f);
    Point3 v10(-1.0f, PHI_INV, 0.0f);
    Point3 v11(-1.0f, -PHI_INV, 0.0f);

    // Triangle faces
    subdivide_face(v0, v6, v2);
    subdivide_face(v6, v3, v2);
    subdivide_face(v6, v5, v3);
    subdivide_face(v7, v5, v6);
    subdivide_face(v7, v6, v0);
    subdivide_face(v2, v3, v8);
    subdivide_face(v1, v2, v8);
    subdivide_face(v0, v2, v1);
    subdivide_face(v10, v0, v1);
    subdivide_face(v10, v1, v9);
    subdivide_face(v1, v8, v9);
    subdivide_face(v3, v4, v8);
    subdivide_face(v5, v4, v3);
    subdivide_face(v11, v4, v5);
    subdivide_face(v11, v7, v10);
    subdivide_face(v7, v0, v10);
    subdivide_face(v9, v4, v11);
    subdivide_face(v9, v8, v4);
    subdivide_face(v11, v5, v7);
    subdivide_face(v11, v10, v9);

    create_vertex_buffers(position_loc, normal_loc);
}

void GeodesicDome::subdivide_face(const Point3 &v0_in, const Point3 &v1_in, const Point3 &v2_in)
{
    Point3 v0 = v0_in;
    Point3 v1 = v1_in;
    Point3 v2 = v2_in;
    Point3 v3 = v0.affine_combination(0.666f, 0.334f, v1);
    Point3 v4 = v0.affine_combination(0.334f, 0.666f, v1);
    Point3 v5 = v0.affine_combination(0.666f, 0.334f, v2);
    Point3 v6 = v0.affine_combination(0.334f, 0.666f, v2);
    Point3 v7 = v1.affine_combination(0.666f, 0.334f, v2);
    Point3 v8 = v1.affine_combination(0.334f, 0.666f, v2);
    Point3 v9 = Point3((v0.x + v1.x + v2.x) * 0.333333f,
                       (v0.y + v1.y + v2.y) * 0.333333f,
                       (v0.z + v1.z + v2.z) * 0.333333f);

    normalize(v0);
    normalize(v1);
    normalize(v2);
    normalize(v3);
    normalize(v4);
    normalize(v5);
    normalize(v6);
    normalize(v7);
    normalize(v8);
    normalize(v9);

    // Add faces
    add_polygon(form_triangle(v0, v3, v5));
    add_polygon(form_triangle(v3, v4, v9));
    add_polygon(form_triangle(v3, v9, v5));
    add_polygon(form_triangle(v5, v9, v6));
    add_polygon(form_triangle(v4, v1, v7));
    add_polygon(form_triangle(v4, v7, v9));
    add_polygon(form_triangle(v9, v7, v8));
    add_polygon(form_triangle(v9, v8, v6));
    add_polygon(form_triangle(v6, v8, v2));
}

void GeodesicDome::normalize(Point3 &p)
{
    const float inv = fast_inv_sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
    p.x *= inv;
    p.y *= inv;
    p.z *= inv;
}

} // namespace cg
