//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:	 David W. Nesbitt
//	File:    scene_state.hpp
//	Purpose: Class used to propogate state during traversal of the scene graph.
//
//============================================================================

#ifndef __SCENE_SCENE_STATE_HPP__
#define __SCENE_SCENE_STATE_HPP__

#include "geometry/matrix.hpp"
#include "scene/graphics.hpp"

#include <array>
#include <list>
#include <memory>

namespace cg
{

constexpr uint32_t MAX_LIGHTS = 8;

// Forward reference
class SceneNode;

// Simple structure to hold light uniform locations
struct LightUniforms
{
    GLint enabled;
    GLint spotlight;
    GLint position;
    GLint ambient;
    GLint diffuse;
    GLint specular;
    GLint att_constant;
    GLint att_linear;
    GLint att_quadratic;
    GLint spot_cutoff;
    GLint spot_exponent;
    GLint spot_direction;
};

/**
 * Scene state structure. Used to store OpenGL state - shader locations,
 * matrices, etc.
 */
struct SceneState
{
    // Vertex attribute locations
    GLint position_loc;  // Vertex position attribute location
    GLint vtx_color_loc; // Vertex color attribute location
    GLint normal_loc;    // Vertex normal
    GLint texture_loc;   // Texture vertex attribute location

    // Uniform locations
    GLint ortho_matrix_loc;    // Orthographic projection location (2-D)
    GLint color_loc;           // Constant color
    GLint pvm_matrix_loc;      // Composite project, view, model matrix location
    GLint model_matrix_loc;    // Model matrix location
    GLint normal_matrix_loc;   // Normal matrix location
    GLint camera_position_loc; // Camera position loc

    // Material uniform locations
    GLint material_ambient_loc;   // Material ambient reflection location
    GLint material_diffuse_loc;   // Material diffuse reflection location
    GLint material_specular_loc;  // Material specular reflection location
    GLint material_emission_loc;  // Material emission location
    GLint material_shininess_loc; // Material shininess location

    // Texture mapping
    GLint use_texture_loc;
    GLint texture_unit_loc;

    // Lights
    uint32_t max_enabled_light; // Index of the maximum enabled light index
    GLint    lightcount_loc;    // Number of lights uniform
    std::array<LightUniforms, MAX_LIGHTS> lights;

    // Current matrices
    std::array<float, 16> ortho;        // Orthographic projection matrix (2-D)
    Matrix4x4             ortho_matrix; // Orthographic projection matrix (2-D)
    Matrix4x4             pv;           // Current composite projection and view matrix
    Matrix4x4             model_matrix; // Current model matrix
    Matrix4x4             normal_matrix;

    Point3 camera_position;

    // Retained state to push/pop modeling matrix
    std::list<Matrix4x4> model_matrix_stack;

    // Intersection/ray-tracing additions
    bool                                    transform_required;
    float                                   t_min;
    float                                   t_scale;
    SceneNode                              *material_node;
    SceneNode                              *geometry_node;
    SceneNode                              *texture_node;
    Matrix4x4                               inverse_matrix;
    std::list<float>                        t_scale_stack;
    std::list<SceneNode *>                  material_stack;
    std::list<SceneNode *>                  texture_stack;
    std::list<Matrix4x4>                    inverse_matrix_stack;
    std::list<Matrix4x4>                    normal_transform_stack;
    std::vector<std::shared_ptr<SceneNode>> light_nodes;

    /**
     * Initialize scene state prior to drawing.
     */
    void init();

    /**
     * Copy current matrix onto stack
     */
    void push_transforms();

    /**
     * Remove the current matrix from the stack and revert to prior
     * (or 0 if none are set at this node)
     */
    void pop_transforms();

    /**
     * Copy current material onto stack
     */
    void push_material();

    /**
     * Remove the current material from the stack and revert to prior
     * (or 0 if none are set at this node)
     */
    void pop_material();

    /**
     * Copy current texture onto stack
     **/
    void push_texture();

    /**
     * Remove the current texture from the stack and revert to prior
     * (or 0 if none are set at this node)
     **/
    void pop_texture();
};

} // namespace cg

#endif
