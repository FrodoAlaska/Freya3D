#include "freya_render.h"
#include "freya_input.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

///---------------------------------------------------------------------------------------------------------------------
/// Private functions

static void update_camera_frustums(CameraController& controller, Camera& cam) {
  // Calculate required values 

  f32 tan_fov    = freya::tan(FREYA_TO_RADIANS(cam.fov) * 0.5f); // Half the FOV
  Vec3 cam_right = vec3_cross(controller.front, controller.up);

  // Calculating the bounds of the frustrum

  f32 near_height = 2.0f * tan_fov * cam.near_plane;
  f32 near_width  = near_height    * cam.aspect;

  f32 far_height = 2.0f * tan_fov * cam.far_plane;
  f32 far_width  = far_height     * cam.aspect;

  // Calculating the center of both the near and far planes

  Vec3 near_center = cam.position + (controller.front * cam.near_plane);
  Vec3 far_center  = cam.position + (controller.front * cam.far_plane);

  // Calculating each far point in both the near and far planes

  Vec3 near_top   = near_center + controller.up * (near_height * 0.5f); 
  Vec3 near_right = near_center + cam_right     * (near_width  * 0.5f);

  Vec3 far_top   = far_center + controller.up * (far_height * 0.5f); 
  Vec3 far_right = far_center + cam_right     * (far_width  * 0.5f);

  //
  // Filling the given frustums
  //

  // Near plane

  cam.frustums[0] = near_center - near_top - near_right; // Bottom left
  cam.frustums[1] = near_center + near_top - near_right; // Top left
  cam.frustums[2] = near_center + near_top + near_right; // Top right
  cam.frustums[3] = near_center - near_top + near_right; // Bottom right
  
  // Far plane

  cam.frustums[4] = far_center - far_top - far_right; // Bottom left
  cam.frustums[5] = far_center + far_top - far_right; // Top left
  cam.frustums[6] = far_center + far_top + far_right; // Top right
  cam.frustums[7] = far_center - far_top + far_right; // Bottom right
}

/// Private functions
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// CameraController functions

void camera_controller_free(CameraController& controller, Camera& camera, f32 delta_time) {
  // Update the yaw and pitch values 

  Vec2 mouse_offset = input_mouse_offset();

  controller.yaw   = mouse_offset.x   * controller.sensitivity;
  controller.pitch = -(mouse_offset.y * controller.sensitivity);

  controller.pitch = clamp_float(controller.pitch, -CAMERA_MAX_DEGREES, CAMERA_MAX_DEGREES);
  camera.fov       = clamp_float(camera.fov, 1.0f, CAMERA_MAX_ZOOM);

  // Update the rotation

  Quat yaw_quat   = quat_angle_axis(Vec3( 0.0f, -1.0f, 0.0f), controller.yaw   * DEG2RAD);
  Quat pitch_quat = quat_angle_axis(Vec3(-1.0f,  0.0f, 0.0f), controller.pitch * DEG2RAD);

  camera.rotation = yaw_quat * pitch_quat;

  // Update the vectors 

  controller.front = mat4_forward(camera.transform);
  controller.up    = mat4_up(camera.transform);
  
  // Move the controller

  f32 speed    = (controller.speed * delta_time);
  bool updated = false; 
  
  if(input_key_down(KEY_W)) {
    camera.position += controller.front * speed;
    updated          = true;
  }
  else if(input_key_down(KEY_S)) {
    camera.position -= controller.front * speed;
    updated          = true;
  }
 
  if(input_key_down(KEY_A)) {
    camera.position -= vec3_normalize(vec3_cross(controller.front, controller.up)) * speed;
    updated          = true;
  }
  else if(input_key_down(KEY_D)) {
    camera.position += vec3_normalize(vec3_cross(controller.front, controller.up)) * speed;
    updated          = true;
  }

  // Recalculate the camera's frustums
  
  if(updated) {
    update_camera_frustums(controller, camera);
  }
}

void camera_controller_fps(CameraController& controller, Camera& camera, f32 delta_time) {
  // Update the yaw and pitch values 

  Vec2 mouse_offset = input_mouse_offset();

  controller.yaw   = mouse_offset.x   * controller.sensitivity;
  controller.pitch = -(mouse_offset.y * controller.sensitivity);

  controller.pitch = clamp_float(controller.pitch, -CAMERA_MAX_DEGREES, CAMERA_MAX_DEGREES);
  camera.fov       = clamp_float(camera.fov, 1.0f, CAMERA_MAX_ZOOM);

  // Update the rotation

  Quat yaw_quat   = quat_angle_axis(Vec3( 0.0f, -1.0f, 0.0f), controller.yaw   * DEG2RAD);
  Quat pitch_quat = quat_angle_axis(Vec3(-1.0f,  0.0f, 0.0f), controller.pitch * DEG2RAD);

  camera.rotation = yaw_quat * pitch_quat;

  // Update the vectors 

  controller.front = mat4_forward(camera.transform);
  controller.up    = mat4_up(camera.transform);
 
  // Move the controller

  f32 speed    = (controller.speed * delta_time);
  bool updated = false; 
  
  if(input_key_down(KEY_W)) {
    camera.position += Vec3(controller.front.x, 0.0f, controller.front.z) * speed;
    updated          = true;
  }
  else if(input_key_down(KEY_S)) {
    camera.position -= Vec3(controller.front.x, 0.0f, controller.front.z) * speed;
    updated          = true;
  }
  
  if(input_key_down(KEY_A)) {
    camera.position -= vec3_normalize(vec3_cross(controller.front, controller.up)) * speed;
    updated          = true;
  }
  else if(input_key_down(KEY_D)) {
    camera.position += vec3_normalize(vec3_cross(controller.front, controller.up)) * speed;
    updated          = true;
  }

  // Recalculate the camera's frustums
  
  if(updated) {
    update_camera_frustums(controller, camera);
  }
}

/// CameraController functions
///---------------------------------------------------------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
