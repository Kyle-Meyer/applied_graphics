#include "PolygonMesh/bilinear_patch.hpp"

namespace cg
{

BilinearPatch::BilinearPatch() {}

BilinearPatch::BilinearPatch(const Point3 &p0,
                             const Point3 &p1,
                             const Point3 &p2,
                             const Point3 &p3,
                             int32_t       n,
                             int32_t       position_loc,
                             int32_t       normal_loc)
{
    float  a1, a2;
    Point3 v0 = p0;
    Point3 v1 = p2;
    Point3 v2, v3;
    for(int32_t i = 1; i <= n; i++)
    {
        a1 = static_cast<float>(i) / static_cast<float>(n);
        a2 = 1.0f - a1;
        v2 = p0.affine_combination(a2, a1, p1);
        v3 = p2.affine_combination(a2, a1, p3);

        // Add as 2 triangles
        add(v1, v0, v2);
        add(v1, v2, v3);

        v0 = v2;
        v1 = v3;
    }
    end(position_loc, normal_loc);
}

} // namespace cg
