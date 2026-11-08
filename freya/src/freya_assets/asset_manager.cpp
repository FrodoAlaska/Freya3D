#include "freya_assets.h"
#include "freya_event.h"
#include "freya_logger.h"
#include "freya_memory.h"
#include "freya_render.h"

#include "asset_list/list.h"
#include "loaders/asset_loaders.h"

#include "fontstash/fontstash.h"

#include <cstring>

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

/// ----------------------------------------------------------------------
/// AssetManager 
struct AssetManager {
  AssetGroupID cache_id;
  HashMap<i32, AssetGroup> groups;
};

static AssetManager s_manager;
/// AssetManager 
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Macros (Unfortunately)

#define PUSH_ASSET(group, assets, asset, type, asset_id) {            \
  group.assets.push_back(asset);                                      \
  asset_id = AssetID(type, group.id, (i16)(group.assets.size() - 1)); \
}

#define GROUP_CHECK(group_id) FREYA_ASSERT(group_id != ASSET_GROUP_INVALID)

/// Macros (Unfortunately)
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Private functions 

template<typename T> 
static T& get_asset(const AssetID& id, DynamicArray<T>& asset, const AssetType type) {
  FREYA_DEBUG_ASSERT((id == type), "Invalid type when trying to retrieve a resource");
  FREYA_DEBUG_ASSERT((id >= 0 && id <= (i16)asset.size()), "Invalid ID when trying to retrieve a resource");

  return asset[id.get_id()];
}

static bool can_build_frpkg(const FilePath& assets_path, const FilePath& output_path) {
  // Check if output exists... 
  // we need to build a package if it doesn't exist

  if(!filesystem_exists(output_path)) {
    return true; 
  }

  // We also need to build (or update) the package if it 
  // was modified recently (or more recently than the package file)

  bool has_changed          = false; 
  FileTimePoint output_time = filesystem_get_last_write_time(output_path);

  // Iterate through each directory and check its write time

  auto iterate_dir = [&](const FilePath& base_dir, const FilePath& current_path, void* user_data) {
    FileTimePoint current_time = filesystem_get_last_write_time(current_path);
    if(current_time > output_time) {
      has_changed = true;
      return false;
    }

    return true;
  };
  filesystem_directory_iterate(assets_path, iterate_dir);

  // No changes. No need to build

  if(has_changed) {
    return true;
  }

  // No need to build the package...
  return false;
}

static void build_textures(File& file, const ListSection& section) {
  FREYA_PROFILE_FUNCTION();

  // Write the number of assets of this type

  u16 asset_count = (u16)section.assets.size(); 
  file_write_bytes(file, &asset_count, sizeof(asset_count));

  // Load and write all of the assets

  for(const auto& path : section.assets) {
    // Write the name of the asset

    FilePath name = filepath_stem(path);
    file_write_bytes(file, name);
     
    // Load the asset

    TextureLoader texture;
    texture_loader_load(texture, path);
     
    // Write the asset 
    texture_loader_save(texture, file);
  }
}

static void build_cubemaps(File& file, const ListSection& section) {
  FREYA_PROFILE_FUNCTION();

  // Write the number of assets of this type

  u16 asset_count = (u16)section.assets.size(); 
  file_write_bytes(file, &asset_count, sizeof(asset_count));

  // Load and write all of the assets

  for(const auto& path : section.assets) {
    // Write the name of the asset

    FilePath name = filepath_stem(path);
    file_write_bytes(file, name);

    // Load the asset

    CubemapLoader cubemap;
    cubemap_loader_load(cubemap, path);
    
    // Write the asset 
    cubemap_loader_save(cubemap, file); 
  }
}

static void build_models(File& file, const ListSection& section) {
  FREYA_PROFILE_FUNCTION();

  // Write the number of assets of this type

  u16 asset_count = (u16)section.assets.size(); 
  file_write_bytes(file, &asset_count, sizeof(asset_count));

  // Load and write all of the assets

  for(const auto& path : section.assets) {
    // Write the name of the asset

    FilePath name = filepath_stem(path);
    file_write_bytes(file, name);

    // Load the model from the loader 
    
    ModelLoader model;
    model_loader_load(model, path);

    // Write the asset 
    model_loader_save(model, file);
  }
}

static void build_fonts(File& file, const ListSection& section) {
  FREYA_PROFILE_FUNCTION();

  // Write the number of assets of this type

  u16 asset_count = (u16)section.assets.size(); 
  file_write_bytes(file, &asset_count, sizeof(asset_count));
  
  // Load and write all of the assets
  
  for(const auto& path : section.assets) {
    // Write the name of the asset

    FilePath name = filepath_stem(path);
    file_write_bytes(file, name);

    // Load the asset

    FontLoader font;
    font_loader_load(font, path);

    // Write the asset
    font_loader_save(font, file);
  }
}

