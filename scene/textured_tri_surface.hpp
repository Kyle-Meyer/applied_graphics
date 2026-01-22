//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:	  David W. Nesbitt
//	File:     textured_trisurface.hpp
//	Purpose:  Scene graph geometry node indicating a triangle based
//            surface with texture coordinates.
//
//============================================================================

#ifndef __SCENE_TEXTURED_TRISURFACE_HPP__
#define __SCENE_TEXTURED_TRISURFACE_HPP__

#include "scene/geometry_node.hpp"

namespace cg
{

/**
 * Textured triangle mesh surface.
 */
class TexturedTriSurface : public GeometryNode
{
  public:
    /**
     * Constructor.
     */
    TexturedTriSurface();

    /**
     * Destructor.
     */
    ~TexturedTriSurface();

    /**
     * Draw this geometry node.
     */
    void draw(SceneState &scene_state) override;

    /**
     * Construct triangle surface by passing in vertex list and face list
     * @param  vertex_list  List of vertices (position and normal)
     * @param  face_list    Index list for triangles
     * @param  position_loc Location of the vertex position attribute
     * @param  normal_loc   Location of the vertex normal attribute
     *@param   texture_loc  Location of the vertex texture attribute
     */
    void construct(std::vector<PNTVertex> &vertex_list,
                   std::vector<uint16_t>   face_list,
                   int32_t                 position_loc,
                   int32_t                 normal_loc,
                   int32_t                 texture_loc);

    /**
     * Marks the end of a triangle mesh. Calculates the vertex normals.
     */
    void end(int32_t position_loc, int32_t normal_loc, int32_t texture_loc);

    /**
     * Form triangle face indexes for a surface constructed using a double loop - one can be
     * considered rows of the surface and the other can be considered columns of the surface.
     * Assumes the vertex list is populated in row order.
     * @param  num_rows  Number of rows
     * @param  num_cols  Number of columns
     */
    void construct_row_col_face_list(uint32_t num_rows, uint32_t num_cols);

    // Convenience method to get the index into the vertex list given the
    // "row" and "column" of the subdivision/grid
    uint16_t get_index(uint32_t row, uint32_t col, uint32_t num_cols) const;

    /**
     * Creates vertex buffers for this object.
     */
    void create_vertex_buffers(int32_t position_loc, int32_t normal_loc, int32_t texture_loc);

  protected:
    // Vertex buffer support
    GLsizei face_count_;
    GLuint  vao_;
    GLuint  vbo_;
    GLuint  facebuffer_;

    // Vertex and normal list
    std::vector<PNTVertex> vertices_;

    // Use uint16_t for face list indexes (OpenGL ES compatible)
    std::vector<uint16_t> faces_;
};

} // namespace cg

#endif
