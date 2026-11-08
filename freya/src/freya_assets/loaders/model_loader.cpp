#include "asset_loaders.h"

#include "freya_assets.h"
#include "freya_logger.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

/// ----------------------------------------------------------------------
/// NodeData
struct NodeData {
  freya::String name; 
  freya::DynamicArray<aiMesh*> meshes;
};
/// NodeData
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// ObjData
struct ObjData {
  freya::FilePath parent_dir;
 
  ModelLoader* loader;
  freya::DynamicArray<NodeData> nodes;
};
/// ObjData
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Private functions

static bool check_valid_extension(const freya::FilePath& ext) {
  return ext == ".gltf" ||
         ext == ".GLTF" || 
         ext == ".glb"  || 
         ext == ".GLB"  || 
         ext == ".fbx"  || 
         ext == ".FBX";
}

static void load_node_mesh(ObjData* data, aiMesh* mesh, MeshLoader& out_loader) {
  // Go through all of the vertices of the given `mesh` and 
  // convert them to the appropriate vertex type. 
  
  out_loader.vertices.reserve(mesh->mNumVertices);
  for(freya::sizei iv = 0; iv < mesh->mNumVertices; iv++) {
    // Adding the position
   
    freya::Vertex3D vertex;
    vertex.position.x = mesh->mVertices[iv].x;
    vertex.position.y = mesh->mVertices[iv].y;
    vertex.position.z = mesh->mVertices[iv].z;
    
    // Adding the normal 

    vertex.normal.x = mesh->mNormals[iv].x;
    vertex.normal.y = mesh->mNormals[iv].y;
    vertex.normal.z = mesh->mNormals[iv].z;

    // Adding tangents 

    vertex.tangent.x = mesh->mTangents[iv].x;
    vertex.tangent.y = mesh->mTangents[iv].y;
    vertex.tangent.z = mesh->mTangents[iv].z;

    // Adding texture coordinates
    
    vertex.texture_coords.x = mesh->mTextureCoords[0][iv].x; 
    vertex.texture_coords.y = mesh->mTextureCoords[0][iv].y; 

    // Add the vertex 
    out_loader.vertices.emplace_back(vertex);
  }
  
  // Add the material index of the mesh to refrence it later on. Much later on.
  out_loader.material_index = mesh->mMaterialIndex;

  // Load indices

  out_loader.indices.reserve(mesh->mNumFaces);
  for(freya::sizei i = 0; i < mesh->mNumFaces; i++) {
    aiFace* face = &mesh->mFaces[i];

    for(freya::sizei j = 0; j < face->mNumIndices; j++) {
      out_loader.indices.emplace_back((freya::u16)face->mIndices[j]);
    }
  } 
}

static void load_material_texture(aiMaterial* material, aiTextureType type, ObjData* data, freya::i8* index) {
  for(freya::u32 i = 0; i < material->GetTextureCount(type); i++) {
    aiString str;
    material->GetTexture(type, i, &str);

    // Convert into our `TextureLoader`
     
    TextureLoader texture;
    if(!texture_loader_load(texture, freya::filepath_append(data->parent_dir, str.C_Str()))) {
      continue;
    }

    // Add the new texture for later

    data->loader->textures.emplace_back(texture);
    *index += (freya::i8)data->loader->textures.size();
  }
}

static void load_nodes(const aiScene* scene, ObjData* data, aiNode* ai_node) {
  data->nodes.push_back(NodeData{.name = ai_node->mName.C_Str()});

  // Starting at the root node, we will recursively go down 
  // the scene tree and queue up each mesh to add it to the model later. 

  for(freya::sizei i = 0; i < ai_node->mNumMeshes; i++) {
    NodeData* node = &data->nodes[data->nodes.size() - 1];
    node->meshes.push_back(scene->mMeshes[ai_node->mMeshes[i]]);
  }

  // The given `ai_node` will also have children of its own. Those 
  // will need to be processed as well, obviously.
  
  for(freya::sizei i = 0; i < ai_node->mNumChildren; i++) {
    load_nodes(scene, data, ai_node->mChildren[i]);
  }
}