static void build_audio_buffers(File& file, const ListSection& section) {
  FREYA_PROFILE_FUNCTION();

  // Write the number of assets of this type

  u16 asset_count = (u16)section.assets.size(); 
  file_write_bytes(file, &asset_count, sizeof(asset_count));
  
  // Load and write all of the assets
  
  for(const auto& path : section.assets) {
    // Write the name of the asset

    FilePath name = filepath_stem(path);
    file_write_bytes(file, name);

    // Load the asset

    AudioBufferDesc audio_desc = {};
    audio_loader_load(audio_desc, path);

    // Write the asset
    audio_loader_save(audio_desc, file);
  }
}

static void build_lua_state(File& file, const ListSection& section) {
  FREYA_PROFILE_FUNCTION();

  // Write the number of assets of this type

  u16 asset_count = (u16)section.assets.size(); 
  file_write_bytes(file, &asset_count, sizeof(asset_count));
  
  // Load and write all of the assets
  
  for(const auto& path : section.assets) {
    // Write the name of the asset

    FilePath name = filepath_stem(path);
    file_write_bytes(file, name);

    // Load the asset
   
    LuaLoader lua;
    lua_state_loader_load(lua, path);
    
    // Save the asset
    lua_state_loader_save(lua, file);
  }
}

static void read_textures(File& file, AssetGroup& group) {
  FREYA_PROFILE_FUNCTION();
  
  // Read the count

  u16 count;
  file_read_bytes(file, &count, sizeof(count));

  // Read the asset

  for(u16 i = 0; i < count; i++) {
    // Read the name

    String name;
    file_read_bytes(file, &name);

    //
    // Read the texture desc
    //

    sg_image_desc image_desc     = {};
    sg_sampler_desc sampler_desc = {};

    // Read the texture's size

    u16 width, height;
    file_read_bytes(file, &width, sizeof(width));  
    file_read_bytes(file, &height, sizeof(height));  

    image_desc.width  = width;
    image_desc.height = height;

    // Read the format

    u8 format;
    file_read_bytes(file, &format, sizeof(format));

    image_desc.pixel_format = (sg_pixel_format)format;

    // Read the filters

    u8 min_filter, mag_filter;

    file_read_bytes(file, &min_filter, sizeof(min_filter));
    file_read_bytes(file, &mag_filter, sizeof(mag_filter));

    sampler_desc.min_filter = (sg_filter)min_filter;
    sampler_desc.mag_filter = (sg_filter)mag_filter;

    // Read the data

    u32 pixel_size = (format == SG_PIXELFORMAT_RGBA32F) ? 4 : 1; 
    u32 data_size  = (width * height) * 4 * pixel_size; // 4 = color components

    void* pixels = memory_allocate(data_size);
    file_read_bytes(file, pixels, data_size);
    
    image_desc.data.mip_levels[0].ptr  = pixels;
    image_desc.data.mip_levels[0].size = data_size;

    // Add the texture to the group
    group.named_ids[name] = asset_group_push_texture(group.id, image_desc, sampler_desc); 
    
    // Done!
    
    memory_free(pixels);
    FREYA_LOG_DEBUG("Loaded texture \'%s\' from frpkg ", name.c_str());
  }
}

static void read_cubemaps(File& file, AssetGroup& group) {
  FREYA_PROFILE_FUNCTION();
  
  // Read the count

  u16 count;
  file_read_bytes(file, &count, sizeof(count));

  // Read the asset

  for(u16 i = 0; i < count; i++) {
    // Read the name

    String name;
    file_read_bytes(file, &name);

    //
    // Read the cubemap desc
    //

    sg_image_desc image_desc     = {};
    sg_sampler_desc sampler_desc = {};

    // Read the cubemap's size (size per-face)

    u16 width, height;
    file_read_bytes(file, &width, sizeof(width));  
    file_read_bytes(file, &height, sizeof(height));  

    image_desc.width  = width;
    image_desc.height = height;

    image_desc.type = SG_IMAGETYPE_CUBE;

    // Read the format

    u8 format;
    file_read_bytes(file, &format, sizeof(format));

    image_desc.pixel_format = (sg_pixel_format)format;

    // Read the filters

    u8 min_filter, mag_filter;

    file_read_bytes(file, &min_filter, sizeof(min_filter));
    file_read_bytes(file, &mag_filter, sizeof(mag_filter));

    sampler_desc.min_filter = (sg_filter)min_filter;
    sampler_desc.mag_filter = (sg_filter)mag_filter;

    // Read the data
    
    u32 data_size = CUBEMAP_FACES_MAX * ((width * height) * 4); // 4 = color components

    void* pixels = memory_allocate(data_size);
    file_read_bytes(file, pixels, data_size);
    
    image_desc.data.mip_levels[0].ptr  = pixels;
    image_desc.data.mip_levels[0].size = data_size;

    // Add the cubemap to the group
    group.named_ids[name] = asset_group_push_cubemap(group.id, image_desc, sampler_desc); 
    
    // Done!
    
    memory_free(pixels);
    FREYA_LOG_DEBUG("Loaded cubemap \'%s\' from frpkg ", name.c_str());
  }
}

