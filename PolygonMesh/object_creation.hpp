//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    objectcreation.hpp
//	Purpose: Construct polyhedra and assorted objects
//
//============================================================================

#ifndef __POLYGON_MESH_OBJECT_CREATION_HPP__
#define __POLYGON_MESH_OBJECT_CREATION_HPP__

#include "geometry/point3.hpp"
#include "scene/scene_node.hpp"
#include "scene/tri_surface.hpp"

#include <memory>
#include <vector>

namespace cg
{

/**
 * Form a vertex list for a pentagon face
 */
std::vector<Point3> form_pentagon(
    const Point3 &v0, const Point3 &v1, const Point3 &v2, const Point3 &v3, const Point3 &v4);

/**
 * Find the dual (center) of a triangle face.
 * @param  v0   Vertex 0 of the triangle.
 * @param  v1   Vertex 0 of the triangle.
 * @param  v2   Vertex 0 of the triangle.
 * @return Returns the center of the triangle.
 */
Point3 dual(const Point3 &v0, const Point3 &v1, const Point3 &v2);

/**
 * Construct cube. Create a flat surface (subdivided by 5). Create
 * TransformNodes to transform for each face and add the flat surface
 * as a child of each Transform node.
 * @param  n  Number of subdivisions for faces of the cube
 */
std::shared_ptr<SceneNode>
    construct_cube(int32_t n, int32_t position_loc, int32_t normal_loc, int32_t texture_loc);

/**
 * Construct a tetrahedron
 */
std::shared_ptr<TriSurface> construct_tetrahedron(int32_t position_loc, int32_t normal_loc);

/**
 * Construct an octahedron
 */
std::shared_ptr<TriSurface> construct_octahedron(int32_t position_loc, int32_t normal_loc);

/**
 * Construct an icosahedron
 */
std::shared_ptr<TriSurface> construct_icosahedron(int32_t position_loc, int32_t normal_loc);

/**
 * Construct dodecahedron
 */
std::shared_ptr<TriSurface> construct_dodecahedron(int32_t position_loc, int32_t normal_loc);

/**
 * Form a pentagon for the buckyball
 */
std::vector<Point3> bucky_pentagon(const Point3 &v0,
                                   const Point3 &v1,
                                   const Point3 &v2,
                                   const Point3 &v3,
                                   const Point3 &v4,
                                   const Point3 &v5);

/**
 * Subdivide each edge into 3 equal pieces.  Connect to form a hexagon.
 */
std::vector<Point3> divide_face(const Point3 &v0, const Point3 &v1, const Point3 &v2);

/**
 * Construct buckyball
 */
std::shared_ptr<TriSurface> construct_buckyball(int32_t position_loc, int32_t normal_loc);

} // namespace cg

#endif
