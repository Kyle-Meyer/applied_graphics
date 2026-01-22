//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    material_node.hpp
//	Purpose: Scene graph material presentation node. Specialized for ray tracing.
//
//============================================================================

#ifndef __RAY_TRACER_MATERIAL_NODE_HPP__
#define __RAY_TRACER_MATERIAL_NODE_HPP__

#include "scene/color4.hpp"
#include "scene/scene_node.hpp"

namespace cg
{

/**
 * Material presentation node to support ray tracing. Applies material.
 */
class MaterialNode : public SceneNode
{
  public:
    /**
     * Constructor
     */
    MaterialNode();

    /**
     * Set material ambient reflection coefficient.
     * @param  c  Color specifying ambient reflection coefficient
     */
    void set_ambient(const Color4 &c);

    /**
     * Gets the ambient reflection coefficient.
     * @return   Returns the ambient reflection coefficient.
     */
    const Color4 &get_ambient() const;

    /**
     * Set material diffuse reflection coefficient.
     * @param  c  Color specifying diffuse reflection coefficient
     */
    void set_diffuse(const Color4 &c);

    /**
     * Gets the diffuse reflection coefficient.
     * @return   Returns the diffuse reflection coefficient.
     */
    const Color4 &get_diffuse() const;

    /**
     * Set material ambient and diffuse reflection coefficients.
     * @param  c  Color specifying ambient and diffuse reflection coefficients
     */
    void set_ambient_and_diffuse(const Color4 &c);

    /**
     * Set material specular reflection coefficient.
     * @param  c  Color specifying specular reflection coefficient
     */
    void set_specular(const Color4 &c);

    /**
     * Gets the specular reflection coefficient.
     * @return   Returns the specular reflection coefficient.
     */
    const Color4 &get_specular() const;

    /**
     * Set material emission.
     * @param  c  Color specifying material emission.
     */
    void set_emission(const Color4 &c);

    /**
     * Gets the material emmission property.
     * @return   Returns the material emmission.
     */
    const Color4 &get_emission() const;

    /**
     * Set material shininess.
     * @param  s  Shininess - generally in the range 0-128.
     */
    void set_shininess(float s);

    /**
     * Gets the material shininess.
     * @return   Returns the material shininess.
     */
    float get_shininess() const;

    /**
     * Sets the material global reflection coefficients.
     * @param    r   Percentage of red light globally reflected
     * @param    g   Percentage of green light globally reflected
     * @param    b   Percentage of blue light globally reflected
     */
    void set_global_reflectivity(float r, float g, float b);

    /**
     * Gets the global reflection coefficients.
     * @return   Returns the global reflection coefficients.
     */
    const Color3 &get_global_reflectivity() const;

    /**
     * Sets the material global transmission coefficients.
     * @param    r   Percentage of red light globally transmitted
     * @param    g   Percentage of green light globally transmitted
     * @param    b   Percentage of blue light globally transmitted
     */
    void set_global_transmission(float r, float g, float b);

    /**
     * Gets the global transmission coefficients.
     * @return  Returns the global reflection coefficients.
     */
    const Color3 &get_global_transmission(void) const;

    /**
     * Sets the material index of refraction.
     * @param   r   Index of refraction.
     */
    void set_index_of_refraction(float r);

    /**
     * Gets the material index of refraction.
     * @return   Returns the material index of refraction.
     */
    float get_index_of_refraction() const;

    /**
     * Is the material reflective
     * @return  Retutrns true if the material is reflective, false if not.
     */
    bool is_reflective() const;

    /**
     * Is the material transparent
     * @return  Returns true if the material is transparent, false if not.
     */
    bool is_transparent() const;

    /**
     * Draw. Simply sets the material properties. Later on when we
     * add more attribute control probably want to push/pop attributes
     */
    void draw(SceneState &scene_state) override;

    /**
     * Intersection method (for picking or ray tracing)
     * @param  ray   Ray to intersect with geometry. Note that this is not
     *               passed by reference, as transformation to the ray can
     *               occur at certain levels of the tree
     * @param  current_state  Current state, updated as the scene is traversed
     * @param  closest        Closest object information
     */
    void find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest) override;

  protected:
    bool    reflective_;
    bool    transparent_;
    Color4  ambient_;
    Color4  diffuse_;
    Color4  specular_;
    Color4  emission_;
    GLfloat shininess_;
    Color3  global_reflection_;   // Global reflected attenuation term
    Color3  global_refraction_;   // Global transmitted attenuation term
    float   index_of_refraction_; // Index of refraction
};

} // namespace cg

#endif