static void read_models(File& file, AssetGroup& group) {
  FREYA_PROFILE_FUNCTION();
  
  // Read the count

  u16 count;
  file_read_bytes(file, &count, sizeof(count));

  // Read the asset

  for(u16 i = 0; i < count; i++) {
    // Read the name

    String name;
    file_read_bytes(file, &name);

    // Making new buffers for the meshes and the materials

    DynamicArray<AssetID> meshes; 
    DynamicArray<AssetID> materials; 
    // @TODO (Asset/models)

    // Add the cubemap to the group
    group.named_ids[name] = asset_group_push_model(group.id, meshes, materials); 
    
    // Done!
    FREYA_LOG_DEBUG("Loaded cubemap \'%s\' from frpkg ", name.c_str());
  }
}

static void read_fonts(File& file, AssetGroup& group) {
  FREYA_PROFILE_FUNCTION();
  
  // Read the count

  u16 count;
  file_read_bytes(file, &count, sizeof(count));

  // Read the asset

  for(u16 i = 0; i < count; i++) {
    // Read the name

    String name;
    file_read_bytes(file, &name);

    // Read the font data
   
    u32 data_size;
    file_read_bytes(file, &data_size, sizeof(data_size));

    DynamicArray<u8> font_data;
    font_data.resize(data_size);
    
    file_read_bytes(file, font_data.data(), data_size);

    // Add the font to the group
    group.named_ids[name] = asset_group_push_font(group.id, font_data, name); 

    FREYA_LOG_DEBUG("Loaded font \'%s\' from frpkg ", name.c_str());
  }
}

static void read_audio_buffers(File& file, AssetGroup& group) {
  FREYA_PROFILE_FUNCTION();
  
  // Read the count

  u16 count;
  file_read_bytes(file, &count, sizeof(count));

  // Read the asset

  for(u16 i = 0; i < count; i++) {
    // Read the name

    String name;
    file_read_bytes(file, &name);
    
    //
    // Read the audio desc
    //

    AudioBufferDesc desc;
  
    // Read the format
 
    u8 format;
    file_read_bytes(file, &format, sizeof(format));

    desc.format = (AudioBufferFormat)format;

    // Read the channels

    u8 channels;
    file_read_bytes(file, &channels, sizeof(channels));

    desc.channels = (u32)channels;

    // Read the sample rate
    file_read_bytes(file, &desc.sample_rate, sizeof(desc.sample_rate));

    // Read the samples

    u32 size;
    file_read_bytes(file, &size, sizeof(size));

    desc.size = (sizei)size;
    desc.data = memory_allocate(desc.size);

    file_read_bytes(file, desc.data, desc.size);

    // Add the audio buffer to the group
    group.named_ids[name] = asset_group_push_audio_buffer(group.id, desc); 

    // Get rid of the audio data on the CPU-side
    memory_free(desc.data); 

    // Done!
    FREYA_LOG_DEBUG("Loaded audio buffer \'%s\' from frpkg ", name.c_str());
  }
}

static void read_lua_state(File& file, AssetGroup& group) {
  FREYA_PROFILE_FUNCTION();
  
  // Read the count

  u16 count;
  file_read_bytes(file, &count, sizeof(count));

  // Read the asset

  for(u16 i = 0; i < count; i++) {
    // Read the name

    String name;
    file_read_bytes(file, &name);

    // Read the LUA file

    String src;
    file_read_bytes(file, &src);

    // Add the LUA state to the group
    group.named_ids[name] = asset_group_push_lua_state(group.id, src); 

    FREYA_LOG_DEBUG("Loaded LUA state \'%s\' from frpkg ", name.c_str());
  }
}

static bool build_package(const FilePath& list_path, const FilePath& output_path) {
  // Load the frlist file

  ListContext list_ctx;
  if(!list_context_load(list_ctx, list_path)) {
    FREYA_LOG_ERROR("Failed to read asset list file at \'%s\'", list_path.c_str());
    return false;
  }

  // Open the frpkg file to write to later
 
  File pkg_file;
  i32 file_flags = (i32)(FILE_OPEN_WRITE | FILE_OPEN_BINARY);

  if(!file_open(pkg_file, output_path, file_flags)) {
    FREYA_LOG_ERROR("Failed to open frpkg file at \'%s\'", output_path.c_str());
    return false;
  }

  // Write the version
  file_write_bytes(pkg_file, &FRPKG_VALID_VERSION, sizeof(FRPKG_VALID_VERSION));

  // Write the number of sections
  
  u8 sections_count = (u8)list_ctx.sections.size();
  file_write_bytes(pkg_file, &sections_count, sizeof(sections_count));

  // Convert each asset in the list and write it to the newly-create frpkg file

  FREYA_LOG_DEBUG("Converting assets from \'%s\' to \'%s\'", list_ctx.parent_dir.c_str(), output_path.c_str());

  for(auto& section : list_ctx.sections) {
    // Write the asset type
     
    u8 asset_type = (u8)section.type;
    file_write_bytes(pkg_file, &asset_type, sizeof(asset_type));

    // Write the actual assets

    switch(section.type) {
      case ASSET_TYPE_TEXTURE:
        build_textures(pkg_file, section);
        break;
      case ASSET_TYPE_CUBEMAP:
        build_cubemaps(pkg_file, section);
        break;
      case ASSET_TYPE_MODEL:
        build_models(pkg_file, section);
        break;
      case ASSET_TYPE_FONT:
        build_fonts(pkg_file, section);
        break;
      case ASSET_TYPE_AUDIO_BUFFER:
        build_audio_buffers(pkg_file, section);
        break;
      case ASSET_TYPE_LUA:
        build_lua_state(pkg_file, section);
        break;
      default:
        break;
    }
  }

  // Done!
  
  file_close(pkg_file);
  list_context_unload(list_ctx);

  FREYA_LOG_DEBUG("Successfully built frpkg at \'%s\'!", output_path.c_str());

  return true;
}

