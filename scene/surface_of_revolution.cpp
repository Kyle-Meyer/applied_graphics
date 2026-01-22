#include "scene/surface_of_revolution.hpp"

#include <algorithm>

namespace cg
{

SurfaceOfRevolution::SurfaceOfRevolution() {}

SurfaceOfRevolution::SurfaceOfRevolution(std::vector<Point3> &v,
                                         uint32_t             n,
                                         int32_t              position_loc,
                                         int32_t              normal_loc)
{
    // Set number of rows and columns
    num_rows_ = static_cast<uint32_t>(v.size());
    num_cols_ = n + 1;

    // Add vertices to the vertex list, compute normals
    Vector3         normal, prev_normal;
    VertexAndNormal vtx;
    auto            vtx_iter_1 = v.begin();
    auto            vtx_iter_2 = vtx_iter_1 + 1;
    for(uint32_t i = 0; vtx_iter_2 != v.end(); vtx_iter_1++, vtx_iter_2++, i++)
    {
        normal = {vtx_iter_2->z - vtx_iter_1->z, 0.0f, vtx_iter_1->x - vtx_iter_2->x};
        normal.normalize();
        vtx.vertex = {vtx_iter_1->x, vtx_iter_1->y, vtx_iter_1->z};
        if(i == 0)
        {
            // Use normal for the first edge vertex
            vtx.normal = normal;
        }
        else
        {
            // Average normals of successive edges
            vtx.normal = (prev_normal + normal).normalize();
        }
        vertices_.push_back(vtx);

        // Copy normal for use in averaging
        prev_normal = normal;
    }

    // Store last vertex
    vtx.vertex = {vtx_iter_1->x, vtx_iter_1->y, vtx_iter_1->z};
    vtx.normal = normal;
    vertices_.push_back(vtx);

    // Reverse the vertex list so we go from top to bottom so
    // ConstructRowColFaceList forms ccw triangles
    std::reverse(vertices_.begin(), vertices_.end());

    // Create a rotation matrix
    Matrix4x4 m;
    m.rotate_z(360.0f / static_cast<float>(n));

    // Rotate the prior "edge" vertices
    uint32_t index = 0; // Index to the prior edge at this row
    for(uint32_t i = 0; i < n; i++)
    {
        for(uint32_t j = 0; j < num_rows_; j++)
        {
            // Rotate the vertex and the normal
            vtx.vertex = m * vertices_[index].vertex;
            vtx.normal = m * vertices_[index].normal;
            vertices_.push_back(vtx);
            index++;
        }
    }

    // Copy the first column of vertices
    for(uint32_t i = 0; i < num_rows_; i++) { vertices_.push_back(vertices_[i]); }

    // Construct the face list and create VBOs
    construct_row_col_face_list(num_cols_, num_rows_);
    create_vertex_buffers(position_loc, normal_loc);
}

TexturedSurfaceOfRevolution::TexturedSurfaceOfRevolution() {}

TexturedSurfaceOfRevolution::TexturedSurfaceOfRevolution(std::vector<Point3> &v,
                                                         uint32_t             n,
                                                         int32_t              position_loc,
                                                         int32_t              normal_loc,
                                                         int32_t              texture_loc)
{
    // Set number of rows and columns
    num_rows_ = static_cast<uint32_t>(v.size());
    num_cols_ = n + 1;

    // Attempt to normalize spacing in t by "delta" between points on the profile edge
    float              total_length = 0.0f;
    std::vector<float> accumulated_length;
    accumulated_length.push_back(0.0f);
    for(uint32_t i = 0; i < num_rows_ - 1; i++)
    {
        total_length += (v[i + 1] - v[i]).norm();
        accumulated_length.push_back(total_length);
    }

    // Add vertices to the vertex list, compute normals
    Vector3   normal, prev_normal;
    PNTVertex vtx;
    auto      vtx1 = v.begin();
    auto      vtx2 = vtx1 + 1;
    for(uint32_t i = 0; vtx2 != v.end(); vtx1++, vtx2++, i++)
    {
        normal = {vtx2->z - vtx1->z, 0.0f, vtx1->x - vtx2->x};
        normal.normalize();
        vtx.vertex = {vtx1->x, vtx1->y, vtx1->z};
        vtx.texture.x = 0.0f;
        vtx.texture.y = accumulated_length[i] / total_length;
        if(i == 0)
        {
            // Use normal for the first edge vertex
            vtx.normal = normal;
        }
        else
        {
            // Average normals of successive edges
            vtx.normal = (prev_normal + normal).normalize();
        }
        vertices_.push_back(vtx);

        // Copy normal for use in averaging
        prev_normal = normal;
    }

    // Store last vertex
    vtx.vertex = {vtx1->x, vtx1->y, vtx1->z};
    vtx.texture.y = 1.0f;
    vtx.normal = normal;
    vertices_.push_back(vtx);

    // Reverse the vertex list so we go from top to bottom so
    // ConstructRowColFaceList forms ccw triangles
    std::reverse(vertices_.begin(), vertices_.end());

    // Create a rotation matrix
    Matrix4x4 m;
    m.rotate_z(360.0f / static_cast<float>(n));

    // Rotate the prior "edge" vertices
    float    d_s = 1.0f / static_cast<float>(n);
    float    s = d_s;
    uint32_t index = 0; // Index to the prior edge at this row
    for(uint32_t i = 0; i < n; i++, s += d_s)
    {
        vtx.texture.x = s;
        for(uint32_t j = 0; j < num_rows_; j++)
        {
            // Rotate the vertex and the normal
            vtx.vertex = m * vertices_[index].vertex;
            vtx.normal = m * vertices_[index].normal;
            vtx.texture.y = vertices_[index].texture.y;
            vertices_.push_back(vtx);
            index++;
        }
    }

    // Copy the first column of vertices
    for(uint32_t i = 0; i < num_rows_; i++) { vertices_.push_back(vertices_[i]); }

    // Construct the face list and create VBOs
    construct_row_col_face_list(num_cols_, num_rows_);
    create_vertex_buffers(position_loc, normal_loc, texture_loc);
}

} // namespace cg
