#include "asset_loaders.h"

#include "freya_assets.h"
#include "freya_logger.h"

#include <cgltf/cgltf.h>

/// ----------------------------------------------------------------------
/// Private functions

static bool check_valid_extension(const freya::FilePath& ext) {
  return ext == ".gltf" ||
         ext == ".GLTF" || 
         ext == ".glb"  || 
         ext == ".GLB";
}

static sg_filter gltf_filter_to_filter(const cgltf_filter_type filter) {
  switch(filter) {
    case cgltf_filter_type_undefined:
      return (sg_filter)-1;
    case cgltf_filter_type_nearest:
      return SG_FILTER_NEAREST;
    case cgltf_filter_type_linear:
      return SG_FILTER_LINEAR;
    case cgltf_filter_type_nearest_mipmap_nearest:
      return SG_FILTER_NEAREST;
    case cgltf_filter_type_linear_mipmap_nearest:
      return SG_FILTER_LINEAR;
    case cgltf_filter_type_nearest_mipmap_linear:
      return SG_FILTER_NEAREST;
    case cgltf_filter_type_linear_mipmap_linear:
      return SG_FILTER_LINEAR;
  }
}

/// Private functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Model loader functions

bool model_loader_load(ModelLoader& out_loader, const freya::FilePath& path) {
  // Sanity check

  freya::FilePath ext = freya::filepath_extension(path);
  if(!check_valid_extension(ext)) {
    FREYA_LOG_WARN("Invalid model file found at \'%s\'!", path.c_str());
    return false;
  }

  //
  // Load the GLTF file
  //

  cgltf_data* gltf      = new cgltf_data{};
  cgltf_options options = {}; 

  cgltf_result res = cgltf_parse_file(&options, path.c_str(), &gltf);
  if(res != cgltf_result_success) {
    cgltf_free(gltf);

    FREYA_LOG_ERROR("Failed to open GLTF file at \'%s\'", path.c_str());
    return false;
  }
  
  //
  // Load the buffers
  //

  res = cgltf_load_buffers(&options, gltf, path.c_str());
  if(res != cgltf_result_success) {
    cgltf_free(gltf);

    FREYA_LOG_ERROR("Failed to load GLTF buffers at \'%s\'", path.c_str());
    return false;
  }

  // 
  // Load the meshes
  //

  for(freya::sizei i = 0; i < gltf->meshes_count; i++) {
    // @TODO   
  }

  // 
  // Load textures 
  //

  for(freya::sizei i = 0; i < gltf->textures_count; i++) {
    cgltf_texture* texture = &gltf->textures[i];
    cgltf_image* image     = texture->image;
    cgltf_sampler* sampler = texture->sampler;

    //
    // Load the texture
    //
     
    TextureLoader loader; 

    // Load the image (either an internal or an external image)

    if(image->uri) {
      texture_loader_load(loader, freya::filepath_append(freya::filepath_parent_path(path), freya::FilePath(image->uri)));
    }
    else {
      texture_loader_load(loader, cgltf_buffer_view_data(image->buffer_view), image->buffer_view->size);
    }

    // Load the sampler
     
    loader.sampler_desc.min_filter = gltf_filter_to_filter(sampler->min_filter);
    loader.sampler_desc.mag_filter = gltf_filter_to_filter(sampler->mag_filter);
  }

  // 
  // Load materials
  //
  
  for(freya::sizei i = 0; i < gltf->materials_count; i++) {
    cgltf_material* mat = &gltf->materials[i]; 

    // Load the material
     
    MaterialLoader mat_loader;
    mat_loader.name = freya::String(mat->name);

    mat_loader.roughness = mat->pbr_metallic_roughness.metallic_factor;
    mat_loader.metallic  = mat->pbr_metallic_roughness.roughness_factor;
    
    freya::f32* color = &mat->pbr_metallic_roughness.base_color_factor[0];
    mat_loader.color  = freya::Vec3(color[0], color[1], color[2]);

    mat_loader.albedo_index    = (freya::u8)cgltf_texture_index(gltf, mat->pbr_metallic_roughness.base_color_texture.texture);
    mat_loader.roughness_index = (freya::u8)cgltf_texture_index(gltf, mat->pbr_metallic_roughness.metallic_roughness_texture.texture);
    mat_loader.metallic_index  = -1; // @TODO (CGLTF): GLTF combines both the metallic and roughness together... shit!
    mat_loader.normal_index    = (freya::u8)cgltf_texture_index(gltf, mat->normal_texture.texture);

    out_loader.materials.emplace_back(mat_loader);
  }

  // Done!
 
  cgltf_free(gltf);
  
  FREYA_LOG_TRACE("Successfully loaded model at '\%s\'", path.c_str());
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