/// Private functions 
/// ----------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Asset manager functions

void asset_manager_init() {
  // Create the cache asset group

  s_manager.cache_id               = AssetGroupID(ASSET_CACHE_ID);
  s_manager.groups[ASSET_CACHE_ID] = AssetGroup {
    .name = "cache", 
    .id   = s_manager.cache_id,
  };

  // Done!
  FREYA_LOG_INFO("Successfully created the global asset manager");
}

void asset_manager_shutdown() {
  // Shutdown the cache asset group
  asset_group_destroy(s_manager.cache_id); 

  // Done!
  FREYA_LOG_INFO("The global asset manager was successfully shutdown");
}

AssetGroup& asset_manager_get_group(const AssetGroupID& group_id) {
  return s_manager.groups[group_id.get_id()];
}

/// Asset manager functions
///---------------------------------------------------------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// AssetGroupID functions

AssetGroupID asset_group_create(const String& name) {
  // Group init

  AssetGroupID id               = AssetGroupID((i32)random_u32() + 1);
  s_manager.groups[id.get_id()] = AssetGroup {
    .name = name, 
    .id   = id,
  };

  // Done!

  FREYA_LOG_INFO("Successfully created an asset group \'%s\'", name.c_str());
  return id;
}

void asset_group_clear(const AssetGroupID& group_id) {
  GROUP_CHECK(group_id);
  AssetGroup& group = s_manager.groups[group_id.get_id()];

  //
  // Destroy GFX compound assets
  //

  for(auto& asset : group.fonts) {
    delete asset;
  }
  group.fonts.clear();

  for(auto& asset : group.models) {
    delete asset;
  }
  group.models.clear();

  //
  // Destroy GFX assets
  //

  for(auto& buff : group.buffers) {
    sg_destroy_buffer(buff);
  }
  group.buffers.clear();

  for(auto& tex : group.textures) {
    sg_destroy_image(tex.image);
    sg_destroy_view(tex.view);
    sg_destroy_sampler(tex.sampler);
  }
  group.textures.clear();

  for(auto& shader : group.shaders) {
    sg_destroy_shader(shader);
  }
  group.shaders.clear();

  // 
  // Destroy other assets
  //

  group.meshes.clear();
  group.materials.clear();

  for(auto& asset : group.lua_states) {
    lua_close(asset);
  }
  group.lua_states.clear();

  // Destroy any other members
  group.named_ids.clear();

  // Done!
  FREYA_LOG_INFO("Asset group \'%s\' was successfully cleared", group.name.c_str());
}

void asset_group_destroy(const AssetGroupID& group_id) {
  // Clear the group
  asset_group_clear(group_id);

  // Destroy anything that survived the clear
  
  AssetGroup& group = s_manager.groups[group_id.get_id()];
  for(auto& watcher : group.watchers) {
    filewatcher_destroy(watcher);
  }
  
  // Done!

  FREYA_LOG_INFO("Asset group \'%s\' was successfully destroyed", group.name.c_str());
  s_manager.groups.erase(group_id.get_id());
}

void asset_group_reload(const AssetGroupID& group_id) {
  // Clear the group
  asset_group_clear(group_id);

  // Load the package again
  
  AssetGroup& group = s_manager.groups[group_id.get_id()];
  asset_group_load_package(group_id, group.frpkg_path);
}

bool asset_group_build(const AssetGroupID& group_id, 
                       const FilePath& list_path, 
                       const FilePath& output_path, 
                       const bool force_build) {
  GROUP_CHECK(group_id);

  AssetGroup& group    = s_manager.groups[group_id.get_id()];
  FilePath assets_path = filepath_parent_path(list_path);

  // Create a watcher to watch the main asset files 
  // @TEMP

  FilePath paths[] = {
    "textures", 
    "cubemaps", 
    "fonts",
    "audio", 
    "lua",
  };

  for(sizei i = 0; i < group.watchers.size(); i++) {
    if(group.watchers[i]) {
      continue;
    }

    if(!filesystem_exists(paths[i])) { // Avoid ghost directories
      continue;
    }

    FilePath dir = filepath_append(assets_path, paths[i]);
    FREYA_LOG_DEBUG("Watching directory \'%s\'", dir.c_str());

    group.watchers[i] = filewatcher_create(dir, [=](const FilePath& path, const FileStatus status) {
      switch(status) {
        case FILE_STATUS_MODIFIED:
          build_package(list_path, output_path);
          asset_group_reload(group_id);
          break;
        default:
          break;
      }
    });
  }

  // We need to check if it's even needed to build the package. 
  // For example, it might already be up-to-date.

  if(!can_build_frpkg(assets_path, output_path) && !force_build) {
    FREYA_LOG_DEBUG("Frpkg at \'%s\' is up-to-date", output_path.c_str());
    return true; // No need to build the package... 
  }

  // Done!
  return build_package(list_path, output_path);
}

