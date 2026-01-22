#include "scene/model_node.hpp"

#include "filesystem_support/file_locator.hpp"
#include "scene/image_data.hpp"

#include <fstream>
#include <iostream>

namespace cg
{

// Note - this does not handle node hierarchy and transformations
// It does handle multiple meshes and textures.

ModelNode::ModelNode(int32_t            position_loc,
                     int32_t            normal_loc,
                     int32_t            texture_loc,
                     const std::string &filename)
{
    import_model_from_file(filename);
    gen_vaos_and_uniform_buffer(ai_scene_, position_loc, normal_loc, texture_loc);
}

ModelNode::~ModelNode()
{
    for(uint32_t n = 0; n < meshes_.size(); ++n)
    {
        // Delete vertex buffer objects, VAO, and texture objects
        if(meshes_[n].position_vbo > 0) glDeleteBuffers(1, &meshes_[n].position_vbo);
        if(meshes_[n].normal_vbo > 0) glDeleteBuffers(1, &meshes_[n].normal_vbo);
        if(meshes_[n].texture_vbo > 0) glDeleteBuffers(1, &meshes_[n].texture_vbo);
        glDeleteVertexArrays(1, &meshes_[n].vao);
        if(meshes_[n].has_texture) { glDeleteTextures(1, &meshes_[n].texture_id); }
    }
}

void ModelNode::draw(SceneState &scene_state)
{
    // Draw all meshes assigned to this node
    for(uint32_t n = 0; n < meshes_.size(); ++n)
    {
        if(meshes_[n].has_texture)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, meshes_[n].texture_id);
            glUniform1i(scene_state.use_texture_loc, 1);  // Tell shader we are using textures
            glUniform1i(scene_state.texture_unit_loc, 0); // Texture unit 0
        }
        else { glUniform1i(scene_state.use_texture_loc, 0); }
        glBindVertexArray(meshes_[n].vao);
        glDrawElements(GL_TRIANGLES, meshes_[n].num_faces * 3, GL_UNSIGNED_INT, 0);
    }
}

void ModelNode::import_model_from_file(const std::string &filename)
{
    auto file_info = locate_path_for_filename(filename, 5);

    if(!file_info.found)
    {
        // If file not found, try a directory called textures, going up a few directories
        std::string tex_fname = std::string("model/") + filename;
        file_info = locate_path_for_filename(tex_fname, 5);
    }

    if(!file_info.found)
    {
        std::cout << "Error getting finding file " << filename << '\n';
        system("pause");
        exit(1);
    }

    model_filename_ = file_info.file_path;
    model_directory_ = get_file_path(file_info.file_path);

    ai_scene_ = ai_importer_.ReadFile(model_filename_, aiProcessPreset_TargetRealtime_Quality);

    // If the import failed, report it
    if(!ai_scene_)
    {
        std::cout << ai_importer_.GetErrorString() << '\n';
        system("pause");
        exit(1);
    }

    // Now we can access the file's contents. Everything will be cleaned up
    // by the importer destructor
}

void ModelNode::gen_vaos_and_uniform_buffer(const aiScene *sc,
                                            int32_t        vertexLoc,
                                            int32_t        normal_loc,
                                            int32_t        texture_loc)
{
    ModelMesh model_mesh;
    GLuint    buffer;

    // For each mesh
    for(uint32_t n = 0; n < sc->mNumMeshes; ++n)
    {
        const aiMesh *mesh = sc->mMeshes[n];

        // create array with faces. Convert from Assimp format to array
        uint32_t *face_array = new uint32_t[sizeof(uint32_t) * mesh->mNumFaces * 3];
        uint32_t  face_index = 0;
        for(uint32_t t = 0; t < mesh->mNumFaces; ++t)
        {
            const aiFace *face = &mesh->mFaces[t];
            memcpy(&face_array[face_index], face->mIndices, 3 * sizeof(uint32_t));
            face_index += 3;
        }
        model_mesh.num_faces = sc->mMeshes[n]->mNumFaces;

        // Generate Vertex Array Object for mesh
        glGenVertexArrays(1, &(model_mesh.vao));
        glBindVertexArray(model_mesh.vao);

        // Buffer for faces
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     sizeof(uint32_t) * mesh->mNumFaces * 3,
                     face_array,
                     GL_STATIC_DRAW);

        // buffer for vertex positions
        if(mesh->HasPositions())
        {
            glGenBuffers(1, &buffer);
            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            glBufferData(GL_ARRAY_BUFFER,
                         sizeof(float) * 3 * mesh->mNumVertices,
                         mesh->mVertices,
                         GL_STATIC_DRAW);
            glEnableVertexAttribArray(vertexLoc);
            glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, 0, 0, 0);
        }

        // buffer for vertex normals
        if(mesh->HasNormals())
        {
            glGenBuffers(1, &buffer);
            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            glBufferData(GL_ARRAY_BUFFER,
                         sizeof(float) * 3 * mesh->mNumVertices,
                         mesh->mNormals,
                         GL_STATIC_DRAW);
            glEnableVertexAttribArray(normal_loc);
            glVertexAttribPointer(normal_loc, 3, GL_FLOAT, 0, 0, 0);
        }

        // buffer for vertex texture coordinates
        if(mesh->HasTextureCoords(0))
        {
            size_t tex_coord_size = sizeof(float) * 2 * mesh->mNumVertices;
            float *tex_coords = new float[tex_coord_size];

            for(uint32_t k = 0; k < mesh->mNumVertices; ++k)
            {
                tex_coords[k * 2] = mesh->mTextureCoords[0][k].x;
                tex_coords[k * 2 + 1] = mesh->mTextureCoords[0][k].y;
            }

            glGenBuffers(1, &buffer);
            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            glBufferData(GL_ARRAY_BUFFER, tex_coord_size, tex_coords, GL_STATIC_DRAW);
            glEnableVertexAttribArray(texture_loc);
            glVertexAttribPointer(texture_loc, 2, GL_FLOAT, 0, 0, 0);

            delete[] tex_coords;
        }

        // unbind buffers
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Create material uniform buffer
        aiMaterial *mtl = sc->mMaterials[mesh->mMaterialIndex];
        aiString    texPath; // contains filename of texture
        if(AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, 0, &texPath))
        {
            std::string tex_filename(texPath.data);
            if(!file_exists(tex_filename))
            {
                tex_filename = model_directory_;
                tex_filename += "/";
                tex_filename += texPath.data;
            }

            // Load the image file
            ImageData im_data;
            load_image_data(im_data, tex_filename);

            if(im_data.data == nullptr)
            {
                printf("Error getting image data\n");
                system("pause");
                exit(1);
            }

            // Open GL stuff to generate texture ID, bind texture, load image data, generate
            // mipmaps, set wrapping and filter modes...
            glGenTextures(1, &model_mesh.texture_id);
            glBindTexture(GL_TEXTURE_2D, model_mesh.texture_id);
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

            // Set wrap, repeat mode
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            model_mesh.has_texture = true;

            free_image_data(im_data);
        }
        else { model_mesh.has_texture = false; }
        meshes_.push_back(model_mesh);

        delete[] face_array;
    }
}

std::string ModelNode::get_file_path(const std::string &str)
{
    size_t found = str.find_last_of("/\\");
    return str.substr(0, found);
}

bool ModelNode::file_exists(const std::string &name)
{
    std::ifstream f(name.c_str());
    bool          exists = f.good();
    f.close();
    return exists;
}

} // namespace cg
