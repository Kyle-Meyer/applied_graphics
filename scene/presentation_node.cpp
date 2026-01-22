#include "scene/presentation_node.hpp"

#include "scene/image_data.hpp"

#include <iostream>

namespace cg
{

PresentationNode::PresentationNode()
{
    node_type_ = SceneNodeType::PRESENTATION;
    material_shininess_ = 1.0f;
    texture_id_ = 0; // Default to no texture
}

PresentationNode::PresentationNode(const Color4 &ambient,
                                   const Color4 &diffuse,
                                   const Color4 &specular,
                                   const Color4 &emission,
                                   float         shininess) :
    material_ambient_(ambient),
    material_diffuse_(diffuse),
    material_specular_(specular),
    material_emission_(emission),
    material_shininess_(shininess),
    texture_id_(0)
{
    node_type_ = SceneNodeType::PRESENTATION;
}

void PresentationNode::set_material_ambient(const Color4 &c) { material_ambient_ = c; }

void PresentationNode::set_material_diffuse(const Color4 &c) { material_diffuse_ = c; }

void PresentationNode::set_material_ambient_and_diffuse(const Color4 &c)
{
    material_ambient_ = c;
    material_diffuse_ = c;
}

void PresentationNode::set_material_specular(const Color4 &c) { material_specular_ = c; }

void PresentationNode::set_material_emission(const Color4 &c) { material_emission_ = c; }

void PresentationNode::set_material_shininess(float s) { material_shininess_ = s; }

void PresentationNode::set_texture(
    const std::string &fname, GLuint s_wrap, GLuint t_wrap, GLuint min_filter, GLuint mag_filter)
{
    ImageData im_data;
    load_image_data(im_data, fname);

    if(im_data.data == nullptr)
    {
        std::cout << "Error getting image data\n";
        return;
    }

    // Generate an OpenGL textureID, bind it
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    // Load image data and generate mipmaps
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 im_data.w,
                 im_data.h,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 im_data.data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Set wrapping mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, s_wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, t_wrap);

    // Set texture filters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);

    // Bind null texture
    glBindTexture(GL_TEXTURE_2D, 0);

    // Free the image data; it is no longer needed
    free_image_data(im_data);
}

void PresentationNode::update_texture_filters(GLuint min_filter, GLuint mag_filter)
{
    if(texture_id_)
    {
        glBindTexture(GL_TEXTURE_2D, texture_id_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    }
}

void PresentationNode::draw(SceneState &scene_state)
{
    // Set the material uniform values
    glUniform4fv(scene_state.material_ambient_loc, 1, &material_ambient_.r);
    glUniform4fv(scene_state.material_diffuse_loc, 1, &material_diffuse_.r);
    glUniform4fv(scene_state.material_specular_loc, 1, &material_specular_.r);
    glUniform4fv(scene_state.material_emission_loc, 1, &material_emission_.r);
    glUniform1f(scene_state.material_shininess_loc, material_shininess_);

    // Enable texture mapping and bind the texture
    if(texture_id_)
    {
        glUniform1i(scene_state.use_texture_loc, 1); // Tell shader we are using textures
        //   glUniform1i(scene_state.texture_unit_loc, 0);  // Texture unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_id_);
    }
    else
    {
        // Set a special value to tell the shader we are not using textures
        glUniform1i(scene_state.use_texture_loc, 0);
    }

    // Draw children of this node
    SceneNode::draw(scene_state);

    // Turn off texture mapping for any nodes not descended from this presentation node
    if(texture_id_)
    {
        // Disable texture mapping
        glUniform1i(scene_state.use_texture_loc, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

} // namespace cg