AssetID asset_group_push_buffer(const AssetGroupID& group_id, const sg_buffer_desc& buff_desc) {
  GROUP_CHECK(group_id);
  AssetGroup& group = s_manager.groups[group_id.get_id()];

  // Create and push the buffer

  sg_buffer buffer = sg_make_buffer(buff_desc); 

  AssetID id;
  PUSH_ASSET(group, buffers, buffer, ASSET_TYPE_BUFFER, id);

  // Some useful debug info
  
  FREYA_LOG_DEBUG("Group \'%s\' pushed buffer:", group.name.c_str());
  FREYA_LOG_DEBUG("     Size = %zu", buff_desc.data.size);

  // Done!
  return id;
}

AssetID asset_group_push_texture(const AssetGroupID& group_id, 
                                 const sg_image_desc& image_desc, 
                                 const sg_sampler_desc& sampler_desc) {
  GROUP_CHECK(group_id);
  AssetGroup& group = s_manager.groups[group_id.get_id()];
  
  // Create the texture
  
  Texture texture;

  texture.id    = (i32)group.textures.size();
  texture.size  = Vec2(image_desc.width, image_desc.height);
  texture.image = sg_make_image(image_desc);

  sg_view_desc view_desc  = {};
  view_desc.texture.image = texture.image; 

  texture.view    = sg_make_view(view_desc);
  texture.sampler = sg_make_sampler(sampler_desc); 

  // Push the texture

  AssetID id; 
  PUSH_ASSET(group, textures, texture, ASSET_TYPE_TEXTURE, id);
 
  // Some useful debug info
  
  FREYA_LOG_DEBUG("Group \'%s\' pushed texture:", group.name.c_str());
  FREYA_LOG_DEBUG("     Size = %i X %i", image_desc.width, image_desc.height);
 
  // Done!
  return id;
}

AssetID asset_group_push_texture(const AssetGroupID& group_id, const MaterialTextureType type) {
  // Image init

  sg_image_desc img_desc = {};
  
  img_desc.width  = 16;
  img_desc.height = 16;

  u32 channels = 4;

  u8 pixels[1024]; // width (16) * height (16) * channels (4)

  switch(type) {
    case MATERIAL_TEXTURE_ALBEDO: 
    case MATERIAL_TEXTURE_ROUGHNESS: 
    case MATERIAL_TEXTURE_METALLIC: 
      memory_set(pixels, 0xffffffff, sizeof(pixels));
      break;
    case MATERIAL_TEXTURE_NORMAL: {
      // Initializing the R and G channels to 0 
      // and the B and A pixels to 1. That way, 
      // normal textures look towards the camera 
      // with complete transparency.
      
      for(i32 i = 0; i < img_desc.width; i++) {
        for(i32 j = 0; j < img_desc.height; j++) {
          u32 index = (i * img_desc.width) + j;
          u32 pixel = index * channels; // Index * channels

          pixels[pixel + 0] = 0;
          pixels[pixel + 1] = 0;
          pixels[pixel + 2] = 255;
          pixels[pixel + 3] = 255;
        }
      }
    } break;
  }

  img_desc.data.mip_levels[0] = SG_RANGE(pixels);

  // Sampler init
  sg_sampler_desc samp_desc = {};

  // Done!
  return asset_group_push_texture(group_id, img_desc, samp_desc);
}

AssetID asset_group_push_cubemap(const AssetGroupID& group_id, 
                                 const sg_image_desc& image_desc, 
                                 const sg_sampler_desc& sampler_desc) {
  GROUP_CHECK(group_id);
  AssetGroup& group = s_manager.groups[group_id.get_id()];
  
  // Create the cubemap
  
  Cubemap cubemap;

  cubemap.face_size   = Vec2(image_desc.width, image_desc.height);
  cubemap.faces_count = CUBEMAP_FACES_MAX;
  
  cubemap.id = (i32)group.cubemaps.size();
  
  cubemap.image = sg_make_image(image_desc);

  sg_view_desc view_desc  = {};
  view_desc.texture.image = cubemap.image; 

  cubemap.view    = sg_make_view(view_desc);
  cubemap.sampler = sg_make_sampler(sampler_desc); 

  // Push the cubemap

  AssetID id; 
  PUSH_ASSET(group, cubemaps, cubemap, ASSET_TYPE_CUBEMAP, id);
 
  // Some useful debug info
  
  FREYA_LOG_DEBUG("Group \'%s\' pushed cubemap:", group.name.c_str());
  FREYA_LOG_DEBUG("     Size = %i X %i", image_desc.width, image_desc.height);
 
  // Done!
  return id;
}

