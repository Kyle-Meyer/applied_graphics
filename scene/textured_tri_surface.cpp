#include "scene/textured_tri_surface.hpp"

namespace cg
{

TexturedTriSurface::TexturedTriSurface()
{
    vao_ = 0;
    vbo_ = 0;
    facebuffer_ = 0;
}

TexturedTriSurface::~TexturedTriSurface()
{
    // Delete vertex buffer objects
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &facebuffer_);
    glDeleteVertexArrays(1, &vao_);
}

void TexturedTriSurface::draw(SceneState &scene_state)
{
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, (GLsizei)face_count_, GL_UNSIGNED_SHORT, (void *)0);
    glBindVertexArray(0);

    // Disable texture vertex attribute
    glDisableVertexAttribArray(scene_state.texture_loc);
}

void TexturedTriSurface::construct(std::vector<PNTVertex> &vertex_list,
                                   std::vector<uint16_t>   face_list,
                                   int32_t                 position_loc,
                                   int32_t                 normal_loc,
                                   int32_t                 texture_loc)
{
    vertices_ = vertex_list;
    faces_ = face_list;

    // Create the vertex and face buffers
    create_vertex_buffers(position_loc, normal_loc, texture_loc);
}

void TexturedTriSurface::end(int32_t position_loc, int32_t normal_loc, int32_t texture_loc)
{
    // Iterate through the face list and calculate the normals for each
    // face and add the normal to each vertex in the face list. This
    // assumes the vertex normals are initilaized to 0 (in constructor
    // of VertexAndNormal)
    uint32_t v0, v1, v2;
    Vector3  e1, e2, face_normal;
    auto     face_vertex = faces_.begin();
    while(face_vertex != faces_.end())
    {
        // Get the vertices of the face (assumes ccw order)
        v0 = *face_vertex++;
        v1 = *face_vertex++;
        v2 = *face_vertex++;

        // Calculate surface normal. Normalize it since cross products
        // do not ensure unit length normals. We need to make sure normals
        // are same length so averaging works properly
        e1.set(vertices_[v0].vertex, vertices_[v1].vertex);
        e2.set(vertices_[v0].vertex, vertices_[v2].vertex);
        face_normal = (e1.cross(e2)).normalize();

        // Add the face normal to the vertex normals of the triangle
        vertices_[v0].normal += face_normal;
        vertices_[v1].normal += face_normal;
        vertices_[v2].normal += face_normal;
    }

    // Normalize the vertex normals - this essentially averages the
    // adjoining face normals.
    for(auto v : vertices_) { v.normal.normalize(); }

    // Create the vertex and face buffers
    create_vertex_buffers(position_loc, normal_loc, texture_loc);
}

void TexturedTriSurface::construct_row_col_face_list(uint32_t num_rows, uint32_t num_cols)
{
    for(uint32_t row = 0; row < num_rows - 1; row++)
    {
        for(uint32_t col = 0; col < num_cols - 1; col++)
        {
            // Divide each square into 2 triangles - make sure they are ccw.
            // GL_TRIANGLES draws independent triangles for each set of 3 vertices
            faces_.push_back(get_index(row + 1, col, num_cols));
            faces_.push_back(get_index(row, col, num_cols));
            faces_.push_back(get_index(row, col + 1, num_cols));

            faces_.push_back(get_index(row + 1, col, num_cols));
            faces_.push_back(get_index(row, col + 1, num_cols));
            faces_.push_back(get_index(row + 1, col + 1, num_cols));
        }
    }
}

uint16_t TexturedTriSurface::get_index(uint32_t row, uint32_t col, uint32_t num_cols) const
{
    return static_cast<uint16_t>((row * num_cols) + col);
}

void TexturedTriSurface::create_vertex_buffers(int32_t position_loc,
                                               int32_t normal_loc,
                                               int32_t texture_loc)
{
    // Generate vertex buffers for the vertex list and the face list
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &facebuffer_);

    // Bind the vertex list to the vertex buffer object
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices_.size() * sizeof(PNTVertex),
                 (void *)&vertices_[0],
                 GL_STATIC_DRAW);

    // Bind the face list to the vertex buffer object
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, facebuffer_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 faces_.size() * sizeof(uint16_t),
                 (void *)&faces_[0],
                 GL_STATIC_DRAW);

    // Copy the face list count for use in Draw (then we can clear the vector)
    face_count_ = static_cast<GLsizei>(faces_.size());

    // We could clear any local memory as it is now in the VBO. However there may be
    // cases where we want to keep it (e.g. collision detection, picking) so I am not
    // going to do that here.

    // Allocate a VAO, enable it and set the vertex attribute arrays and pointers
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    // Bind the vertex buffer, set the vertex position attribute and the vertex normal attribute
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(PNTVertex), (void *)0);
    glVertexAttribPointer(
        normal_loc, 3, GL_FLOAT, GL_FALSE, sizeof(PNTVertex), (void *)(sizeof(Point3)));
    glVertexAttribPointer(texture_loc,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(PNTVertex),
                          (void *)(sizeof(Point3) + sizeof(Vector3)));
    glEnableVertexAttribArray(position_loc);
    glEnableVertexAttribArray(normal_loc);
    glEnableVertexAttribArray(texture_loc);

    // Bind the face list buffer and draw. Note the use of 0 offset in glDrawElements
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, facebuffer_);

    // Make sure changes to this VAO are local
    glBindVertexArray(0);
}

} // namespace cg
