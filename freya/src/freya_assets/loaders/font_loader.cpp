#include "asset_loaders.h"

#include "freya_assets.h"
#include "freya_logger.h"

/// ----------------------------------------------------------------------
/// Font loader functions

bool font_loader_load(FontLoader& out_loader, const freya::FilePath& path) {
  // Open the file

  freya::File file;
  freya::i32 file_flags = (freya::i32)(freya::FILE_OPEN_READ | freya::FILE_OPEN_BINARY);

  if(!freya::file_open(file, path, file_flags)) {
    FREYA_LOG_ERROR("Could not load font at \'%s\'", path.c_str());
    return false;
  } 

  // Load the actual data from the file

  freya::sizei data_size = freya::filesystem_get_size(path);
  out_loader.bytes.resize(data_size);

  freya::file_read_bytes(file, out_loader.bytes.data(), data_size); 

  // Done!
   
  FREYA_LOG_TRACE("Successfully loaded font at '\%s\'", path.c_str());
  return true;
}

void font_loader_save(const FontLoader& loader, freya::File& file) {
  freya::u32 data_size = (freya::u32)loader.bytes.size();

  freya::file_write_bytes(file, &data_size, sizeof(data_size));
  freya::file_write_bytes(file, loader.bytes.data(), loader.bytes.size());
}

void font_loader_read(FontLoader& loader, freya::File& file) {
  freya::u32 data_size;
  freya::file_read_bytes(file, &data_size, sizeof(data_size));

  loader.bytes.resize(data_size);
  freya::file_read_bytes(file, loader.bytes.data(), data_size);
}

/// Font loader functions
/// ----------------------------------------------------------------------