AssetID asset_group_push_shader(const AssetGroupID& group_id, const sg_shader_desc& shader_desc) {
  GROUP_CHECK(group_id);
  AssetGroup& group = s_manager.groups[group_id.get_id()];
  
  // Create and load the shader
  sg_shader shader = sg_make_shader(shader_desc);

  // New shader ID

  AssetID id; 
  PUSH_ASSET(group, shaders, shader, ASSET_TYPE_SHADER, id);
  
  // Some useful debug info
  
  FREYA_LOG_DEBUG("Group \'%s\' pushed shader:", group.name.c_str());
  if(shader_desc.compute_func.source) {
    FREYA_LOG_DEBUG("     Compute source length = %zu", strlen(shader_desc.compute_func.source));
  }
  else {
    FREYA_LOG_DEBUG("     Vertex source length = %zu", strlen(shader_desc.vertex_func.source));
    FREYA_LOG_DEBUG("     Pixel source length  = %zu", strlen(shader_desc.fragment_func.source));
  }

  // Done!
  return id;
}

AssetID asset_group_push_mesh(const AssetGroupID& group_id, 
                              const sg_buffer_desc& vertex_desc,
                              const sg_buffer_desc& index_desc, 
                              const u32 elements_count) {

  GROUP_CHECK(group_id);
  AssetGroup& group = s_manager.groups[group_id.get_id()];

  // Buffers init

  AssetID vertex_id = asset_group_push_buffer(group_id, vertex_desc);
  AssetID index_id  = asset_group_push_buffer(group_id, index_desc);
  
  // Create the mesh

  Mesh mesh = {
    .vertex_buffer = asset_group_get_buffer(vertex_id),
    .index_buffer  = asset_group_get_buffer(index_id),

    .elements_count = (u32)elements_count,
  };

  // New mesh ID

  AssetID id; 
  PUSH_ASSET(group, meshes, mesh, ASSET_TYPE_MESH, id);
  
  // Done!
  
  FREYA_LOG_DEBUG("Group \'%s\' pushed mesh:", group.name.c_str());
  return id;
}

AssetID asset_group_push_mesh(const AssetGroupID& group_id, 
                              const DynamicArray<Vertex3D>& vertices, 
                              const DynamicArray<u16>& indices) {
  GROUP_CHECK(group_id);
  AssetGroup& group = s_manager.groups[group_id.get_id()];

  // Vertex buffer init

  sg_buffer_desc vert_desc = {};
  vert_desc.data           = sg_range{vertices.data(), vertices.size() * sizeof(Vertex3D)};

  // Index buffer init
  
  sg_buffer_desc idx_desc = {};

  idx_desc.usage.index_buffer = true;
  idx_desc.data               = sg_range{indices.data(), indices.size() * sizeof(u16)};

  // Create the mesh

  AssetID id = asset_group_push_mesh(group_id, vert_desc, idx_desc, indices.size());
  FREYA_LOG_DEBUG("     Vertices = %zu", vertices.size());
  FREYA_LOG_DEBUG("     Indices  = %zu", indices.size());

  // Done!
  return id;
} 

AssetID asset_group_push_mesh(const AssetGroupID& group_id, const MeshType type) {
  GROUP_CHECK(group_id);
  AssetGroup& group = s_manager.groups[group_id.get_id()];

  // Generate the mesh with the same given type

  MeshLoader loader;
  mesh_loader_load(loader, type);

  // Done!
  return asset_group_push_mesh(group_id, loader.vertices, loader.indices);
}

AssetID asset_group_push_material(const AssetGroupID& group_id, const MaterialDesc& desc) {
  GROUP_CHECK(group_id);
  AssetGroup& group = s_manager.groups[group_id.get_id()];

  // Create the material

  Material material = {};

  material.name  = desc.name;
  material.color = desc.color;

  material.roughness = desc.roughness;
  material.metallic  = desc.metallic;

  material.shadows_enabled = desc.shadows_enabled;

  // Set the textures 

  const RendererDefaults& defaults = renderer_get_defaults();

  material.albedo_map    = defaults.albedo_map;
  material.roughness_map = defaults.roughness_map;
  material.metallic_map  = defaults.metallic_map;
  material.normal_map    = defaults.normal_map;

  if(desc.albedo_id.is_valid()) {
    material.albedo_map = asset_group_get_texture(desc.albedo_id);
  }

  if(desc.roughness_id.is_valid()) {
    material.roughness_map = asset_group_get_texture(desc.roughness_id);
  }
  
  if(desc.metallic_id.is_valid()) {
    material.metallic_map = asset_group_get_texture(desc.metallic_id);
  }

  if(desc.normal_id.is_valid()) {
    material.normal_map = asset_group_get_texture(desc.normal_id);
  }

  // New material ID

  AssetID id; 
  PUSH_ASSET(group, materials, material, ASSET_TYPE_MATERIAL, id);
  
  // Some useful debug info
  
  FREYA_LOG_DEBUG("Group \'%s\' pushed material:", group.name.c_str());
  FREYA_LOG_DEBUG("     Name      = \'%s\'", material.name.c_str());
  FREYA_LOG_DEBUG("     Color     = \'%f, %f, %f, %f\'", material.color.r, material.color.g, material.color.g, material.color.a);
  FREYA_LOG_DEBUG("     Roughness = \'%f\'", material.roughness);
  FREYA_LOG_DEBUG("     Metallic  = \'%f\'", material.metallic);
  FREYA_LOG_DEBUG("     Shadows   = %s", (material.shadows_enabled ? "true" : "false"));

  // Done!
  return id;
}

