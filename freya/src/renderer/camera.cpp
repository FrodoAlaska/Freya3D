#include "freya_render.h"
#include "freya_input.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

///---------------------------------------------------------------------------------------------------------------------
/// PlaneType
enum PlaneType {
  PLANE_TOP, 
  PLANE_BOTTOM,
  PLANE_LEFT, 
  PLANE_RIGHT,
  PLANE_NEAR, 
  PLANE_FAR,
};
/// PlaneType
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Private functions

static void update_camera_frustums(Camera& camera) {
  // Calculate the height and width of the near plane  

  f32 half_tan = (f32)freya::tan(FREYA_TO_RADIANS(camera.fov) / 2.0f);

  camera.near_height = camera.near_plane  * half_tan; 
  camera.near_width  = camera.near_height * camera.aspect;

  // Get the required vectors 

  Vec3 right = Vec3(camera.view[0][0], camera.view[0][1], camera.view[0][2]);
  Vec3 up    = Vec3(camera.view[1][0], camera.view[1][1], camera.view[1][2]);
  Vec3 front = Vec3(camera.view[2][0], camera.view[2][1], camera.view[2][2]);

  // Get the world-space center points of the near and far planes 

  Vec3 near_center = camera.position - front * camera.near_plane;
  Vec3 far_center  = camera.position - front * camera.far_plane;

  // 
  // Calculate the planes 
  //

  Vec3 point, normal;

  // Near and far 
  
  plane_create(camera.planes[PLANE_NEAR], -front, near_center);
  plane_create(camera.planes[PLANE_FAR], front, far_center);

  // Top 

  point  = near_center + up * camera.near_height; 
  normal = vec3_cross(vec3_normalize(point - camera.position), right); 

  plane_create(camera.planes[PLANE_TOP], normal, point);

  // Bottom

  point  = near_center - up * camera.near_height; 
  normal = vec3_cross(right, vec3_normalize(point - camera.position)); 

  plane_create(camera.planes[PLANE_BOTTOM], normal, point);

  // Left

  point  = near_center - right * camera.near_width; 
  normal = vec3_cross(vec3_normalize(point - camera.position), up); 

  plane_create(camera.planes[PLANE_LEFT], normal, point);
  
  // Right

  point  = near_center + right * camera.near_width; 
  normal = vec3_cross(up, vec3_normalize(point - camera.position)); 

  plane_create(camera.planes[PLANE_RIGHT], normal, point);
}

/// Private functions
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Camera functions

void camera_create(Camera& out_camera, const CameraDesc& desc) {
  out_camera.fov      = desc.fov;
  out_camera.exposure = desc.exposure;

  out_camera.near_plane = desc.near_plane;
  out_camera.far_plane  = desc.far_plane;

  out_camera.near_width  = 0.0f; 
  out_camera.near_height = 0.0f;

  out_camera.aspect = renderer_get_aspect_ratio();
  out_camera.view   = Mat4(1.0f);

  update_camera_frustums(out_camera);
}

bool camera_check_bounds(const Camera& camera, const BoundingBox& bounds) {
  bool result = true;

  // Go through each plane and check against the bounds
  //
  // @NOTE: The code here was taken from the source below: 
  // https://github.com/Angelo1211/HybridRenderingEngine/blob/master/src/geometry.cpp
  //

  for(sizei i = 0; i < 6; i++) {
    // Check against each corner of the box
    
    i32 out = 0; 

    out += ((plane_get_distance(camera.planes[i], bounds.min) < 0.0f) ? 1 : 0);
    out += ((plane_get_distance(camera.planes[i], Vec3(bounds.max.x, bounds.min.y, bounds.min.z)) < 0.0f) ? 1 : 0);
    out += ((plane_get_distance(camera.planes[i], Vec3(bounds.min.x, bounds.max.y, bounds.min.z)) < 0.0f) ? 1 : 0);
    out += ((plane_get_distance(camera.planes[i], Vec3(bounds.max.x, bounds.max.y, bounds.min.z)) < 0.0f) ? 1 : 0);
    out += ((plane_get_distance(camera.planes[i], Vec3(bounds.min.x, bounds.min.y, bounds.max.z)) < 0.0f) ? 1 : 0);
    out += ((plane_get_distance(camera.planes[i], Vec3(bounds.max.x, bounds.min.y, bounds.max.z)) < 0.0f) ? 1 : 0);
    out += ((plane_get_distance(camera.planes[i], Vec3(bounds.min.x, bounds.max.y, bounds.min.z)) < 0.0f) ? 1 : 0);
    out += ((plane_get_distance(camera.planes[i], bounds.max) < 0.0f) ? 1 : 0);

    // Check if we missed any corners

    if (out != 8) {
      continue;
    }

    // We missed every single one!
    
    result = false; 
    break;
  }

  // Done!
  return result;
}

/// Camera functions
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
    update_camera_frustums(camera);
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
    update_camera_frustums(camera);
  }
}

/// CameraController functions
///---------------------------------------------------------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
