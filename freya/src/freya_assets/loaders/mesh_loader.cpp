#include "asset_loaders.h"

#include "sokol/sokol_shape.h"

/// ----------------------------------------------------------------------
/// Private functions

static void generate_normals(freya::DynamicArray<freya::Vertex3D>& vertices, 
                             const freya::DynamicArray<freya::u16>& indices) {
  for(freya::sizei i = 0; i < indices.size(); i += 3) {
    freya::u32 idx0 = indices[i + 0];
    freya::u32 idx1 = indices[i + 1];
    freya::u32 idx2 = indices[i + 2];
    
    freya::Vec3 edge1 = vertices[idx1].position - vertices[idx0].position;
    freya::Vec3 edge2 = vertices[idx2].position - vertices[idx0].position;

    freya::Vec3 normal = freya::vec3_normalize(freya::vec3_cross(edge2, edge1));

    vertices[idx0].normal = normal;
    vertices[idx1].normal = normal;
    vertices[idx2].normal = normal;
  }
}

static void generate_tangents(freya::DynamicArray<freya::Vertex3D>& vertices, 
                              const freya::DynamicArray<freya::u16>& indices) {
  // Credits to Travis Vroman for this code. 
  // https://github.com/travisvroman/kohi/blob/main/kohi.core/src/math/geometry.c

  // In order to retrieve the tangents for each face we need: 
  //    1 - Calculate the differences between the points to get the edges. 
  //    2 - Calculate the differences between the texture coordinates which we call the "delta".
  //    3 - Using the edges and deltas, calculate a "d" value which will be used to calculate the tangents.
  //    4 - Use the "d" value, edges, and the "V" coordinate to calculate the tangents. 
  //    5 (Optional) - Use the "d" value, edges, and "U" coordinate to calculate the bi-tangents. However, we calculate the bi-tangents in the shader.

  for(freya::sizei i = 0; i < indices.size(); i += 3) {
    freya::u32 idx0 = indices[i + 0];
    freya::u32 idx1 = indices[i + 1];
    freya::u32 idx2 = indices[i + 2];

    // Retrieve the edges
    
    freya::Vec3 edge1 = vertices[idx1].position - vertices[idx0].position;
    freya::Vec3 edge2 = vertices[idx2].position - vertices[idx0].position;

    // Retrieve the deltas 
    
    freya::f32 deltaU1 = vertices[idx1].texture_coords.x - vertices[idx0].texture_coords.x; 
    freya::f32 deltaV1 = vertices[idx1].texture_coords.y - vertices[idx0].texture_coords.y; 
  
    freya::f32 deltaU2 = vertices[idx2].texture_coords.x - vertices[idx0].texture_coords.x; 
    freya::f32 deltaV2 = vertices[idx2].texture_coords.y - vertices[idx0].texture_coords.y; 

    // Calculate the tangents
    
    freya::f32 dividend = 1.0f / (deltaU1 * deltaV2 - deltaU2 * deltaV1);
  
    freya::Vec3 tan; 
    tan.x = dividend * (deltaV2 * edge1.x - deltaV1 * edge2.x);
    tan.y = dividend * (deltaV2 * edge1.y - deltaV1 * edge2.y);
    tan.z = dividend * (deltaV2 * edge1.z - deltaV1 * edge2.z);

    tan = freya::vec3_normalize(tan);

    vertices[idx0].tangent = tan;
    vertices[idx1].tangent = tan;
    vertices[idx2].tangent = tan;
  }
}