AssetID asset_group_push_model(const AssetGroupID& group_id, 
                               const DynamicArray<AssetID>& meshes, 
                               const DynamicArray<AssetID>& materials) {
  GROUP_CHECK(group_id);
  AssetGroup& group = s_manager.groups[group_id.get_id()];
 
  // Allocate the model 
  Model* model = new Model{};

  // Push all of the meshes
  // @TODO (Assets/model): Copying the meshes... is that a good idea??

  for(auto& mesh : meshes) {
    model->meshes.emplace_back(asset_group_get_mesh(mesh));
  }

  // Push all of the materials 
  
  for(auto& mat : materials) {
    model->materials.emplace_back(asset_group_get_material(mat));
  }

  // New model ID

  AssetID id; 
  PUSH_ASSET(group, models, model, ASSET_TYPE_MODEL, id);
  
  // Some useful debug info
  
  FREYA_LOG_DEBUG("Group \'%s\' pushed model:", group.name.c_str());
  FREYA_LOG_DEBUG("     Meshes    = \'%zu\'", model->meshes.size());
  FREYA_LOG_DEBUG("     Materials = \'%zu\'", model->materials.size());

  // Done!
  return id;
}

AssetID asset_group_push_font(const AssetGroupID& group_id, const DynamicArray<u8>& font_data, const String& name) {
  GROUP_CHECK(group_id);
  AssetGroup& group = s_manager.groups[group_id.get_id()];
  
  // Allocate a new font
 
  FONScontext* fons = (FONScontext*)renderer_get_font_context();

  Font* font = new Font{name, font_data};
  font->_id  = fonsAddFontMem(fons, font->name.c_str(), (u8*)font->font_data.data(), font->font_data.size(), false);

  // Check for errors

  if(font->_id == -1) {
    FREYA_LOG_ERROR("Failed to load font '\%s\'", name.c_str());
    return AssetID{};
  }

  // New font added!
  
  AssetID id;
  PUSH_ASSET(group, fonts, font, ASSET_TYPE_FONT, id);

  // Some useful debug info
  
  FREYA_LOG_DEBUG("Group \'%s\' pushed a font:", group.name.c_str());
  FREYA_LOG_DEBUG("     Data size = %zu", font_data.size());

  // Done!
  return id;
}

AssetID asset_group_push_audio_buffer(const AssetGroupID& group_id, const AudioBufferDesc& audio_desc) {
  GROUP_CHECK(group_id);
  AssetGroup& group = s_manager.groups[group_id.get_id()];
  
  // Create a new audio buffer
  AudioBufferID buffer = audio_buffer_create(audio_desc);

  // New audio buffer added!
  
  AssetID id;
  PUSH_ASSET(group, audio_buffers, buffer, ASSET_TYPE_AUDIO_BUFFER, id);

  // Some useful debug info
  
  FREYA_LOG_DEBUG("Group \'%s\' pushed an audio buffer:", group.name.c_str());
  FREYA_LOG_DEBUG("     Channels    = %i", audio_desc.channels);
  FREYA_LOG_DEBUG("     Size        = %zu", audio_desc.size);
  FREYA_LOG_DEBUG("     Sample Rate = %zu", audio_desc.sample_rate);

  // Done!
  return id;
}

AssetID asset_group_push_lua_state(const AssetGroupID& group_id, const String& lua_source) {
  GROUP_CHECK(group_id);
  AssetGroup& group = s_manager.groups[group_id.get_id()];
 
  // @TEMP(LUA): We'll probably have a dedicated system to handle all of this 
  // instead of just leaving it inside the asset group.

  // Create a new LUA config
  
  lua_State* state = luaL_newstate();

  luaopen_base(state);
  luaopen_table(state);

  // Load the LUA file

  i32 load_res = luaL_loadstring(state, lua_source.c_str());
  if(load_res != LUA_OK) {
    FREYA_LOG_WARN("LUA-ERROR: %s", lua_tostring(state, -1));
    lua_pop(state, 1);

    return AssetID{};
  }

  // Run the whole LUA file

  i32 run_res = lua_pcall(state, 0, 0, 0);
  if(run_res != LUA_OK) {
    FREYA_LOG_WARN("LUA-ERROR: %s", lua_tostring(state, -1));
    lua_pop(state, 1);

    return AssetID{};
  }

  // New LUA state added!
  
  AssetID id;
  PUSH_ASSET(group, lua_states, state, ASSET_TYPE_LUA, id);

  // Some useful debug info
  
  FREYA_LOG_DEBUG("Group \'%s\' pushed a new LUA state:", group.name.c_str());
  FREYA_LOG_DEBUG("     Length = %zu", lua_source.size());

  // Done!
  return id;
}