static void convert_meshes(ObjData* data) {
  for(auto& node : data->nodes) {
    for(auto& mesh : node.meshes) {
      // Convert an `aiMesh` into our `MeshLoader`

      MeshLoader mesh_loader;
      load_node_mesh(data, mesh, mesh_loader);

      // Add the new mesh for later
      data->loader->meshes.emplace_back(mesh_loader);
    }
  }
}

static void convert_materials(const aiScene* scene, ObjData* data) {
  data->loader->materials.reserve(scene->mNumMaterials);

  // Go through each material in the scene 
  for(freya::sizei i = 0; i < scene->mNumMaterials; i++) {
    aiMaterial* material = scene->mMaterials[i];
    MaterialLoader loader; 
     
    // Convert the colors
     
    aiColor3D diffuse;
    material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
    
    loader.color.r = diffuse.r;  
    loader.color.g = diffuse.g; 
    loader.color.b = diffuse.b;

    // Load individual values

    material->Get(AI_MATKEY_ROUGHNESS_FACTOR, loader.roughness);
    material->Get(AI_MATKEY_METALLIC_FACTOR, loader.metallic);

    // Load textures of each kind

    load_material_texture(material, aiTextureType_BASE_COLOR, data, &loader.albedo_index); 
    load_material_texture(material, aiTextureType_METALNESS, data, &loader.metallic_index); 
    load_material_texture(material, aiTextureType_DIFFUSE_ROUGHNESS, data, &loader.roughness_index); 
    load_material_texture(material, aiTextureType_NORMALS, data, &loader.normal_index); 

    // New material!
    data->loader->materials.emplace_back(loader); 
  }
}

/// Private functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Model loader functions

bool model_loader_load(ModelLoader& out_loader, const freya::FilePath& path) {
  // Try to get the model file inside the given path
  
  freya::FilePath full_path = "";
  bool found                = false;

  freya::filesystem_directory_iterate(path, [&](const freya::FilePath& base_dir, 
                                                const freya::FilePath& current_path, 
                                                void* user_data) {
      // Check the extension of the current file

      freya::FilePath ext = freya::filepath_extension(current_path);
      if(!check_valid_extension(ext)) {
        return true; // This is not a valid file... keep going
      }

      // Done! 
     
      full_path = current_path; 
      found     = true;

      return false; // Returning false to stop the iteration
  });

  // Sanity check

  if(!found) {
    FREYA_LOG_ERROR("Could not find a valid model file at \'%s\'!", full_path.c_str());
    return false;
  }
  
  // Load Assimp file
  
  freya::i32 flags = (aiProcess_Triangulate           | 
                      aiProcess_FlipUVs               | 
                      aiProcess_CalcTangentSpace      |
                      aiProcess_GenNormals            | 
                      aiProcess_SplitLargeMeshes      |
                      aiProcess_ImproveCacheLocality  | 
                      aiProcess_JoinIdenticalVertices |
                      aiProcess_OptimizeMeshes        | 
                      aiProcess_OptimizeGraph         |
                      // aiProcess_LimitBoneWeights      | 
                      // aiProcess_PopulateArmatureData  |
                      aiProcess_GlobalScale); 

  Assimp::Importer imp; 
  imp.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, freya::MODEL_IMPORT_SCALE);
  // imp.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, freya::JOINT_WEIGHTS_MAX);

  const aiScene* scene = imp.ReadFile(full_path, flags);
  if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    FREYA_LOG_ERROR("Could not load Model at \'%s\' - %s", full_path.c_str(), imp.GetErrorString());
    return false;
  }

  // Loading everything into `ObjData`
  
  ObjData data; 
  data.parent_dir = path;
  data.loader     = &out_loader;

  // Queue up all of the meshes that need to be converted
  load_nodes(scene, &data, scene->mRootNode);

  // Meshes init 
  convert_meshes(&data);

  // Materials init
  convert_materials(scene, &data);  

  // Done!
 
  FREYA_LOG_TRACE("Successfully loaded model at '\%s\'", full_path.c_str());
  return true;
}

