#include "freya_math.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

/// ----------------------------------------------------------------------
/// Transform functions

Mat4 transform_apply(Transform& trans) {
  trans._is_dirty = false;
  return mat4_translate(trans._position) * quat_to_mat4(trans._rotation) * mat4_scale(trans._scale);
}

void transform_translate(Transform& trans, const Vec3& pos) {
  trans._position = pos; 
  trans._is_dirty = true;
}

void transform_rotate(Transform& trans, const Quat& rot) {
  trans._rotation = quat_normalize(rot); 
  trans._is_dirty = true;
}

void transform_rotate(Transform& trans, const Vec4& axis_angle) {
  transform_rotate(trans, quat_angle_axis(Vec3(axis_angle), axis_angle.w));
}

void transform_rotate(Transform& trans, const Vec3& axis, const f32 angle) {
  transform_rotate(trans, quat_angle_axis(axis, angle));
}

void transform_scale(Transform& trans, const Vec3& scale) {
  trans._scale    = scale; 
  trans._is_dirty = true;
}

void transform_lerp(Transform& trans_a, const Transform& trans_b, const f32 delta) {
  trans_a._position = vec3_lerp(trans_a._position, trans_b._position, delta);
  trans_a._rotation = quat_slerp(trans_a._rotation, trans_b._rotation, delta);
  trans_a._scale    = vec3_lerp(trans_a._scale, trans_b._scale, delta);

  trans_a._is_dirty = true;
}

void transform_lerp(Transform& trans, 
                    const Vec3& position, 
                    const Quat& rotation, 
                    const Vec3& scale,
                    const f32 delta) {
  trans._position = vec3_lerp(trans._position, position, delta);
  trans._rotation = quat_normalize(quat_slerp(trans._rotation, rotation, delta));
  trans._scale    = vec3_lerp(trans._scale, scale, delta);

  trans._is_dirty = true;
}

void transform_lerp_position(Transform& trans, const Vec3& position, const f32 delta) {
  transform_translate(trans, vec3_lerp(trans._position, position, delta));
}

void transform_slerp_rotation(Transform& trans, const Quat& rotation, const f32 delta) {
  transform_rotate(trans, quat_slerp(trans._rotation, rotation, delta));
}

void transform_lerp_rotation(Transform& trans, const Vec3& axis, const f32 angle, const f32 delta) {
  transform_slerp_rotation(trans, quat_angle_axis(axis, angle), delta);
}

void transform_lerp_scale(Transform& trans, const Vec3& scale, const f32 delta) {
  transform_scale(trans, vec3_lerp(trans._scale, scale, delta));
}

/// Transform functions
/// ----------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
