//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    procedural_texture.hpp
//	Purpose: Base class for defining procedural textures
//
//============================================================================

#ifndef __RAY_TRACER_PROCEDURAL_TEXTURE_HPP__
#define __RAY_TRACER_PROCEDURAL_TEXTURE_HPP__

#include "geometry/point3.hpp"
#include "scene/color3.hpp"
#include "scene/scene_node.hpp"

namespace cg
{

/**
 * Base class for procedural textures. Derived classes can implement specific
 * produral texture generation methods
 */
class ProceduralTexture : public SceneNode
{
  public:
    ProceduralTexture();

    /**
     * Get the color based on a 3D point. Uses a procedural method.
     * @param   p   Point to compute color
     * @return  Returns the color assigned to this point
     */
    virtual Color3 get_color(const Point3 &p) = 0;
};

} // namespace cg

#endif
