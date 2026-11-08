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

  // Load the GLTF file

  cgltf_data* gltf      = new cgltf_data{};
  cgltf_options options = {}; 

  cgltf_result res = cgltf_parse_file(&options, path.c_str(), &gltf);
  if(res != cgltf_result_success) {
    cgltf_free(gltf);

    FREYA_LOG_ERROR("Failed to open GLTF file at \'%s\'", path.c_str());
    return false;
  }

  // Load the buffers

  res = cgltf_load_buffers(&options, gltf, path.c_str());
  if(res != cgltf_result_success) {
    cgltf_free(gltf);

    FREYA_LOG_ERROR("Failed to load GLTF buffers at \'%s\'", path.c_str());
    return false;
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
}

/// Model loader functions
/// ----------------------------------------------------------------------
