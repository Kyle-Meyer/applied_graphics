//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    unit_subdivided_sphere.hpp
//	Purpose: Scene graph geometry node representing a unit sphere. Constructed
//          using subdivision of a tetrahedron.
//
//============================================================================

#ifndef __POLYGON_MESH_UNIT_SUBDIVIDED_SPHERE_HPP__
#define __POLYGON_MESH_UNIT_SUBDIVIDED_SPHERE_HPP__

#include "geometry/point3.hpp"
#include "scene/tri_surface.hpp"

namespace cg
{

struct CTriangle
{
    Point3 p1;
    Point3 p2;
    Point3 p3;

    CTriangle(const Point3 &ip1, const Point3 &ip2, const Point3 &ip3);

    // Normalize - make each vertex a unit radius from the origin
    void normalize();
};

class UnitSubdividedSphere : public TriSurface
{
  public:
    /**
     * Creates a unit sphere using a subdivision of a tetrahedron.
     * @param  iterations  Number of subdivisions
     */
    UnitSubdividedSphere(uint32_t iterations, int32_t position_loc, int32_t normal_loc);
    ;

  protected:
    // Make default constructor private to force use of the constructor
    // with number of subdivisions.
    UnitSubdividedSphere();
};

} // namespace cg

#endif