void gen_cube_mesh(freya::DynamicArray<freya::Vertex3D>& out_vertices, 
                   freya::DynamicArray<freya::u16>& out_indices) {
  // Build the shape

  sshape_sizes_t shape_info = sshape_box_sizes(1);

  freya::DynamicArray<sshape_vertex_t> sk_vertices(shape_info.vertices.num);
  out_vertices.resize(shape_info.vertices.num);
  out_indices.resize(shape_info.indices.num);

  sshape_buffer_t buffer = {};

  buffer.vertices.buffer = sshape_range{sk_vertices.data(), sk_vertices.size() * sizeof(sshape_vertex_t)};
  buffer.indices.buffer  = sshape_range{out_indices.data(), out_indices.size() * sizeof(freya::u16)};

  sshape_box_t box_params = {};
  buffer = sshape_build_box(&buffer, &box_params);

  // Fill in the vertices array 

  for(freya::sizei i = 0; i < sk_vertices.size(); i++) {
    freya::Vertex3D& vertex   = out_vertices[i];
    sshape_vertex_t sk_vertex = sk_vertices[i]; 
    
    vertex.position       = freya::Vec3(sk_vertex.x, sk_vertex.y, sk_vertex.z);
    vertex.texture_coords = freya::Vec2((freya::f32)(sk_vertex.u / 65535.0f), (freya::f32)(sk_vertex.v / 65535.0f));
   
    // sokol_shape stores the normal as a `u32`. We have to 
    // extract each byte individually and turn it into a `f32`. 

    freya::i8 iw = (freya::i8)((sk_vertex.normal >> 24));
    freya::i8 iz = (freya::i8)((sk_vertex.normal >> 16));
    freya::i8 iy = (freya::i8)((sk_vertex.normal >> 8));
    freya::i8 ix = (freya::i8)((sk_vertex.normal & 0xFF));

    freya::f32 fx = (freya::f32)(ix / 127.0f);
    freya::f32 fy = (freya::f32)(iy / 127.0f);
    freya::f32 fz = (freya::f32)(iz / 127.0f);

    vertex.normal = freya::Vec3(fx, fy, fz);
  }

  // Generate the missing tangents and normals
  generate_tangents(out_vertices, out_indices);
}

void gen_plane_mesh(freya::DynamicArray<freya::Vertex3D>& out_vertices, 
                    freya::DynamicArray<freya::u16>& out_indices) {
  // Build the shape

  freya::u16 tiles_count    = 12;
  sshape_sizes_t shape_info = sshape_plane_sizes(tiles_count);

  freya::DynamicArray<sshape_vertex_t> sk_vertices(shape_info.vertices.num);
  out_vertices.resize(shape_info.vertices.num);
  out_indices.resize(shape_info.indices.num);

  sshape_buffer_t buffer = {};

  buffer.vertices.buffer = sshape_range{sk_vertices.data(), sk_vertices.size() * sizeof(sshape_vertex_t)};
  buffer.indices.buffer  = sshape_range{out_indices.data(), out_indices.size() * sizeof(freya::u16)};

  sshape_plane_t params = {
    .tiles = tiles_count,
  };
  buffer = sshape_build_plane(&buffer, &params);

  // Fill in the vertices array 

  for(freya::sizei i = 0; i < sk_vertices.size(); i++) {
    freya::Vertex3D& vertex   = out_vertices[i];
    sshape_vertex_t sk_vertex = sk_vertices[i]; 
    
    vertex.position       = freya::Vec3(sk_vertex.x, sk_vertex.y, sk_vertex.z);
    vertex.texture_coords = freya::Vec2((freya::f32)(sk_vertex.u / 65535.0f), (freya::f32)(sk_vertex.v / 65535.0f));
   
    // sokol_shape stores the normal as a `u32`. We have to 
    // extract each byte individually and turn it into a `f32`. 

    freya::i8 iw = (freya::i8)((sk_vertex.normal >> 24));
    freya::i8 iz = (freya::i8)((sk_vertex.normal >> 16));
    freya::i8 iy = (freya::i8)((sk_vertex.normal >> 8));
    freya::i8 ix = (freya::i8)((sk_vertex.normal & 0xFF));

    freya::f32 fx = (freya::f32)(ix / 127.0f);
    freya::f32 fy = (freya::f32)(iy / 127.0f);
    freya::f32 fz = (freya::f32)(iz / 127.0f);

    vertex.normal = freya::Vec3(fx, fy, fz);
  }

  // Generate the missing tangents and normals
  generate_tangents(out_vertices, out_indices);
}

