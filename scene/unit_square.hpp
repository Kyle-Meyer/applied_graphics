//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:	 David W. Nesbitt
//	File:    unit_square.hpp
//	Purpose: Scene graph geometry node representing a subdivided unit square.
//
//============================================================================

#ifndef __SCENE_UNIT_SQUARE_HPP__
#define __SCENE_UNIT_SQUARE_HPP__

#include "scene/textured_tri_surface.hpp"
#include "scene/tri_surface.hpp"

namespace cg
{

class UnitSquareSurface : public TriSurface
{
  public:
    /**
     * Creates a unit length and width "flat surface".  The surface is composed of
     * triangles such that the unit length/width surface is divided into n
     * equal paritions in both x and y. Constructs a vertex list and face list
     * for the surface.
     * @param  n   Number of subdivisions in x and y
     */
    UnitSquareSurface(uint32_t n, int32_t position_loc, int32_t normal_loc);

  private:
    // Make default constructor private to force use of the constructor
    // with number of subdivisions.
    UnitSquareSurface();
};

class TexturedUnitSquareSurface : public TexturedTriSurface
{
  public:
    /**
     * Creates a unit length and width "flat surface".  The surface is composed of
     * triangles such that the unit length/width surface is divided into n
     * equal paritions in both x and y. Constructs a vertex list and face list
     * for the surface.
     * @param  n   Number of subdivisions in x and y
     */
    TexturedUnitSquareSurface(uint32_t n,
                              float    texture_scale,
                              int32_t  position_loc,
                              int32_t  normal_loc,
                              int32_t  texture_loc);

  private:
    // Make default constructor private to force use of the constructor
    // with number of subdivisions.
    TexturedUnitSquareSurface();
};

} // namespace cg

#endif
