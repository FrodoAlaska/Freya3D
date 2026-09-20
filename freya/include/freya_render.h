#pragma once

#include "freya_entity.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

///---------------------------------------------------------------------------------------------------------------------
/// Consts

/// The maximum amount of render targets a post-process pass can have. 
const u32 RENDER_TARGETS_MAX         = 8; 

/// The maximum degrees the camera can achieve. 
const f32 CAMERA_MAX_DEGREES         = 89.0f;

/// The maximum amount of zoom the camera can achieve.
const f32 CAMERA_MAX_ZOOM            = 180.0f;

/// The maximum amount of corners a camera's frustum can have.
const u32 CAMERA_FRUSTUM_CORNERS_MAX = 8;

/// Consts
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// CameraDesc
struct CameraDesc {
  /// The initial position of the camera
  Vec3 position = Vec3(0.0f); 

  /// The forward looking target of the camera. 
  ///
  /// @NOTE: This is set to `Vec3(-3.0f, 0.0f, 0.0f)` by default.
  Vec3 target   = Vec3(-3.0f, 0.0f, 0.0f); 

  /// The up vector of the camera. 
  ///
  /// @NOTE: This is set to `Vec3(0.0f, 1.0f, 0.0f)` by default.
  Vec3 up_axis  = Vec3(0.0f, 1.0f, 0.0f);

  /// The aspect ratio of the created camera. 
  /// This can be a completely different value from the 
  /// default window aspect ratio if desired.
  ///
  /// @NOTE: This is set to `0.0f` by default.
  f32 aspect_ratio = 0.0f;

  /// The near distance of the camera. 
  ///
  /// @NOTE: This is set to `0.1f` by default.
  f32 near_plane   = 0.1f;

  /// The far distance of the camera. 
  ///
  /// @NOTE: This is set to `100.0f` by default.
  f32 far_plane    = 100.0f;

  /// The zoom value of the camera. 
  /// This can also be called the FOV of the camera.
  ///
  /// @NOTE: This is set to `45.0f` by default.
  f32 zoom         = 45.0f;
};
/// CameraDesc
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Camera 
struct Camera {
  /// Camera's floats

  f32 yaw, pitch;
  f32 zoom, aspect_ratio;

  f32 near_plane  = 0.1f; 
  f32 far_plane   = 100.0f;
  f32 sensitivity = 0.1f;
  f32 exposure    = 1.0f; 

  /// The camera's vectors

  Vec3 position;
  Vec3 up, direction, front;

  /// The camera's matrices
  Mat4 view, projection, view_projection;

  /// Some state to keep
  Vec3 corners[CAMERA_FRUSTUM_CORNERS_MAX]; // The calculated corners of this camera's frustum.
};
/// Camera 
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// RendererDefaults
struct RendererDefaults {
  Texture diffuse_map;
  Texture specular_map;
  Texture normal_map;
  Texture emissive_map;

  Material material;

  Mesh quad_mesh;
  Mesh skybox_mesh;

  sg_sampler sampler;
};
/// RendererDefaults
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Camera functions

/// A function to mimick a free-form camera movement.
FREYA_API void camera_free_move_func(Camera& camera);

/// A function to mimick a first-person shooter camera movement.
FREYA_API void camera_fps_move_func(Camera& camera);

/// Fill the information in `out_cam` using the given `CameraDesc`.
FREYA_API void camera_create(Camera& out_cam, const CameraDesc& desc);

/// Using, the given `target`, update the internal matrices of `cam`, updating 
/// the direction and front vectors in the process. 
///
/// @NOTE: If the given `target` is set to `nullptr`, the camera will look 
/// along its `front` vector.
FREYA_API void camera_update(Camera& cam, const Vec3* target = nullptr);

/// Have the given `cam` follow the `target`, taking into account the given `offset`.
FREYA_API void camera_follow(Camera& cam, const Vec3& target, const Vec3& offset);

/// Using linear interpolation, have the given `cam` follow `target` by `delta`, taking
/// into account the given `offset`.
FREYA_API void camera_follow_lerp(Camera& cam, const Vec3& target, const Vec3& offset, const f32 delta);

/// Camera functions
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

/// Set whether the renderer should sort the renderable items or not.
FREYA_API void renderer_set_sort(bool sort);

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
