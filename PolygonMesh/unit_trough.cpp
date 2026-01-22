#include "PolygonMesh/unit_trough.hpp"

#include "geometry/geometry.hpp"

#include <cmath>

namespace cg
{

UnitTrough::UnitTrough() {}

UnitTrough::UnitTrough(uint32_t num_sides,
                       uint32_t num_stacks,
                       int32_t  position_loc,
                       int32_t  normal_loc)
{
    // Set a rotation matrix for the normals (only rotatiing 180 degrees)
    Matrix4x4 m;
    m.rotate_z(180.0f / static_cast<float>(num_sides));

    // Create a normal at theta = 0 pointing inwards.
    Vector3 n(-1.0f, 0.0f, 0.0f);

    // Create a vertex list. Note that normals can be computed based
    // on the difference in bottom and top radius. Note that we
    // change radius linearly from the bottom radius to top radius
    uint32_t        i, j;
    float           z;
    float           d_z = 1.0f / static_cast<float>(num_stacks);
    float           theta = 0.0f;
    float           d_theta = (PI) / static_cast<float>(num_sides);
    VertexAndNormal vtx;
    for(i = 0; i <= num_sides; i++, theta += d_theta)
    {
        // Compute sin,cos to use in the loop below
        float cos_theta = std::cos(theta);
        float sin_theta = std::sin(theta);

        // Set the normal for this side
        vtx.normal = n;

        // Form vertices along the side for this angle increment
        for(j = 0, z = -0.5f; j <= num_stacks; j++, z += d_z)
        {
            vtx.vertex.set(cos_theta, sin_theta, z);
            vertices_.push_back(vtx);
        }

        // Rotate the normal for next angle
        n = m * n;
    }

    // Form triangle strip face indexes.
    // Note: there are n+1 rows and n+1 columns.
    num_rows_ = num_stacks + 1;
    num_cols_ = num_sides + 1;
    for(uint32_t row = 0; row < num_rows_ - 1; row++)
    {
        // Repeat the first face index (unless the 1st row)
        if(row > 0) { faces_.push_back(get_index(row, 0)); }

        // Iterate along columns - alternate between rows
        for(uint32_t col = 0; col < num_cols_; col++)
        {
            faces_.push_back(get_index(row, col));
            faces_.push_back(get_index(row + 1, col));
        }

        // Repeat the last index along the rowm (except for last row)
        if(row < num_rows_ - 2) { faces_.push_back(get_index(row + 1, num_cols_ - 1)); }
    }

    // Create vertex buffers
    create_vertex_buffers(position_loc, normal_loc);
}

void UnitTrough::draw(SceneState &scene_state)
{
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLE_STRIP, (GLsizei)face_count_, GL_UNSIGNED_SHORT, (void *)0);
    glBindVertexArray(0);
}

uint32_t UnitTrough::get_index(uint32_t row, uint32_t col) const { return (col * num_rows_) + row; }

} // namespace cg
