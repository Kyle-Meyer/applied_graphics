//============================================================================
//	Johns Hopkins University Engineering Programs for Professionals
//	605.667 Computer Graphics and 605.767 Applied Computer Graphics
//	Instructor:	Brian Russin
//
//	Author:  David W. Nesbitt
//	File:    modelnode.hpp
//	Purpose: Scene graph geometry node that loads a model file using ASSIMP.
//
//============================================================================

#ifndef __MODEL_NODE_HPP__
#define __MODEL_NODE_HPP__

#include "scene/scene_node.hpp"

// Assimp include files. These three are usually needed.
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include <string>

namespace cg
{

// Note - this does not handle node hierarchy and transformations
// It does handle multiple meshes and textures.

// Information to render each assimp node
struct ModelMesh
{
    bool    has_texture;
    GLuint  texture_id;
    int32_t num_faces;
    GLuint  vao;
    GLuint  position_vbo;
    GLuint  normal_vbo;
    GLuint  texture_vbo;
};

/**
 * Node that loads a model using Assimp.
 */
class ModelNode : public SceneNode
{
  public:
    /**
     * Constructor.  Loads the given model.  (Implements a modified version
     * of: http://www.opengl-tutorial.org/beginners-tutorials/tutorial-7-model-loading/)
     * Modified code from Brian Foster - student in 2014. Updates to fix destructor
     * and use texture mapping along with Phong shading shaders.
     * @param filename : Model file path/name
     */
    ModelNode(int32_t            position_loc,
              int32_t            normal_loc,
              int32_t            texture_loc,
              const std::string &filename);

    ~ModelNode();

    /**
     * Draw this model node.
     * @param  scene_state   Current scene state
     */
    void draw(SceneState &scene_state) override;

  protected:
    std::vector<ModelMesh> meshes_;
    const aiScene         *ai_scene_;
    Assimp::Importer       ai_importer_;
    std::string            model_filename_;
    std::string            model_directory_;

    /**
     * Import the model into a Assimp scene
     */
    void import_model_from_file(const std::string &filename);

    /**
     * Load the model from a Assimp scene into VBOs.
     */
    void gen_vaos_and_uniform_buffer(const aiScene *sc,
                                     int32_t        vertexLoc,
                                     int32_t        normal_loc,
                                     int32_t        texture_loc);

    std::string get_file_path(const std::string &str);

    bool file_exists(const std::string &name);
};

} // namespace cg

#endif
