#include "asset_loaders.h"

#include "freya_assets.h"
#include "freya_logger.h"
#include "freya_gfx.h"

#include <stb/stb_image.h>

/// ----------------------------------------------------------------------
/// Private functions

static bool check_valid_extension(const freya::FilePath& ext) {
  return ext == ".png"  ||  
         ext == ".jpg"  ||  
         ext == ".jpeg" ||  
         ext == ".bmp"  ||  
         ext == ".psd"  ||  
         ext == ".tga"  ||  
         ext == ".gif"  ||  
         ext == ".hdr"  ||  
         ext == ".pic"  ||  
         ext == ".ppm"  ||  
         ext == ".pgm";
}

/// Private functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Cubemap loader functions

bool cubemap_loader_load(const freya::FilePath& path, sg_image_desc& out_img, freya::u8** out_data) {
  // Sanity check

  //
  // Load all of the cubemap faces
  //

  freya::i32 width, height; 
  freya::i32 channels = 4;

  freya::u32 count = 0;
  freya::Array<stbi_uc*, freya::CUBEMAP_FACES_MAX> faces;
  
  bool is_valid = true;

  freya::filesystem_directory_iterate(path, [&](const freya::FilePath& base_dir, 
                                                const freya::FilePath& current_path, 
                                                void* user_data) {
    if(!check_valid_extension(freya::filepath_extension(current_path))) {
      FREYA_LOG_WARN("Invalid cubemap file found at \'%s\'!", current_path.c_str());
      return true;
    }

    faces[count] = stbi_load(current_path.c_str(), &width, &height, NULL, channels);
    if(!faces[count]) {
      FREYA_LOG_ERROR("Failed to load cubemap face at \'%s\'", current_path.c_str());

      is_valid = false;
      return false;
    }

    count++;
    return true;
  });

  if(!is_valid) {
    return false;
  }

  // 
  // Load each face into the out data
  //

  freya::u32 face_bytes = (width * height * channels);
  freya::u32 cube_bytes = count * face_bytes;

  *out_data = (freya::u8*)freya::memory_allocate(cube_bytes);
  for(freya::sizei i = 0; i < faces.size(); i++) {
    freya::u8* current_face = ((*out_data) + (i * face_bytes));
    freya::memory_copy(current_face, faces[i], face_bytes);
  }
  
  //
  // Filling the rest of the desc
  //

  out_img.type         = SG_IMAGETYPE_CUBE;
  out_img.pixel_format = SG_PIXELFORMAT_RGBA8;
  out_img.width        = width;
  out_img.height       = height;

  // Done!
 
  FREYA_LOG_TRACE("Successfully loaded cubemap at '\%s\'", path.c_str());
  return true;
}

/// Cubemap loader functions
/// ----------------------------------------------------------------------
