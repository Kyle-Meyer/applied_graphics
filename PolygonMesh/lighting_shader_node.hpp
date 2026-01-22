//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:	David W. Nesbitt
//	File:    lighting_shader_node.hpp
//	Purpose:	Derived class to handle an offset line shader and its uniforms and
//          attribute locations.
//
//============================================================================

#ifndef __POLYGON_MESH_LIGHTING_SHADER_NODE_HPP__
#define __POLYGON_MESH_LIGHTING_SHADER_NODE_HPP__

#include "scene/color4.hpp"
#include "scene/shader_node.hpp"

#include <array>

namespace cg
{

/**
 * Lighting shader node.
 */
class LightingShaderNode : public ShaderNode
{
  public:
    /**
     * Gets uniform and attribute locations.
     */
    bool get_locations() override;

    /**
     * Draw method for this shader - enable the program and set up uniforms
     * and vertex attribute locations
     * @param  scene_state   Current scene state.
     */
    void draw(SceneState &scene_state) override;

    /**
     * Set the lighting
     */
    void SetGlobalAmbient(const Color4 &global_ambient);

    /**
     * Set light (used for Lighting and Viewing HW)
     */
    void SetLight(const uint32_t n,
                  const HPoint3 &position,
                  const Color4  &ambient,
                  const Color4  &diffuse,
                  const Color4  &specular);

    /**
     * Get the location of the vertex position attribute.
     * @return  Returns the vertex position attribute location.
     */
    int GetPositionLoc() const;

    /**
     * Get the location of the vertex position attribute.
     * @return  Returns the vertex position attribute location.
     */
    int GetNormalLoc() const;

    /**
     * Get the location of the vertex texture attribute.
     * @return  Returns the vertex texture attribute location.
     */
    int GetTextureLoc() const;

  protected:
    // Uniform and attribute locations
    GLint position_loc_;
    GLint normal_loc_;
    GLint texture_loc_;
    GLint pvm_matrix_loc_;
    GLint model_matrix_loc_;
    GLint normal_matrix_loc_;
    GLint material_ambient_loc_;
    GLint material_diffuse_loc_;
    GLint material_specular_loc_;
    GLint material_emission_loc_;
    GLint material_shininess_loc_;
    GLint camera_position_loc_;
    GLint global_ambient_loc_;
    GLint use_texture_loc_;
    GLint texture_unit_loc_;

    uint32_t                     light_count_;
    GLint                        light_count_loc_;
    std::array<LightUniforms, 3> lights_;
};

} // namespace cg

#endif
