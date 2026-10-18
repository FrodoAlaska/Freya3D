#include "freya_render.h"
#include "freya_input.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

///---------------------------------------------------------------------------------------------------------------------
/// CameraController functions

void camera_controller_free(CameraController& controller, Camera& camera, f32 delta_time) {
  // Update the yaw and pitch values 

  Vec2 mouse_offset = input_mouse_offset();

  controller.yaw   = mouse_offset.x * controller.sensitivity;
  controller.pitch = -(mouse_offset.y * controller.sensitivity);

  controller.pitch = clamp_float(controller.pitch, -CAMERA_MAX_DEGREES, CAMERA_MAX_DEGREES);
  camera.fov       = clamp_float(camera.fov, 1.0f, CAMERA_MAX_ZOOM);

  // Update the rotation

  Quat yaw_quat   = quat_angle_axis(Vec3(0.0f, -1.0f, 0.0f), controller.yaw * DEG2RAD);
  Quat pitch_quat = quat_angle_axis(Vec3(-1.0f, 0.0f, 0.0f), controller.pitch * DEG2RAD);

  camera.rotation = yaw_quat * pitch_quat;

  // Update the vectors 

  controller.front = mat4_forward(camera.transform);
  controller.up    = mat4_up(camera.transform);
  
  // Move the controller
  // @TODO (Camera controller): Maybe make the movment more generic? 

  f32 speed = (controller.speed * delta_time);
  
  if(input_key_down(KEY_W)) {
    camera.position += controller.front * speed;
  }
  else if(input_key_down(KEY_S)) {
    camera.position -= controller.front * speed;
  }
 
  if(input_key_down(KEY_A)) {
    camera.position -= vec3_normalize(vec3_cross(controller.front, controller.up)) * speed;
  }
  else if(input_key_down(KEY_D)) {
    camera.position += vec3_normalize(vec3_cross(controller.front, controller.up)) * speed;
  }
}

void camera_controller_fps(CameraController& controller, Camera& camera, f32 delta_time) {
  // Update the yaw and pitch values 

  Vec2 mouse_offset = input_mouse_offset();

  controller.yaw   = mouse_offset.x * controller.sensitivity;
  controller.pitch = -(mouse_offset.y * controller.sensitivity);

  controller.pitch = clamp_float(controller.pitch, -CAMERA_MAX_DEGREES, CAMERA_MAX_DEGREES);
  camera.fov       = clamp_float(camera.fov, 1.0f, CAMERA_MAX_ZOOM);

  // Update the rotation

  Quat yaw_quat   = quat_angle_axis(Vec3(0.0f, -1.0f, 0.0f), controller.yaw * DEG2RAD);
  Quat pitch_quat = quat_angle_axis(Vec3(-1.0f, 0.0f, 0.0f), controller.pitch * DEG2RAD);

  camera.rotation = yaw_quat * pitch_quat;

  // Update the vectors 

  controller.front = mat4_forward(camera.transform);
  controller.up    = mat4_up(camera.transform);
 
  // Move the controller

  f32 speed = (controller.speed * delta_time);
  
  if(input_key_down(KEY_W)) {
    camera.position += Vec3(controller.front.x, 0.0f, controller.front.z) * speed;
  }
  else if(input_key_down(KEY_S)) {
    camera.position -= Vec3(controller.front.x, 0.0f, controller.front.z) * speed;
  }
  
  if(input_key_down(KEY_A)) {
    camera.position -= vec3_normalize(vec3_cross(controller.front, controller.up)) * speed;
  }
  else if(input_key_down(KEY_D)) {
    camera.position += vec3_normalize(vec3_cross(controller.front, controller.up)) * speed;
  }
}

/// CameraController functions
///---------------------------------------------------------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