void gen_sphere_mesh(freya::DynamicArray<freya::Vertex3D>& out_vertices, 
                     freya::DynamicArray<freya::u16>& out_indices) {
  // Build the shape

  freya::u16 slices_count = 32;
  freya::u16 stacks_count = 24;

  sshape_sizes_t shape_info = sshape_sphere_sizes(slices_count, stacks_count);

  freya::DynamicArray<sshape_vertex_t> sk_vertices(shape_info.vertices.num);
  out_vertices.resize(shape_info.vertices.num);
  out_indices.resize(shape_info.indices.num);

  sshape_buffer_t buffer = {};

  buffer.vertices.buffer = sshape_range{sk_vertices.data(), sk_vertices.size() * sizeof(sshape_vertex_t)};
  buffer.indices.buffer  = sshape_range{out_indices.data(), out_indices.size() * sizeof(freya::u16)};

  sshape_sphere_t params = {
    .slices = slices_count, 
    .stacks = stacks_count,
  };
  buffer = sshape_build_sphere(&buffer, &params);

  // Fill in the vertices array 

  for(freya::sizei i = 0; i < sk_vertices.size(); i++) {
    freya::Vertex3D& vertex   = out_vertices[i];
    sshape_vertex_t sk_vertex = sk_vertices[i]; 
    
    vertex.position       = freya::Vec3(sk_vertex.x, sk_vertex.y, sk_vertex.z);
    vertex.texture_coords = freya::Vec2((freya::f32)(sk_vertex.u / 65535.0f), (freya::f32)(sk_vertex.v / 65535.0f));
   
    // sokol_shape stores the normal as a `u32`. We have to 
    // extract each byte individually and turn it into a `f32`. 

    freya::i8 iw = (freya::i8)((sk_vertex.normal >> 24));
    freya::i8 iz = (freya::i8)((sk_vertex.normal >> 16));
    freya::i8 iy = (freya::i8)((sk_vertex.normal >> 8));
    freya::i8 ix = (freya::i8)((sk_vertex.normal & 0xFF));

    freya::f32 fx = (freya::f32)(ix / 127.0f);
    freya::f32 fy = (freya::f32)(iy / 127.0f);
    freya::f32 fz = (freya::f32)(iz / 127.0f);

    vertex.normal = freya::Vec3(fx, fy, fz);
  }

  // Generate the missing tangents and normals
  generate_tangents(out_vertices, out_indices);
}

void gen_cylinder_mesh(freya::DynamicArray<freya::Vertex3D>& out_vertices, 
                       freya::DynamicArray<freya::u16>& out_indices) {
  // Build the shape

  freya::u16 slices_count = 24;
  freya::u16 stacks_count = 18;

  sshape_sizes_t shape_info = sshape_cylinder_sizes(slices_count, stacks_count);

  freya::DynamicArray<sshape_vertex_t> sk_vertices(shape_info.vertices.num);
  out_vertices.resize(shape_info.vertices.num);
  out_indices.resize(shape_info.indices.num);

  sshape_buffer_t buffer = {};

  buffer.vertices.buffer = sshape_range{sk_vertices.data(), sk_vertices.size() * sizeof(sshape_vertex_t)};
  buffer.indices.buffer  = sshape_range{out_indices.data(), out_indices.size() * sizeof(freya::u16)};

  sshape_cylinder_t params = {
    .slices = slices_count, 
    .stacks = stacks_count,
  };
  buffer = sshape_build_cylinder(&buffer, &params);

  // Fill in the vertices array 

  for(freya::sizei i = 0; i < sk_vertices.size(); i++) {
    freya::Vertex3D& vertex   = out_vertices[i];
    sshape_vertex_t sk_vertex = sk_vertices[i]; 
    
    vertex.position       = freya::Vec3(sk_vertex.x, sk_vertex.y, sk_vertex.z);
    vertex.texture_coords = freya::Vec2((freya::f32)(sk_vertex.u / 65535.0f), (freya::f32)(sk_vertex.v / 65535.0f));
   
    // sokol_shape stores the normal as a `u32`. We have to 
    // extract each byte individually and turn it into a `f32`. 

    freya::i8 iw = (freya::i8)((sk_vertex.normal >> 24));
    freya::i8 iz = (freya::i8)((sk_vertex.normal >> 16));
    freya::i8 iy = (freya::i8)((sk_vertex.normal >> 8));
    freya::i8 ix = (freya::i8)((sk_vertex.normal & 0xFF));

    freya::f32 fx = (freya::f32)(ix / 127.0f);
    freya::f32 fy = (freya::f32)(iy / 127.0f);
    freya::f32 fz = (freya::f32)(iz / 127.0f);

    vertex.normal = freya::Vec3(fx, fy, fz);
  }

  // Generate the missing tangents and normals
  generate_tangents(out_vertices, out_indices);
}

