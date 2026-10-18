#pragma once

#include "freya_entity.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

///---------------------------------------------------------------------------------------------------------------------
/// Consts

/// The maximum amount of render targets a post-process pass can have. 
const u32 RENDER_TARGETS_MAX         = 8; 

/// Consts
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// AnimatorDesc
struct AnimatorDesc {
  /// The IDs of all the texture frames.
  DynamicArray<AssetID> frames_id; 
 
  /// The speed of the animation. 
  f32 flip_speed;

  /// Indicates whether the animation 
  /// can loop to the beginning when finished.
  ///
  /// @NOTE: The default value is `true`.
  bool can_loop      = true;

  /// Indicates whether the animation can 
  /// alternate directions when finished.
  ///
  /// @NOTE: The default value is `true`.
  bool can_alternate = false;
};
/// AnimatorDesc
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Animator
struct Animator {
  Texture* current = nullptr;
  DynamicArray<Texture> frames;

  i32 current_index, direction;

  f32 counter, flip_speed;
  bool is_active, can_loop, can_alternate;
};
/// Animator
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// RendererDefaults
struct RendererDefaults {
  Texture albedo_map;
  Texture roughness_map;
  Texture metallic_map;
  Texture normal_map;

  Material material;

  Mesh quad_mesh;
  Mesh skybox_mesh;
  Mesh billboard_mesh;

  sg_sampler sampler;
};
/// RendererDefaults
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Animator functions

/// Initialize an animation `out_anim` using the information found in `desc`.
FREYA_API void animator_create(Animator& out_anim, const AnimatorDesc& desc);

/// Update the given `anim` scaled to `delta_time`.
FREYA_API void animator_update(Animator& anim, f32 dt);

/// Animator functions
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Renderer functions

/// Initialize the internal data of the renderer.
FREYA_API void renderer_init(Window* window);

/// Shutdown and destroy any resources created by the renderer.
FREYA_API void renderer_shutdown();

/// Apply the given `group_id` to be used internally in the renderer. 
///
/// @NOTE: If no custom `AssetGroupID` is applied, the renderer 
/// will use the default `ASSET_CACHE_ID` group instead.
FREYA_API void renderer_apply_asset_group(const AssetGroupID& group_id);

/// Sumbit the given `world` to be renderer in this frame. 
///
/// @NOTE: This function _MUST_ be called at least once during 
/// the application's runtime. Otherwise, when the engine calls `renderer_prepare`, 
/// the application will assert due to an invalid current world.
FREYA_API void renderer_sumbit_world(EntityWorld* world);

/// Set renderer's clear color to the given `color`.
FREYA_API void renderer_set_clear_color(const Color& color);

/// Retrieve the renderer's current clear color.
FREYA_API const Color& renderer_get_clear_color();

/// Retrieve a default platform-specific swapchain to give to any passes
FREYA_API sg_swapchain renderer_get_default_swapchain();

/// Retrieve the `AssetGroupID` the renderer is currently using.
FREYA_API AssetGroupID& renderer_get_asset_group_id();

/// Retrieve the `RendererDefaults` the renderer is currently holding.
FREYA_API const RendererDefaults& renderer_get_defaults();

///
/// Internal functions 
///

/// Set up the internal render buffers of the renderer using the 
/// `EntityWorld` that was sumbitted using `renderer_sumbit_world`.
///
/// @NOTE: The application does NOT need to call this function. This is 
/// only required to be called by the engine.
FREYA_API void renderer_prepare();

/// Commit and render all of the objects sumbitted to the renderer 
/// and present the last image to the swapchain.
///
/// @NOTE: The application does NOT need to call this function. This is 
/// only required to be called by the engine.
FREYA_API void renderer_commit();

/// Get the internal font context that could be used anywhere else.
FREYA_API void* renderer_get_font_context();

/// Renderer functions
///---------------------------------------------------------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
