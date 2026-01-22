//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    torus.hpp
//	Purpose: Scene graph geometry node representing a torus.
//
//============================================================================

#ifndef __SCENE_TORUS_HPP__
#define __SCENE_TORUS_HPP__

#include "scene/textured_tri_surface.hpp"
#include "scene/tri_surface.hpp"

namespace cg
{

class TorusSurface : public TriSurface
{
  public:
    /**
     * Creates a torus with two radii specified: the ring radius and the radius
     * of the swept circle (tube).  The number of divisions around the ring and
     * the number of divisions around the tube are specified.  Scaling of the
     * torus can be performed but will it will scale both the ring and tube
     * radius.
     * @param   ring_radius  Radius of the ring
     * @param   tube_radius  Radius of the circle swept about the ring
     * @param   num_ring     Number of divisions around the ring
     * @param   num_tube     Number of divisions around the tube
     */
    TorusSurface(float    ring_radius,
                 float    tube_radius,
                 uint32_t num_ring,
                 uint32_t num_tube,
                 int32_t  position_loc,
                 int32_t  normal_loc);

  private:
    // Make default constructor private to force use of the constructor
    // with arguments.
    TorusSurface();
};

class TexturedTorusSurface : public TexturedTriSurface
{
  public:
    /**
     * Creates a torus with two radii specified: the ring radius and the radius
     * of the swept circle (tube).  The number of divisions around the ring and
     * the number of divisions around the tube are specified.  Scaling of the
     * torus can be performed but will it will scale both the ring and tube
     * radius.
     * @param   ring_radius  Radius of the ring
     * @param   tube_radius  Radius of the circle swept about the ring
     * @param   num_ring     Number of divisions around the ring
     * @param   num_tube     Number of divisions around the tube
     */
    TexturedTorusSurface(float   ring_radius,
                         float   tube_radius,
                         int32_t num_ring,
                         int32_t num_tube,
                         int32_t position_loc,
                         int32_t normal_loc,
                         int32_t texture_loc);

  private:
    // Make default constructor private to force use of the constructor
    // with arguments.
    TexturedTorusSurface();
};

} // namespace cg

#endif
