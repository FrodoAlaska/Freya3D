@module bb

/// --------------------------------------------------
/// Types 

@ctype mat4 freya::Mat4
@ctype vec2 freya::Vec2
@ctype vec3 freya::Vec3
@ctype vec4 freya::Vec4

/// Types 
/// --------------------------------------------------

/// --------------------------------------------------
/// Vertex shader

@vs billboard_vs

// Inputs

in vec3 a_pos;
in vec3 a_normal; 
in vec2 a_tex_coords;

// Instance data 

struct InstanceData {
  mat4 model;
};

// Uniforms 

layout(binding = 0) uniform FrameData {
  mat4 u_view;
  mat4 u_projection;
  mat4 u_light_space;
  
  vec3 u_camera_pos;
  vec3 u_ambient_color;

  float u_gamma; 
  float u_exposure;
};

layout(binding = 1) uniform ModelIndex {
  int u_index;
};

layout(binding = 0) readonly buffer InstanceBuffer {
  InstanceData u_data[];
};

// Output 

out vec2 o_tex_coords;

// Main

void main() {
  // Calculate the world position of the billboard

  mat4 model  = u_data[u_index + gl_InstanceIndex].model;
  vec3 center = vec3(model * vec4(a_pos, 1.0));

  // Calculate the up and right vector that the billboard 
  // will be oriented around.

  vec3 up = vec3(0, 1, 0);

  vec3 direction = u_camera_pos - center; 
  direction.y    = 0.0; // Cancel any rotation on the Y-axis...

  direction  = normalize(direction);
  vec3 right = normalize(cross(up, direction));

  // Calculate the final position
  vec3 pos = (right * a_pos.x) + (up * a_pos.y);

  // Done!

  o_tex_coords = a_tex_coords;
  gl_Position  = u_projection * u_view * model * vec4(pos, 1.0);
}
@end

/// Vertex shader
/// --------------------------------------------------

/// --------------------------------------------------
/// Fragment shaders

@fs billboard_fs

// Inputs 

in vec2 o_tex_coords;

// Output
out vec4 frag_color;

// Samplers

layout(binding = 1) uniform texture2D u_texture; 
layout(binding = 0) uniform sampler u_sampler; 

// Main 

void main() {
  frag_color = texture(sampler2D(u_texture, u_sampler), o_tex_coords);
}

@end

/// Fragment shaders
/// --------------------------------------------------

/// --------------------------------------------------
/// Programs

@program billboard_pass billboard_vs billboard_fs 

/// Programs
/// --------------------------------------------------