void gen_torus_mesh(freya::DynamicArray<freya::Vertex3D>& out_vertices, 
                    freya::DynamicArray<freya::u16>& out_indices) {
  // Build the shape

  freya::u16 sides_count = 24;
  freya::u16 rings_count = 18;

  sshape_sizes_t shape_info = sshape_torus_sizes(sides_count, rings_count);

  freya::DynamicArray<sshape_vertex_t> sk_vertices(shape_info.vertices.num);
  out_vertices.resize(shape_info.vertices.num);
  out_indices.resize(shape_info.indices.num);

  sshape_buffer_t buffer = {};

  buffer.vertices.buffer = sshape_range{sk_vertices.data(), sk_vertices.size() * sizeof(sshape_vertex_t)};
  buffer.indices.buffer  = sshape_range{out_indices.data(), out_indices.size() * sizeof(freya::u16)};

  sshape_torus_t params = {
    .sides = sides_count, 
    .rings = rings_count,
  };
  buffer = sshape_build_torus(&buffer, &params);

  // Fill in the vertices array 

  for(freya::sizei i = 0; i < sk_vertices.size(); i++) {
    freya::Vertex3D& vertex   = out_vertices[i];
    sshape_vertex_t sk_vertex = sk_vertices[i]; 
    
    vertex.position       = freya::Vec3(sk_vertex.x, sk_vertex.y, sk_vertex.z);
    vertex.texture_coords = freya::Vec2((freya::f32)(sk_vertex.u / 65535.0f), (freya::f32)(sk_vertex.v / 65535.0f));
   
    // sokol_shape stores the normal as a `u32`. We have to 
    // extract each byte individually and turn it into a `f32`. 

    freya::i8 iw = (freya::i8)((sk_vertex.normal >> 24));
    freya::i8 iz = (freya::i8)((sk_vertex.normal >> 16));
    freya::i8 iy = (freya::i8)((sk_vertex.normal >> 8));
    freya::i8 ix = (freya::i8)((sk_vertex.normal & 0xFF));

    freya::f32 fx = (freya::f32)(ix / 127.0f);
    freya::f32 fy = (freya::f32)(iy / 127.0f);
    freya::f32 fz = (freya::f32)(iz / 127.0f);

    vertex.normal = freya::Vec3(fx, fy, fz);
  }

  // Generate the missing tangents and normals
  generate_tangents(out_vertices, out_indices);
}

/// Private functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// Mesh loader functions

void mesh_loader_load(const freya::MeshType type, 
                      freya::DynamicArray<freya::Vertex3D>& out_vertices, 
                      freya::DynamicArray<freya::u16>& out_indices) {
  switch(type) {
    case freya::MESH_CUBE:
      gen_cube_mesh(out_vertices, out_indices);
      break;
    case freya::MESH_PLANE:
      gen_plane_mesh(out_vertices, out_indices);
      break;
    case freya::MESH_SPHERE:
      gen_sphere_mesh(out_vertices, out_indices);
      break;
    case freya::MESH_CYLINDER:
      gen_cylinder_mesh(out_vertices, out_indices);
      break;
    case freya::MESH_TORUS:
      gen_torus_mesh(out_vertices, out_indices);
      break;
    default:
      break;
  }
}

/// Mesh loader functions
/// ----------------------------------------------------------------------
