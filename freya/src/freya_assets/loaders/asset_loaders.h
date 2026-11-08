#pragma once

#include "freya_assets.h"

/// ----------------------------------------------------------------------
/// Texture loader functions

bool texture_loader_load(const freya::FilePath& path, sg_image_desc& out_img, void** out_data);

/// Texture loader functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Cubemap loader functions

bool cubemap_loader_load(const freya::FilePath& path, sg_image_desc& out_img, freya::u8** out_data);

/// Cubemap loader functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Font loader functions

bool font_loader_load(const freya::FilePath& path, freya::DynamicArray<freya::u8>& font_data);

/// Font loader functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Audio loader functions

bool audio_loader_load(const freya::FilePath& path, freya::AudioBufferDesc* audio_desc);

void audio_loader_unload(freya::AudioBufferDesc& audio_desc);

/// Audio loader functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// LUA state loader functions

bool lua_state_loader_load(const freya::FilePath& path, freya::String* out_src);

/// LUA state loader functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Mesh loader functions

void mesh_loader_load(const freya::MeshType type, 
                      freya::DynamicArray<freya::Vertex3D>& out_vertices, 
                      freya::DynamicArray<freya::u16>& out_indices);

/// Mesh loader functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Model loader functions

// void model_loader_load(const freya::FilePath& path);

/// Model loader functions
/// ----------------------------------------------------------------------
