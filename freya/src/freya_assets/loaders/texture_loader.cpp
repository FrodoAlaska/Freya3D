#include "asset_loaders.h"

#include "freya_assets.h"
#include "freya_logger.h"
#include "freya_memory.h"

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
/// Texture loader functions

bool texture_loader_load(TextureLoader& out_loader, const freya::FilePath& path) {
  // Sanity check
  // @NOTE: C++ sucks! You really don't have a function to make a whole string 
  // into lowercase???? Really, man??

  freya::FilePath ext = freya::filepath_extension(path);
  for(auto& ch : ext) {
    ch = std::tolower(ch);
  } 

  if(!check_valid_extension(ext)) {
    FREYA_LOG_WARN("Invalid texture file found at \'%s\'!", path.c_str());
    return false;
  }

  // Loading the texture 

  freya::i32 width, height; 
  if(stbi_is_hdr(path.c_str())) {
    out_loader.image_desc.pixel_format = SG_PIXELFORMAT_RGBA32F;
    out_loader.data                    = stbi_loadf(path.c_str(), &width, &height, NULL, 4);
  }
  else {
    out_loader.image_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    out_loader.data                    = stbi_load(path.c_str(), &width, &height, NULL, 4);
  }

  // Fuck!

  if(!out_loader.data) {
    FREYA_LOG_ERROR("Could not load texture at \'%s'\, %s", path.c_str(), stbi_failure_reason());
    stbi_image_free(out_loader.data);

    return false;
  }

  // Filling the rest of the desc

  out_loader.image_desc.width  = width;
  out_loader.image_desc.height = height;
  
  // Set up the sampler

  out_loader.sampler_desc.min_filter = SG_FILTER_NEAREST;
  out_loader.sampler_desc.mag_filter = SG_FILTER_NEAREST;

  // Done!
 
  FREYA_LOG_TRACE("Successfully loaded texture at '\%s\'", path.c_str());
  return true;
}

bool texture_loader_load(TextureLoader& out_loader, const freya::u8* data, freya::sizei data_size) {
  FREYA_DEBUG_ASSERT(data, "Invalid data given to texture_loader_load");
 
  // Load the image from memory

  freya::i32 width, height; 
  out_loader.data = stbi_load_from_memory(data, data_size, &width, &height, NULL, 4);

  // Fuck!

  if(!out_loader.data) {
    FREYA_LOG_ERROR("Could not load raw texture, %s", stbi_failure_reason());
    stbi_image_free(out_loader.data);

    return false;
  }

  // Filling the rest of the desc

  out_loader.image_desc.width  = width;
  out_loader.image_desc.height = height;
  
  // Set up the sampler

  out_loader.sampler_desc.min_filter = SG_FILTER_NEAREST;
  out_loader.sampler_desc.mag_filter = SG_FILTER_NEAREST;

  // Done!
 
  FREYA_LOG_TRACE("Successfully loaded raw texture");
  return true;
}

void texture_loader_save(const TextureLoader& loader, freya::File& file) {
  // Write the texture's size

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

  freya::u32 pixel_size = (format == SG_PIXELFORMAT_RGBA32F) ? 4 : 1; 
  freya::u32 data_size  = (width * height) * 4 * pixel_size; // 4 = color components

  freya::file_write_bytes(file, loader.data, data_size);

  // Free the data
  freya::memory_free(loader.data); 
}

void texture_loader_read(TextureLoader& loader, freya::File& file) {
  // Read the texture's size

  freya::u16 width, height;
  
  freya::file_read_bytes(file, &width, sizeof(width));  
  freya::file_read_bytes(file, &height, sizeof(height));  

  loader.image_desc.width  = width;
  loader.image_desc.height = height;

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

  freya::u32 pixel_size = (format == SG_PIXELFORMAT_RGBA32F) ? 4 : 1; 
  freya::u32 data_size  = (width * height) * 4 * pixel_size; // 4 = color components

  loader.data = freya::memory_allocate(data_size);
  freya::file_read_bytes(file, loader.data, data_size);

  loader.image_desc.data.mip_levels[0].ptr  = loader.data;
  loader.image_desc.data.mip_levels[0].size = data_size;
}

/// Texture loader functions
/// ----------------------------------------------------------------------
