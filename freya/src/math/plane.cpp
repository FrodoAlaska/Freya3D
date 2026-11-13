#include "freya_math.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

///---------------------------------------------------------------------------------------------------------------------
/// Plane functions

void plane_create(Plane& out_plane, const Vec3& normal, const Vec3& point) {
  out_plane.normal = normal; 
  out_plane.d      = -vec3_dot(normal, point);
}

f32 plane_get_distance(const Plane& plane, const Vec3& points) {
  return vec3_dot(plane.normal, points) + plane.d;
}

/// Plane functions
///---------------------------------------------------------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
