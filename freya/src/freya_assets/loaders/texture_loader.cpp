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
  sg_image_desc& img_desc = out_loader.desc;

  if(stbi_is_hdr(path.c_str())) {
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA32F;
    out_loader.data       = stbi_loadf(path.c_str(), &width, &height, NULL, 4);
  }
  else {
    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    out_loader.data       = stbi_load(path.c_str(), &width, &height, NULL, 4);
  }

  // Fuck!

  if(!out_loader.data) {
    FREYA_LOG_ERROR("Could not load texture at \'%s'\, %s", path.c_str(), stbi_failure_reason());
    stbi_image_free(out_loader.data);

    return false;
  }

  // Filling the rest of the desc

  img_desc.width  = width;
  img_desc.height = height;

  // Done!
 
  FREYA_LOG_TRACE("Successfully loaded texture at '\%s\'", path.c_str());
  return true;
}

void texture_loader_save(const TextureLoader& loader, freya::File& file) {
  // Set up the sampler

  sg_sampler_desc sampler_desc = {};
  sampler_desc.min_filter      = SG_FILTER_NEAREST;
  sampler_desc.mag_filter      = SG_FILTER_NEAREST;

  // Write the texture's size

  freya::u16 width  = (freya::u16)loader.desc.width;
  freya::u16 height = (freya::u16)loader.desc.height;

  freya::file_write_bytes(file, &width, sizeof(width));
  freya::file_write_bytes(file, &height, sizeof(height));

  // Write the format

  freya::u8 format = (freya::u8)loader.desc.pixel_format;
  freya::file_write_bytes(file, &format, sizeof(format));

  // Write the filters 

  freya::u8 min_filter = (freya::u8)sampler_desc.min_filter;
  freya::u8 mag_filter = (freya::u8)sampler_desc.mag_filter;

  freya::file_write_bytes(file, &min_filter, sizeof(min_filter));
  freya::file_write_bytes(file, &mag_filter, sizeof(mag_filter));

  // Write the data

  freya::u32 pixel_size = (format == SG_PIXELFORMAT_RGBA32F) ? 4 : 1; 
  freya::u32 data_size  = (width * height) * 4 * pixel_size; // 4 = color components

  freya::file_write_bytes(file, loader.data, data_size);

  // Free the data
  freya::memory_free(loader.data); 
}

/// Texture loader functions
/// ----------------------------------------------------------------------
