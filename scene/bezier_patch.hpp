#ifndef __SCENE_BEZIER_PATCH_HPP__
#define __SCENE_BEZIER_PATCH_HPP__

#include "scene/tri_surface.hpp"

#include <array>

namespace cg
{

/**
 * Bicubic Bezier patch surface.
 *
 * Defined by a 4x4 grid of 16 control points stored row-major:
 *   control_points[i*4 + j]  =  P(u_i, v_j),  i,j in {0,1,2,3}
 * where u_i = i/3 and v_j = j/3 are the parametric knot values.
 *
 * The patch is tessellated into a (subdivisions x subdivisions) quad mesh
 * and uploaded to the GPU as a TriSurface.  Analytical normals are computed
 * from the cross product of the partial derivatives dS/du x dS/dv, so
 * lighting is smooth at every subdivision level.
 *
 * Typical LOD usage:
 *   auto lod = std::make_shared<LODNode>("Patch");
 *   lod->add_level(50.0f,  std::make_shared<BezierPatchSurface>(cp, 16, pos, nrm));
 *   lod->add_level(100.0f, std::make_shared<BezierPatchSurface>(cp,  8, pos, nrm));
 *   lod->add_level(FLT_MAX, std::make_shared<BezierPatchSurface>(cp, 4, pos, nrm));
 */
class BezierPatchSurface : public TriSurface
{
  public:
    /**
     * Tessellate and upload the Bezier patch to the GPU.
     * @param control_points  4x4 control point array (row-major, u then v)
     * @param subdivisions    Number of quad divisions along each parametric axis
     * @param position_loc    Shader attribute location for vertex position
     * @param normal_loc      Shader attribute location for vertex normal
     */
    BezierPatchSurface(const std::array<Point3, 16> &control_points,
                       uint32_t                      subdivisions,
                       int32_t                       position_loc,
                       int32_t                       normal_loc);

    /**
     * Generate the vertex and face lists for a Bezier patch without uploading
     * to the GPU.  Useful for constructing ray-tracing mesh nodes.
     *
     * Vertex ordering matches the constructor: outer loop over v (rows),
     * inner loop over u (cols), so the index of vertex (uIdx, vIdx) is
     * vIdx*(subdivisions+1) + uIdx.
     *
     * @param control_points  4x4 control point array (row-major, u then v)
     * @param subdivisions    Number of quad divisions along each parametric axis
     * @param vertices        Output: vertex positions + analytical normals
     * @param faces           Output: CCW triangle index list
     */
    static void tessellate(const std::array<Point3, 16>  &control_points,
                           uint32_t                        subdivisions,
                           std::vector<VertexAndNormal>   &vertices,
                           std::vector<uint16_t>          &faces);

  private:
    // Evaluate surface point S(u,v)
    static Point3 evaluate(float u, float v, const std::array<Point3, 16> &cp);

    // Partial derivative dS/du
    static Vector3 compute_du(float u, float v, const std::array<Point3, 16> &cp);

    // Partial derivative dS/dv
    static Vector3 compute_dv(float u, float v, const std::array<Point3, 16> &cp);

    // Cubic Bernstein basis value B_i^3(t)
    static float bernstein(int i, float t);

    // Derivative of cubic Bernstein basis dB_i^3/dt
    static float bernstein_deriv(int i, float t);
};

} // namespace cg

#endif
