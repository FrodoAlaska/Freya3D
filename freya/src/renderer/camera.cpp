#include "freya_render.h"
#include "freya_input.h"
#include "freya_event.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

/// ----------------------------------------------------------------------
/// Camera consts

const f32 CAMERA_SPEED = 20.0f;

/// Camera consts
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Callbacks

static bool window_resize_callback(const Event& event, const void* dispatcher, const void* listener) {
  // Figure out the new size

  IVec2 new_size = Vec2(0.0f);
  switch(event.type) {
    case EVENT_WINDOW_FRAMEBUFFER_RESIZED:
      new_size = event.window_framebuffer_size;
      break;
    case EVENT_WINDOW_RESIZED:
    case EVENT_WINDOW_FULLSCREEN:
      new_size = event.window_size;
      break;
    default:
      return true;
  }

  // Change the aspect ratio 

  if(new_size.x >= 0 && new_size.y >= 0) {
    Camera* cam       = (Camera*)listener;
    cam->aspect_ratio = (f32)(new_size.x / new_size.y);
  }

  // Done!
  return true;
}

/// Callbacks
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Private functions

static void calculate_frustum_corners(Camera& cam) {
  f32 tan_fov    = freya::tan(FREYA_TO_RADIANS(cam.zoom) * 0.5f); // Half the zoom
  Vec3 cam_right = vec3_cross(cam.front, cam.up);

  // Calculating the bounds of the frustrum

  f32 near_height = 2.0f * tan_fov * cam.near_plane;
  f32 near_width  = near_height * cam.aspect_ratio;

  f32 far_height = 2.0f * tan_fov * cam.far_plane;
  f32 far_width  = far_height * cam.aspect_ratio;

  // Calculating the center of both the near and far planes

  Vec3 near_center = cam.position + (cam.front * cam.near_plane);
  Vec3 far_center  = cam.position + (cam.front * cam.far_plane);

  // Calculating each far point in both the near and far planes

  Vec3 near_top   = near_center + cam.up    * (near_height * 0.5f); 
  Vec3 near_right = near_center + cam_right * (near_width  * 0.5f);

  Vec3 far_top   = far_center + cam.up    * (far_height * 0.5f); 
  Vec3 far_right = far_center + cam_right * (far_width  * 0.5f);

  //
  // Filling the given `corners` array with the new calculated values
  //

  // Near plane

  cam.corners[0] = near_center - near_top - near_right; // Bottom left
  cam.corners[1] = near_center + near_top - near_right; // Top left
  cam.corners[2] = near_center + near_top + near_right; // Top right
  cam.corners[3] = near_center - near_top + near_right; // Bottom right
  
  // Far plane

  cam.corners[4] = far_center - far_top - far_right; // Bottom left
  cam.corners[5] = far_center + far_top - far_right; // Top left
  cam.corners[6] = far_center + far_top + far_right; // Top right
  cam.corners[7] = far_center - far_top + far_right; // Bottom right
}

/// Private functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Camera functions

void camera_free_move_func(Camera& camera) {
  float speed       = CAMERA_SPEED * clock_get_delta_time();
  Vec2 mouse_offset = input_mouse_offset(); 

  camera.yaw   = mouse_offset.x * camera.sensitivity;
  camera.pitch = mouse_offset.y * camera.sensitivity;

  camera.pitch = clamp_float(camera.pitch, -CAMERA_MAX_DEGREES, CAMERA_MAX_DEGREES);
  camera.zoom  = clamp_float(camera.zoom, 1.0f, CAMERA_MAX_ZOOM);

  if(input_key_down(KEY_UP)) {
    camera.position += camera.front * speed;
  }
  else if(input_key_down(KEY_DOWN)) {
    camera.position -= camera.front * speed;
  }
 
  if(input_key_down(KEY_RIGHT)) {
    camera.position += vec3_normalize(vec3_cross(camera.front, camera.up)) * speed;
  }
  else if(input_key_down(KEY_LEFT)) {
    camera.position -= vec3_normalize(vec3_cross(camera.front, camera.up)) * speed;
  }
}