bool asset_group_load_package(const AssetGroupID& group_id, const FilePath& frpkg_path) {
  GROUP_CHECK(group_id);
  AssetGroup& group = s_manager.groups[group_id.get_id()];

  // Open the frpkg file

  File file; 
  i32 file_flags = (i32)(FILE_OPEN_READ | FILE_OPEN_BINARY);

  if(!file_open(file, frpkg_path, file_flags)) {
    FREYA_LOG_ERROR("Failed to open frpkg file at '\%s\'", frpkg_path.c_str());
    return false;
  }

  group.frpkg_path = frpkg_path;

  // Read and check the version

  u8 version;
  file_read_bytes(file, &version, sizeof(version));

  if(version != FRPKG_VALID_VERSION) {
    FREYA_LOG_ERROR("Invalid frpkg version found \'%s\'. Expected \'%i\', but found \'%i\'", frpkg_path.c_str(), FRPKG_VALID_VERSION, version);
    return false;
  }

  // Read the sections count

  u8 sections_count;
  file_read_bytes(file, &sections_count, sizeof(sections_count));

  //
  // Read and add all of the assets in the package file
  //
  
  FREYA_LOG_DEBUG("Loading assets from \'%s\'...", frpkg_path.c_str());

  for(u8 i = 0; i < sections_count; i++) {
    // Read the asset type

    u8 asset_type;
    file_read_bytes(file, &asset_type, sizeof(asset_type));

    // Read the asset depending on its type

    switch(asset_type) {
      case ASSET_TYPE_TEXTURE:
        read_textures(file, group);
        break;
      case ASSET_TYPE_CUBEMAP:
        read_cubemaps(file, group);
        break;
      case ASSET_TYPE_MODEL:
        read_models(file, group);
        break;
      case ASSET_TYPE_FONT:
        read_fonts(file, group);
        break;
      case ASSET_TYPE_AUDIO_BUFFER:
        read_audio_buffers(file, group);
        break;
      case ASSET_TYPE_LUA:
        read_lua_state(file, group);
        break;
      default:
        break;
    }
  }

  // Send an event for the rest of the engine
  
  Event event = {
    .type     = EVENT_ASSET_GROUP_LOADED,
    .group_id = group_id, 
  };
  event_dispatch(event);

  // Done!
  
  file_close(file);
  return true;
}

const AssetID& asset_group_get_id(const AssetGroupID& group_id, const String& asset_name) {
  GROUP_CHECK(group_id);
  AssetGroup& group = s_manager.groups[group_id.get_id()];
 
  // The asset was not found
  
  if(group.named_ids.find(asset_name) == group.named_ids.end()) {
    FREYA_LOG_ERROR("Could not find asset \'%s\' in asset group \'%s\'", asset_name.c_str(), group.name.c_str());
    return group.named_ids["invalid"];
  }

  // Done!
  return group.named_ids[asset_name];
}

sg_buffer asset_group_get_buffer(const AssetID& id) {
  AssetGroup& group = s_manager.groups[id.get_group_id()];
  return get_asset(id, group.buffers, ASSET_TYPE_BUFFER);
}

Texture& asset_group_get_texture(const AssetID& id) {
  AssetGroup& group = s_manager.groups[id.get_group_id()];
  return get_asset(id, group.textures, ASSET_TYPE_TEXTURE);
}

Cubemap& asset_group_get_cubemap(const AssetID& id) {
  AssetGroup& group = s_manager.groups[id.get_group_id()];
  return get_asset(id, group.cubemaps, ASSET_TYPE_CUBEMAP);
}

sg_shader asset_group_get_shader(const AssetID& id) {
  AssetGroup& group = s_manager.groups[id.get_group_id()];
  return get_asset(id, group.shaders, ASSET_TYPE_SHADER);
}

Mesh& asset_group_get_mesh(const AssetID& id) {
  AssetGroup& group = s_manager.groups[id.get_group_id()];
  return get_asset(id, group.meshes, ASSET_TYPE_MESH);
}

Material& asset_group_get_material(const AssetID& id) {
  AssetGroup& group = s_manager.groups[id.get_group_id()];
  return get_asset(id, group.materials, ASSET_TYPE_MATERIAL);
}

Model* asset_group_get_model(const AssetID& id) {
  AssetGroup& group = s_manager.groups[id.get_group_id()];
  return get_asset(id, group.models, ASSET_TYPE_MODEL);
}

Font* asset_group_get_font(const AssetID& id) {
  AssetGroup& group = s_manager.groups[id.get_group_id()];
  return get_asset(id, group.fonts, ASSET_TYPE_FONT);
}

const AudioBufferID& asset_group_get_audio_buffer(const AssetID& id) {
  AssetGroup& group = s_manager.groups[id.get_group_id()];
  return get_asset(id, group.audio_buffers, ASSET_TYPE_AUDIO_BUFFER);
}

lua_State* asset_group_get_lua_state(const AssetID& id) {
  AssetGroup& group = s_manager.groups[id.get_group_id()];
  return get_asset(id, group.lua_states, ASSET_TYPE_LUA);
}

/// AssetGroupID functions
/// ----------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
