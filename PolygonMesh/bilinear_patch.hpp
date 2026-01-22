//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:	David W. Nesbitt
//	File:    bilinearpatch.hpp
//	Purpose: Scene graph geometry node representing a bilinear patch.
//
//============================================================================

#ifndef __POLYGON_MESH_BILINEAR_PATCH_HPP__
#define __POLYGON_MESH_BILINEAR_PATCH_HPP__

#include "geometry/point3.hpp"
#include "scene/tri_surface.hpp"

namespace cg
{

class BilinearPatch : public TriSurface
{
  public:
    /**
     * Creates a scene node representation of a bilinear patch.
     */
    BilinearPatch(const Point3 &p0,
                  const Point3 &p1,
                  const Point3 &p2,
                  const Point3 &p3,
                  int32_t       n,
                  int32_t       position_loc,
                  int32_t       normal_loc);

  private:
    // Make default constructor private to force use of the constructor
    // with number of subdivisions.
    BilinearPatch();
};

} // namespace cg

#endif