void camera_fps_move_func(Camera& camera) {
  float speed       = CAMERA_SPEED * clock_get_delta_time();
  Vec2 mouse_offset = input_mouse_offset(); 

  camera.yaw   = mouse_offset.x * camera.sensitivity;
  camera.pitch = mouse_offset.y * camera.sensitivity;

  camera.pitch = clamp_float(camera.pitch, -CAMERA_MAX_DEGREES, CAMERA_MAX_DEGREES);
  camera.zoom  = clamp_float(camera.zoom, 1.0f, CAMERA_MAX_ZOOM);

  if(input_key_down(KEY_W)) {
    camera.position += Vec3(camera.front.x, 0.0f, camera.front.z) * speed;
  }
  else if(input_key_down(KEY_S)) {
    camera.position -= Vec3(camera.front.x, 0.0f, camera.front.z) * speed;
  }
 
  if(input_key_down(KEY_A)) {
    camera.position -= vec3_normalize(vec3_cross(camera.front, camera.up)) * speed;
  }
  else if(input_key_down(KEY_D)) {
    camera.position += vec3_normalize(vec3_cross(camera.front, camera.up)) * speed;
  }
}

void camera_create(Camera& out_cam, const CameraDesc& desc) {
  out_cam.yaw   = -90.0f; 
  out_cam.pitch = 0.0f; 

  out_cam.zoom         = desc.zoom;
  out_cam.aspect_ratio = desc.aspect_ratio;

  out_cam.near_plane = desc.near_plane;
  out_cam.far_plane  = desc.far_plane;

  out_cam.sensitivity = 0.1f;
  out_cam.exposure    = 1.0f;

  Vec3 look_dir(desc.position - desc.target);
  Vec3 right_axis(vec3_cross(desc.up_axis, look_dir));

  out_cam.position = desc.position;
  out_cam.up       = vec3_normalize(vec3_cross(look_dir, right_axis));
  
  out_cam.direction.x = freya::cos(out_cam.yaw    * DEG2RAD)  * freya::cos(out_cam.pitch * DEG2RAD);
  out_cam.direction.y = freya::sin((out_cam.pitch * DEG2RAD));
  out_cam.direction.z = freya::sin((out_cam.yaw   * DEG2RAD)) * freya::cos(out_cam.pitch * DEG2RAD);
  out_cam.front       = vec3_normalize(out_cam.direction);

  out_cam.view            = Mat4(1.0f);
  out_cam.projection      = Mat4(1.0f);
  out_cam.view_projection = Mat4(1.0f);
}

void camera_update(Camera& cam, const Vec3* target) {
  // View-projection matrix calculation

  Vec3 front = cam.position + cam.front;
  if(target) {
    front = *target + -cam.front;
  }

  cam.view            = mat4_look_at(cam.position, front, cam.up);
  cam.projection      = mat4_perspective((cam.zoom * DEG2RAD), cam.aspect_ratio, cam.near_plane, cam.far_plane);
  cam.view_projection = (cam.projection * cam.view);

  // Direction calculation

  cam.direction.x = freya::cos(cam.yaw   * DEG2RAD) * freya::cos(cam.pitch * DEG2RAD);
  cam.direction.y = freya::sin(cam.pitch * DEG2RAD);
  cam.direction.z = freya::sin(cam.yaw   * DEG2RAD) * freya::cos(cam.pitch * DEG2RAD);
  cam.front       = vec3_normalize(cam.direction);

  // Re-calculating frustrum corners
  calculate_frustum_corners(cam);
}

void camera_follow(Camera& cam, const Vec3& target, const Vec3& offset) {
  cam.position = target + offset;
}

void camera_follow_lerp(Camera& cam, const Vec3& target, const Vec3& offset, const f32 delta) {
  cam.position = vec3_lerp(cam.position, target + offset, delta);
}

/// Camera functions
/// ----------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
