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

bool cubemap_loader_load(CubemapLoader& out_loader, const freya::FilePath& path) {
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
    // Sanity check
     
    if(!check_valid_extension(freya::filepath_extension(current_path))) {
      FREYA_LOG_WARN("Invalid cubemap file found at \'%s\'!", current_path.c_str());
      return true;
    }

    faces[count] = stbi_load(current_path.c_str(), &width, &height, NULL, channels);
    if(!faces[count]) {
      FREYA_LOG_ERROR("Failed to load cubemap face at \'%s\'", current_path.c_str());
      stbi_image_free(faces[count]);

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

  out_loader.data = (freya::u8*)freya::memory_allocate(cube_bytes);
  for(freya::sizei i = 0; i < faces.size(); i++) {
    freya::u8* current_face = (out_loader.data + (i * face_bytes));
    freya::memory_copy(current_face, faces[i], face_bytes);
  }
  
  //
  // Filling the rest of the desc
  //

  out_loader.image_desc.type         = SG_IMAGETYPE_CUBE;
  out_loader.image_desc.pixel_format = SG_PIXELFORMAT_SRGB8A8;
  out_loader.image_desc.width        = width;
  out_loader.image_desc.height       = height;
  
  out_loader.sampler_desc.min_filter      = SG_FILTER_LINEAR;
  out_loader.sampler_desc.mag_filter      = SG_FILTER_LINEAR;

  // Done!
 
  FREYA_LOG_TRACE("Successfully loaded cubemap at '\%s\'", path.c_str());
  return true;
}

void cubemap_loader_save(const CubemapLoader& loader, freya::File& file) {
  // Write the cubemap's size (size per-face)

  freya::u16 width  = (freya::u16)loader.image_desc.width;
  freya::u16 height = (freya::u16)loader.image_desc.height;

  freya::file_write_bytes(file, &width, sizeof(width));
  freya::file_write_bytes(file, &height, sizeof(height));

  // Write the format

  freya::u8 format = (freya::u8)loader.image_desc.pixel_format;
  freya::file_write_bytes(file, &format, sizeof(format));

  // Write the filters 

  freya::u8 min_filter = (freya::u8)loader.sampler_desc.min_filter;
  freya::u8 mag_filter = (freya::u8)loader.sampler_desc.mag_filter;

  freya::file_write_bytes(file, &min_filter, sizeof(min_filter));
  freya::file_write_bytes(file, &mag_filter, sizeof(mag_filter));

  // Write the data

  freya::u32 data_size = freya::CUBEMAP_FACES_MAX * ((width * height) * 4); // 4 = color components
  freya::file_write_bytes(file, loader.data, data_size);

  // Free the data 
  freya::memory_free(loader.data);
}

void cubemap_loader_read(CubemapLoader& loader, freya::File& file) {
  // Read the cubemap's size (size per-face)

  freya::u16 width, height;
  freya::file_read_bytes(file, &width, sizeof(width));  
  freya::file_read_bytes(file, &height, sizeof(height));  

  loader.image_desc.width  = width;
  loader.image_desc.height = height;

  loader.image_desc.type = SG_IMAGETYPE_CUBE;

  // Read the format

  freya::u8 format;
  freya::file_read_bytes(file, &format, sizeof(format));

  loader.image_desc.pixel_format = (sg_pixel_format)format;

  // Read the filters

  freya::u8 min_filter, mag_filter;

  freya::file_read_bytes(file, &min_filter, sizeof(min_filter));
  freya::file_read_bytes(file, &mag_filter, sizeof(mag_filter));

  loader.sampler_desc.min_filter = (sg_filter)min_filter;
  loader.sampler_desc.mag_filter = (sg_filter)mag_filter;

  // Read the data

  freya::u32 data_size = freya::CUBEMAP_FACES_MAX * ((width * height) * 4); // 4 = color components

  loader.data = (freya::u8*)freya::memory_allocate(data_size);
  freya::file_read_bytes(file, loader.data, data_size);

  loader.image_desc.data.mip_levels[0].ptr  = loader.data;
  loader.image_desc.data.mip_levels[0].size = data_size;
}

/// Cubemap loader functions
/// ----------------------------------------------------------------------
