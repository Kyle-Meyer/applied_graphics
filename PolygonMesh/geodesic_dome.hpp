//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:	David W. Nesbitt
//	File:    geodesicdome.hpp
//	Purpose: Scene graph geometry node representing a geodesic dome.
//
//============================================================================

#ifndef __POLYGON_MESH_GEODESIC_DOME_HPP__
#define __POLYGON_MESH_GEODESIC_DOME_HPP__

#include "geometry/point3.hpp"
#include "scene/tri_surface.hpp"

namespace cg
{

class GeodesicDome : public TriSurface
{
  public:
    /**
     * Creates a scene node representation of a geodesic dome.
     */
    GeodesicDome(int32_t position_loc, int32_t normal_loc);

  private:
    // Subdivide a face of the geodesic dome
    void subdivide_face(const Point3 &v0_in, const Point3 &v1_in, const Point3 &v2_in);

    // Place a point on a unit sphere
    void normalize(Point3 &p);
};

} // namespace cg

#endif
