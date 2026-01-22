//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:	David W. Nesbitt
//	File:    unittrough.hpp
//	Purpose: Scene graph geometry node representing a circular trough
//           (inside of a half cyclinder).
//
//============================================================================

#ifndef __POLYGON_MESH_UNIT_TROUGH_HPP__
#define __POLYGON_MESH_UNIT_TROUGH_HPP__

#include "scene/tri_surface.hpp"

namespace cg
{

/**
 * Conic surface geometry node.
 */
class UnitTrough : public TriSurface
{
  public:
    /**
     * Creates a conic surface (cone, cylinder) with specified bottom radius,
     * top radius, number of sides (how many times the top/bottom circles
     * are divided), and number of stacks (subdivisions in z).  z values range
     * from -0.5 to 0.5 (unit height). If either the bottom or top radius is 0,
     * a cone outer surface is created. If bottom_radius and top_radius are equal,
     * a cylinder surface is created. End caps are not included!
     * @param   num_sides  Number of sides (divisions of the top/bottom)
     * @param   num_stacks Number of height divisions
     */
    UnitTrough(uint32_t num_sides, uint32_t num_stacks, int32_t position_loc, int32_t normal_loc);

    /**
     * Draw this geometry node.
     */
    void draw(SceneState &scene_state) override;

  private:
    uint32_t num_rows_;
    uint32_t num_cols_;

    // Make default constructor private to force use of the constructor
    // with number of subdivisions.
    UnitTrough();

    // Convenience method to get the index into the vertex list given the
    // "row" and "column" of the subdivision/grid
    uint32_t get_index(uint32_t row, uint32_t col) const;
};

} // namespace cg

#endif
