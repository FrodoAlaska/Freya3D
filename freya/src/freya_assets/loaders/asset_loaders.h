#pragma once

#include "freya_assets.h"

/// ----------------------------------------------------------------------
/// TextureLoader
struct TextureLoader {
  sg_image_desc desc; 
  void* data;
};
/// TextureLoader
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// CubemapLoader
struct CubemapLoader {
  sg_image_desc desc;
  freya::u8* data;
};
/// CubemapLoader
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// MeshLoader 
struct MeshLoader {
  freya::DynamicArray<freya::Vertex3D> vertices; 
  freya::DynamicArray<freya::u16> indices;

  freya::u8 material_index = 0;
};
/// MeshLoader 
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// MaterialLoader
struct MaterialLoader {
  freya::String name; 

  freya::f32 roughness;
  freya::f32 metallic;

  freya::Vec3 color; 

  freya::i8 albedo_index    = -1;
  freya::i8 roughness_index = -1;
  freya::i8 metallic_index  = -1;
  freya::i8 normal_index    = -1;
};
/// MaterialLoader
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// ModelLoader
struct ModelLoader {
  freya::DynamicArray<MeshLoader> meshes;
  freya::DynamicArray<TextureLoader> textures;
  freya::DynamicArray<MaterialLoader> materials;
};
/// ModelLoader
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// FontLoader
struct FontLoader {
  freya::DynamicArray<freya::u8> bytes;
};
/// FontLoader
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// LuaLoader
struct LuaLoader {
  freya::String source;
};
/// LuaLoader
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Texture loader functions

bool texture_loader_load(TextureLoader& out_loader, const freya::FilePath& path);

void texture_loader_save(const TextureLoader& loader, freya::File& file);

/// Texture loader functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Cubemap loader functions

bool cubemap_loader_load(CubemapLoader& out_loader, const freya::FilePath& path);

void cubemap_loader_save(const CubemapLoader& loader, freya::File& file);

/// Cubemap loader functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Mesh loader functions

void mesh_loader_load(MeshLoader& out_loader, const freya::MeshType type);

void mesh_loader_save(const MeshLoader& loader, freya::File& file);

/// Mesh loader functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Model loader functions

bool model_loader_load(ModelLoader& out_loader, const freya::FilePath& path);

void model_loader_save(const ModelLoader& loader, freya::File& file);

/// Model loader functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Font loader functions

bool font_loader_load(FontLoader& out_loader, const freya::FilePath& path);

void font_loader_save(const FontLoader& loader, freya::File& file);

/// Font loader functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Audio loader functions

bool audio_loader_load(freya::AudioBufferDesc& out_desc, const freya::FilePath& path);

void audio_loader_save(const freya::AudioBufferDesc& audio_desc, freya::File& file);

void audio_loader_unload(freya::AudioBufferDesc& audio_desc);

/// Audio loader functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// LUA state loader functions

bool lua_state_loader_load(LuaLoader& out_loader, const freya::FilePath& path);

void lua_state_loader_save(const LuaLoader& loader, freya::File& file);

/// LUA state loader functions
/// ----------------------------------------------------------------------
