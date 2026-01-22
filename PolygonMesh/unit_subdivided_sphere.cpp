#include "PolygonMesh/unit_subdivided_sphere.hpp"

#include "geometry/geometry.hpp"

namespace cg
{

CTriangle::CTriangle(const Point3 &ip1, const Point3 &ip2, const Point3 &ip3)
{
    p1 = ip1;
    p2 = ip2;
    p3 = ip3;
}

// Normalize - make each vertex a unit radius from the origin
void CTriangle::normalize()
{
    float scale = fast_inv_sqrt(p1.x * p1.x + p1.y * p1.y + p1.z * p1.z);
    p1.x *= scale;
    p1.y *= scale;
    p1.z *= scale;
    scale = fast_inv_sqrt(p2.x * p2.x + p2.y * p2.y + p2.z * p2.z);
    p2.x *= scale;
    p2.y *= scale;
    p2.z *= scale;
    scale = fast_inv_sqrt(p3.x * p3.x + p3.y * p3.y + p3.z * p3.z);
    p3.x *= scale;
    p3.y *= scale;
    p3.z *= scale;
}

UnitSubdividedSphere::UnitSubdividedSphere() {}

UnitSubdividedSphere::UnitSubdividedSphere(uint32_t iterations,
                                           int32_t  position_loc,
                                           int32_t  normal_loc)
{
    uint32_t i, j, n, nstart;

    // Initial shape is 4 vertices at z = 0 and a top and bottom vertex
    Point3 p1(1.0f, 0.0f, 0.0f);
    Point3 p2(0.0f, 1.0f, 0.0f);
    Point3 p3(-1.0f, 0.0f, 0.0f);
    Point3 p4(0.0f, -1.0f, 0.0f);
    Point3 p5(0.0f, 0.0f, 1.0f);
    Point3 p6(0.0f, 0.0f, -1.0f);

    CTriangle *triangle[4096];
    triangle[0] = new CTriangle(p1, p2, p5);
    triangle[1] = new CTriangle(p2, p3, p5);
    triangle[2] = new CTriangle(p3, p4, p5);
    triangle[3] = new CTriangle(p4, p1, p5);
    triangle[4] = new CTriangle(p1, p6, p2);
    triangle[5] = new CTriangle(p2, p6, p3);
    triangle[6] = new CTriangle(p3, p6, p4);
    triangle[7] = new CTriangle(p4, p6, p1);
    n = 8;
    for(i = 1; i <= iterations; ++i)
    {
        // Subdivide all the current triangles
        nstart = n;
        for(j = 0; j < nstart; ++j)
        {
            // Calculate the midpoints of the current triangle edges
            p1 = triangle[j]->p1.affine_combination(0.5f, 0.5f, triangle[j]->p2);
            p2 = triangle[j]->p2.affine_combination(0.5f, 0.5f, triangle[j]->p3);
            p3 = triangle[j]->p3.affine_combination(0.5f, 0.5f, triangle[j]->p1);

            // Each triangle creates 4 triangles (current one + 3 new ones)
            triangle[n] = new CTriangle(triangle[j]->p1, p1, p3);
            triangle[n + 1] = new CTriangle(triangle[j]->p2, p2, p1);
            triangle[n + 2] = new CTriangle(triangle[j]->p3, p3, p2);
            n += 3;

            // Replace the current triangle with the last subdivided one
            triangle[j]->p1 = p1;
            triangle[j]->p2 = p2;
            triangle[j]->p3 = p3;
        }
    }

    // Normalize all the vertices (to put them on the unit sphere)
    // and add triangles to the sphere
    for(i = 0; i < n; ++i)
    {
        triangle[i]->normalize();
        add(triangle[i]->p1, triangle[i]->p2, triangle[i]->p3);
        delete triangle[i];
    }
    end(position_loc, normal_loc);
}

} // namespace cg
