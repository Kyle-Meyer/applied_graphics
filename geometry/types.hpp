//============================================================================
//	Johns Hopkins University Engineering for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  Brian Russin
//	File:    structs.hpp
//	Purpose: Helper stucts to group vertex attributes
//
//============================================================================

#ifndef __GEOMETRY_TYPES_HPP__
#define __GEOMETRY_TYPES_HPP__

#include "geometry/point2.hpp"
#include "geometry/point3.hpp"
#include "geometry/vector3.hpp"

namespace cg
{

/**
 * Structure to hold a vertex position and normal
 */
struct VertexAndNormal
{
    Point3  vertex;
    Vector3 normal;

    VertexAndNormal();

    VertexAndNormal(const Point3 &v);
};

/**
 * Structure to hold a texture coordinate
 */
struct TextureCoord2
{
    float s, t;

    TextureCoord2();
    TextureCoord2(float s_in, float t_in);
};

/**
 * Structure to hold a vertex position, normal, and texture coordinate
 */
struct PNTVertex
{
    Point3  vertex;
    Vector3 normal;
    Point2  texture;

    PNTVertex();

    PNTVertex(const Point3 &v);

    PNTVertex(const Point3 &v, const Vector3 &n, const Point2 &tex);
};

} // namespace cg

#endif
