#include "freya_math.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

/// ----------------------------------------------------------------------
/// Transform functions

Mat4 transform_apply(Transform& trans) {
  return mat4_translate(trans.position) *
         quat_to_mat4(trans.rotation)   *
         mat4_scale(trans.scale);
}

Mat4 transform_translate(Transform& trans, const Vec3& pos) {
  trans.position = pos; 
  return transform_apply(trans);
}

Mat4 transform_rotate(Transform& trans, const Quat& rot) {
  trans.rotation = quat_normalize(rot); 
  return transform_apply(trans);
}

Mat4 transform_rotate(Transform& trans, const Vec4& axis_angle) {
  return transform_rotate(trans, quat_angle_axis(Vec3(axis_angle), axis_angle.w));
}

Mat4 transform_rotate(Transform& trans, const Vec3& axis, const f32 angle) {
  return transform_rotate(trans, quat_angle_axis(axis, angle));
}

Mat4 transform_scale(Transform& trans, const Vec3& scale) {
  trans.scale = scale; 
  return transform_apply(trans);
}

Mat4 transform_lerp(Transform& trans_a, const Transform& trans_b, const f32 delta) {
  trans_a.position = vec3_lerp(trans_a.position, trans_b.position, delta);
  trans_a.rotation = quat_slerp(trans_a.rotation, trans_b.rotation, delta);
  trans_a.scale    = vec3_lerp(trans_a.scale, trans_b.scale, delta);

  return transform_apply(trans_a);
}

Mat4 transform_lerp(Transform& trans, 
                    const Vec3& position, 
                    const Quat& rotation, 
                    const Vec3& scale,
                    const f32 delta) {
  trans.position = vec3_lerp(trans.position, position, delta);
  trans.rotation = quat_normalize(quat_slerp(trans.rotation, rotation, delta));
  trans.scale    = vec3_lerp(trans.scale, scale, delta);

  return transform_apply(trans);
}

Mat4 transform_lerp_position(Transform& trans, const Vec3& position, const f32 delta) {
  return transform_translate(trans, vec3_lerp(trans.position, position, delta));
}

Mat4 transform_slerp_rotation(Transform& trans, const Quat& rotation, const f32 delta) {
  return transform_rotate(trans, quat_slerp(trans.rotation, rotation, delta));
}

Mat4 transform_lerp_rotation(Transform& trans, const Vec3& axis, const f32 angle, const f32 delta) {
  return transform_slerp_rotation(trans, quat_angle_axis(axis, angle), delta);
}

Mat4 transform_lerp_scale(Transform& trans, const Vec3& scale, const f32 delta) {
  return transform_scale(trans, vec3_lerp(trans.scale, scale, delta));
}

/// Transform functions
/// ----------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
