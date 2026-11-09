#include "freya_math.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

///---------------------------------------------------------------------------------------------------------------------
/// BoundingBox functions

BoundingBox bounding_box_calculate(const DynamicArray<Vertex3D>& vertices) {
  BoundingBox bounds;
  bounds.min = vertices[0].position;
  bounds.max = vertices[0].position;

  for(sizei i = 1; i < vertices.size(); i++) {
    bounds.min = vec3_min(bounds.min, vertices[i].position);
    bounds.max = vec3_max(bounds.max, vertices[i].position);
  }

  return bounds;
}

/// BoundingBox functions
///---------------------------------------------------------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
