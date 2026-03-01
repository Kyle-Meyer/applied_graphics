#include "scene/bezier_patch.hpp"

#include <cmath>

namespace cg
{

// ---------------------------------------------------------------------------
// Bernstein basis functions and their derivatives
// ---------------------------------------------------------------------------

/**
 * Cubic Bernstein basis polynomial B_i^3(t):
 *   B0 = (1-t)^3
 *   B1 = 3t(1-t)^2
 *   B2 = 3t^2(1-t)
 *   B3 = t^3
 */
float BezierPatchSurface::bernstein(int i, float t)
{
    float omt = 1.0f - t;
    switch (i)
    {
        case 0: return omt * omt * omt;
        case 1: return 3.0f * t * omt * omt;
        case 2: return 3.0f * t * t * omt;
        case 3: return t * t * t;
        default: return 0.0f;
    }
}

/**
 * Derivative dB_i^3/dt:
 *   dB0/dt = -3(1-t)^2
 *   dB1/dt = 3(1-t)(1-3t)
 *   dB2/dt = 3t(2-3t)
 *   dB3/dt = 3t^2
 */
float BezierPatchSurface::bernstein_deriv(int i, float t)
{
    float omt = 1.0f - t;
    switch (i)
    {
        case 0: return -3.0f * omt * omt;
        case 1: return  3.0f * omt * (1.0f - 3.0f * t);
        case 2: return  3.0f * t   * (2.0f - 3.0f * t);
        case 3: return  3.0f * t   * t;
        default: return 0.0f;
    }
}

// ---------------------------------------------------------------------------
// Patch evaluation
// ---------------------------------------------------------------------------

/**
 * Evaluate the surface point S(u,v) = sum_i sum_j B_i(u) * B_j(v) * P[i][j].
 * cp[i*4+j] = control point at (u_i, v_j).
 */
Point3 BezierPatchSurface::evaluate(float u, float v, const std::array<Point3, 16> &cp)
{
    Point3 result(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; ++i)
    {
        float bu = bernstein(i, u);
        for (int j = 0; j < 4; ++j)
        {
            float w = bu * bernstein(j, v);
            result.x += w * cp[i * 4 + j].x;
            result.y += w * cp[i * 4 + j].y;
            result.z += w * cp[i * 4 + j].z;
        }
    }
    return result;
}

/**
 * Partial derivative dS/du = sum_i sum_j B'_i(u) * B_j(v) * P[i][j].
 */
Vector3 BezierPatchSurface::compute_du(float u, float v, const std::array<Point3, 16> &cp)
{
    Vector3 result(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; ++i)
    {
        float dbu = bernstein_deriv(i, u);
        for (int j = 0; j < 4; ++j)
        {
            float w = dbu * bernstein(j, v);
            result.x += w * cp[i * 4 + j].x;
            result.y += w * cp[i * 4 + j].y;
            result.z += w * cp[i * 4 + j].z;
        }
    }
    return result;
}

/**
 * Partial derivative dS/dv = sum_i sum_j B_i(u) * B'_j(v) * P[i][j].
 */
Vector3 BezierPatchSurface::compute_dv(float u, float v, const std::array<Point3, 16> &cp)
{
    Vector3 result(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; ++i)
    {
        float bu = bernstein(i, u);
        for (int j = 0; j < 4; ++j)
        {
            float w = bu * bernstein_deriv(j, v);
            result.x += w * cp[i * 4 + j].x;
            result.y += w * cp[i * 4 + j].y;
            result.z += w * cp[i * 4 + j].z;
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Static tessellation helper
// ---------------------------------------------------------------------------

/**
 * Generate vertex and face lists without uploading to the GPU.
 *
 * Vertex ordering: outer loop over v (rows), inner loop over u (cols).
 * This ordering makes du x dv consistent with the CCW face winding built
 * by the loop below.
 */
void BezierPatchSurface::tessellate(const std::array<Point3, 16>  &control_points,
                                    uint32_t                        subdivisions,
                                    std::vector<VertexAndNormal>   &vertices,
                                    std::vector<uint16_t>          &faces)
{
    uint32_t n    = subdivisions;
    uint32_t rows = n + 1; // v steps
    uint32_t cols = n + 1; // u steps

    vertices.clear();
    vertices.reserve(rows * cols);

    for (uint32_t vIdx = 0; vIdx <= n; ++vIdx)
    {
        float v = static_cast<float>(vIdx) / static_cast<float>(n);
        for (uint32_t uIdx = 0; uIdx <= n; ++uIdx)
        {
            float u = static_cast<float>(uIdx) / static_cast<float>(n);

            VertexAndNormal vtx;
            vtx.vertex = evaluate(u, v, control_points);

            Vector3 du = compute_du(u, v, control_points);
            Vector3 dv = compute_dv(u, v, control_points);

            vtx.normal = du.cross(dv).normalize();
            vertices.push_back(vtx);
        }
    }

    // Build the face list (same quad split used in TriSurface::construct_row_col_face_list)
    faces.clear();
    for (uint32_t row = 0; row < rows - 1; ++row)
    {
        for (uint32_t col = 0; col < cols - 1; ++col)
        {
            uint16_t bl = static_cast<uint16_t>( row      * cols + col    );
            uint16_t br = static_cast<uint16_t>( row      * cols + col + 1);
            uint16_t tl = static_cast<uint16_t>((row + 1) * cols + col    );
            uint16_t tr = static_cast<uint16_t>((row + 1) * cols + col + 1);

            // Triangle 1
            faces.push_back(tl);
            faces.push_back(bl);
            faces.push_back(br);

            // Triangle 2
            faces.push_back(tl);
            faces.push_back(br);
            faces.push_back(tr);
        }
    }
}

// ---------------------------------------------------------------------------
// Constructor: tessellate and upload to GPU
// ---------------------------------------------------------------------------

BezierPatchSurface::BezierPatchSurface(const std::array<Point3, 16> &control_points,
                                       uint32_t                      subdivisions,
                                       int32_t                       position_loc,
                                       int32_t                       normal_loc)
{
    tessellate(control_points, subdivisions, vertices_, faces_);
    create_vertex_buffers(position_loc, normal_loc);
}

} // namespace cg
