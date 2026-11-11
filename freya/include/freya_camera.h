#pragma once

#include "freya_math.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

///---------------------------------------------------------------------------------------------------------------------
/// Consts

/// The maximum degrees the camera can achieve. 
const f32 CAMERA_MAX_DEGREES         = 89.0f;

/// The maximum amount of zoom the camera can achieve.
const f32 CAMERA_MAX_ZOOM            = 180.0f;

/// The maximum amount of corners a camera's frustum can have.
const u32 CAMERA_FRUSTUM_CORNERS_MAX = 8;

/// Consts
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// CameraControllerType
enum CameraControllerType {
  CAMERA_CONTROLLER_FREE, 
  CAMERA_CONTROLLER_FPS,
};
/// CameraControllerType
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// CameraDesc
struct CameraDesc {
  /// The near distance of the camera. 
  ///
  /// @NOTE: This is set to `0.1f` by default.
  f32 near_plane = 0.1f;

  /// The far distance of the camera. 
  ///
  /// @NOTE: This is set to `100.0f` by default.
  f32 far_plane  = 100.0f;

  /// The FOV value of the camera. 
  ///
  /// @NOTE: This is set to `45.0f` by default.
  f32 fov        = 45.0f;

  /// The exposure value of the camera. 
  ///
  /// @NOTE: This is set to `1.0f` by default.
  f32 exposure   = 1.0f;
};
/// CameraDesc
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Camera 
struct Camera {
  f32 fov;
  f32 exposure;

  f32 near_plane; 
  f32 far_plane;

  Vec3 position; 
  Quat rotation;

  Mat4 transform, view;
};
/// Camera 
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// CameraController
struct CameraController {
  CameraControllerType type;

  f32 yaw, pitch, sensitivity;
  f32 speed;

  Vec3 front, up;
};
/// CameraController
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// CameraController functions

/// Move the given `camera` in free mode according to the information in `controller`, where speed is scaled to `delta_time`
FREYA_API void camera_controller_free(CameraController& controller, Camera& camera, f32 delta_time);

/// Move the given `camera` in FPS mode according to the information in `controller`, where speed is scaled to `delta_time`
FREYA_API void camera_controller_fps(CameraController& controller, Camera& camera, f32 delta_time);

/// CameraController functions
///---------------------------------------------------------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
