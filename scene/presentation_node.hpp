//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    presentation_node.hpp
//	Purpose: Scene graph presentation node.
//
//============================================================================

#ifndef __SCENE_PRESENTATION_NODE_HPP__
#define __SCENE_PRESENTATION_NODE_HPP__

#include "scene/color4.hpp"
#include "scene/scene_node.hpp"

#include <string>

namespace cg
{

/**
 * Presentation node. Holds material properties.
 */
class PresentationNode : public SceneNode
{
  public:
    /**
     * Constructor
     */
    PresentationNode();

    /**
     * Constructor given material properties.
     * @param  ambient  Material ambient reflection coefficients (color).
     * @param  diffuse  Material diffuse reflection coefficients (color).
     * @param  specular  Material specular reflection coefficients (color).
     * @param  emission  Material emission (color).
     * @param  shininess   Material shininess.
     */
    PresentationNode(const Color4 &ambient,
                     const Color4 &diffuse,
                     const Color4 &specular,
                     const Color4 &emission,
                     float         shininess);

    /**
     * Set material ambient reflection coefficient.
     * @param  c  Ambient reflection coefficients (color).
     */
    void set_material_ambient(const Color4 &c);

    /**
     * Set material diffuse reflection coefficient.
     * @param  c  Diffuse reflection coefficients (color).
     */
    void set_material_diffuse(const Color4 &c);

    /**
     * Set material ambient and diffuse reflection coefficient to the
     * same value.
     * @param  c  Ambient/diffuse reflection coefficients (color).
     */
    void set_material_ambient_and_diffuse(const Color4 &c);

    /**
     * Set material specular reflection coefficient.
     * @param  c  Specular reflection coefficients (color).
     */
    void set_material_specular(const Color4 &c);

    /**
     * Set material emission.
     * @param  c  Emission (color).
     */
    void set_material_emission(const Color4 &c);

    /**
     * Set material shininess.
     * @param  s  Shininess.
     */
    void set_material_shininess(float s);

    /**
     * Set the texture to use for the material.
     * @param  fname  Texture image filename
     * @param  s_wrap  OpenGL wrap option (s)
     * @param  t_wrap  OpenGL wrap option (t)
     * @param  min_filter  OpenGL filter to use for minification
     * @param  mag_filter  OpenGL filter to use for magnification
     */
    void set_texture(const std::string &fname,
                     GLuint             s_wrap,
                     GLuint             t_wrap,
                     GLuint             min_filter,
                     GLuint             mag_filter);

    /**
     * Update texture filtering for this material
     * @param  min_filter  OpenGL filter to use for minification
     * @param  mag_filter  OpenGL filter to use for magnification
     */
    void update_texture_filters(GLuint min_filter, GLuint mag_filter);

    /**
     * Draw. Sets the material properties.
     * @param  scene_state  Scene state (holds material uniform locations)
     */
    void draw(SceneState &scene_state) override;

  protected:
    Color4  material_ambient_;
    Color4  material_diffuse_;
    Color4  material_specular_;
    Color4  material_emission_;
    GLfloat material_shininess_;
    GLuint  texture_id_;
};

} // namespace cg

#endif