void model_loader_save(const ModelLoader& loader, freya::File& file) {
  //
  // Write all of the meshes 
  // 

  freya::u16 meshes_count = (freya::u16)loader.meshes.size();
  freya::file_write_bytes(file, &meshes_count, sizeof(meshes_count));

  for(auto& mesh : loader.meshes) {
    mesh_loader_save(mesh, file);
  }

  //
  // Write the textures
  //

  freya::u8 textures_count = (freya::u32)loader.textures.size();
  freya::file_write_bytes(file, &textures_count, sizeof(textures_count));

  for(auto& texture : loader.textures) {
    texture_loader_save(texture, file);
  }

  //
  // Write the materials 
  //
  
  freya::u16 materials_count = (freya::u16)loader.materials.size();
  freya::file_write_bytes(file, &materials_count, sizeof(materials_count));
  
  for(auto& mat : loader.materials) {
    // Write the name 
    freya::file_write_bytes(file, mat.name);

    // Write the material variables 
     
    freya::file_write_bytes(file, &mat.roughness, sizeof(mat.roughness));
    freya::file_write_bytes(file, &mat.metallic, sizeof(mat.metallic));
    freya::file_write_bytes(file, &mat.color[0], sizeof(mat.color));

    // Write the indices 

    freya::file_write_bytes(file, &mat.albedo_index, sizeof(mat.albedo_index));
    freya::file_write_bytes(file, &mat.roughness_index, sizeof(mat.roughness_index));
    freya::file_write_bytes(file, &mat.metallic_index, sizeof(mat.metallic_index));
    freya::file_write_bytes(file, &mat.normal_index, sizeof(mat.normal_index));
  }
}

void model_loader_read(ModelLoader& loader, freya::File& file) {
  //
  // Read all of the meshes 
  // 

  freya::u16 meshes_count = 0;
  freya::file_read_bytes(file, &meshes_count, sizeof(meshes_count));

  loader.meshes.resize(meshes_count);
  for(freya::u16 i = 0; i < meshes_count; i++) {
    MeshLoader& mesh = loader.meshes[i];
    mesh_loader_read(mesh, file);
  }

  //
  // Read the textures
  //

  freya::u8 textures_count = 0;
  freya::file_read_bytes(file, &textures_count, sizeof(textures_count));

  loader.textures.resize(textures_count);
  for(freya::u16 i = 0; i < textures_count; i++) {
    TextureLoader& texture = loader.textures[i];
    texture_loader_read(texture, file);
  }

  //
  // Write the materials 
  //
  
  freya::u16 materials_count = 0;
  freya::file_read_bytes(file, &materials_count, sizeof(materials_count));
  
  loader.materials.resize(materials_count);
  for(freya::u16 i = 0; i < materials_count; i++) {
    MaterialLoader& mat = loader.materials[i];

    // Write the name 
    freya::file_read_bytes(file, &mat.name);

    // Write the material variables 
     
    freya::file_read_bytes(file, &mat.roughness, sizeof(mat.roughness));
    freya::file_read_bytes(file, &mat.metallic, sizeof(mat.metallic));
    freya::file_read_bytes(file, &mat.color[0], sizeof(mat.color));

    // Write the indices 

    freya::file_read_bytes(file, &mat.albedo_index, sizeof(mat.albedo_index));
    freya::file_read_bytes(file, &mat.roughness_index, sizeof(mat.roughness_index));
    freya::file_read_bytes(file, &mat.metallic_index, sizeof(mat.metallic_index));
    freya::file_read_bytes(file, &mat.normal_index, sizeof(mat.normal_index));
  }
}

/// Model loader functions
/// ----------------------------------------------------------------------
