#include "asset_loaders.h"

#include "freya_logger.h"

/// ----------------------------------------------------------------------
/// LUA state loader functions

bool lua_state_loader_load(LuaLoader& out_loader, const freya::FilePath& path) {
  // Open the LUA file

  freya::File file;
  if(!freya::file_open(file, path, (freya::i32)freya::FILE_OPEN_READ)) {
    FREYA_LOG_ERROR("Failed to open LUA file at \'%s\'", path.c_str());
    return false;
  }
  
  // Read the string from the file
  
  freya::file_read_string(file, &out_loader.source);
  freya::file_close(file);

  // Done!
  
  FREYA_LOG_TRACE("Successfully loaded LUA file at '\%s\'", path.c_str());
  return true;
}

void lua_state_loader_save(const LuaLoader& loader, freya::File& file) {
  // @TODO (Loader/lua): Do more?
  freya::file_write_bytes(file, loader.source);
}

/// LUA state loader functions
/// ----------------------------------------------------------------------
