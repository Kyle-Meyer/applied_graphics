//
//	Johns Hopkins University Whiting School of Engineering
//	605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:		 lighting.hpp
//	Purpose:Lighting definition classes for use in ray tracing.
//
//=============================================================================

#ifndef __RAY_TRACER_LIGHTING_HPP__
#define __RAY_TRACER_LIGHTING_HPP__

#include "RayTracer/material_node.hpp"
#include "geometry/point3.hpp"
#include "geometry/vector3.hpp"
#include "scene/color3.hpp"
#include "scene/light_node.hpp"

namespace cg
{

/**
 * Class to support lighting computations for ray tracing using Phong-Blinn model.
 * Possible extension - support other lighting/reflection models
 */
class Lighting
{
  public:
    // Constructor-sets default values
    Lighting();

    /**
     * Set the global ambient illumination (color)
     * @param  color  Global ambient illumination
     */
    void set_ambient(const Color3 &color);

    /**
     * Compute the global ambient contribution given the material.
     * @param   material_node  Pointer to the active material node
     */
    Color3 get_ambient(const MaterialNode *material_node) const;

    /**
     * Set the view position.
     * @param  pos  View (camera) position in world coordinates.
     */
    void set_view_position(const Point3 &pos);

    /**
     * Compute the local contribution given a light source, material, intersection point,
     * and normal.
     * @param  light         Pointer to the light source
     * @param  material_node Pointer to the active material node
     * @param  int_pt        Intersection point (where lighting contribution is computed)
     * @param  normal        Normal at the intersection point
     * @param  diffuse       (OUT) Diffuse component
     * @param  specular      (OUT) Specular component
     */
    void local_contribution(LightNode          *light,
                            const MaterialNode *material_node,
                            const Point3       &int_pt,
                            const Vector3      &normal,
                            Color3             &diffuse,
                            Color3             &specular) const;

  private:
    Point3 view_position_;
    Color3 ambient_;
};

} // namespace cg

#endif
