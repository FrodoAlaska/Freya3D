/// --------------------------------------------------
/// Types 

@ctype mat4 freya::Mat4
@ctype vec2 freya::Vec2
@ctype vec3 freya::Vec3
@ctype vec4 freya::Vec4

/// Types 
/// --------------------------------------------------

/// --------------------------------------------------
/// Blocks

@module screen
@vs vs

// Inputs

in vec2 a_pos;
in vec2 a_tex_coords;

// Outputs

out vec2 o_tex_coords;

out float o_gamma; 
out float o_exposure;

layout(binding = 0) uniform FrameData {
  mat4 u_view;
  mat4 u_projection;
  mat4 u_light_space;
  
  vec3 u_camera_pos;
  vec3 u_ambient_color;

  float u_gamma;
  float u_exposure;
};

/// Blocks
/// --------------------------------------------------

/// --------------------------------------------------
/// Vertex shader

void main() {
  o_tex_coords = a_tex_coords;
  
  o_gamma    = u_gamma; 
  o_exposure = u_exposure;

  gl_Position  = vec4(a_pos, 0.0, 1.0);
}
@end

/// Vertex shader
/// --------------------------------------------------

/// --------------------------------------------------
/// Fragment shader

@fs fs

// Inputs

in vec2 o_tex_coords;

in float o_gamma; 
in float o_exposure;

// Uniforms 

layout(binding = 0) uniform texture2D u_bloom_texture; 
layout(binding = 0) uniform sampler u_bloom_sampler; 

layout(binding = 1) uniform texture2D u_scene_texture; 
layout(binding = 1) uniform sampler u_scene_sampler; 

// Outputs 

out vec4 frag_color;

// Functions 

vec3 tonemap_aces(vec3 color) {
  // Constants that were used in the original algorithm
  // Source: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/

  const float A = 2.51; 
  const float B = 0.03; 
  const float C = 2.43; 
  const float D = 0.59; 
  const float E = 0.14;

  // Tonemapped!
  return clamp((color * (A * color + B)) / (color * (C * color + D) + E), 0.0, 1.0);
}

// Main 

void main() {
  // Sample the main textures

  vec3 bloom_texel = texture(sampler2D(u_bloom_texture, u_bloom_sampler), o_tex_coords).rgb;
  vec3 scene_texel = texture(sampler2D(u_scene_texture, u_scene_sampler), o_tex_coords).rgb * exp2(o_exposure);

  vec3 result = mix(scene_texel, bloom_texel, 0.05); // Mix the scene and the bloom textures
  
  // Apply some tone-mapping
  vec3 toned = tonemap_aces(result);

  // Apply some gamma correction and output the final color

  vec3 final_color = pow(toned, vec3(1.0 / o_gamma));
  frag_color       = vec4(final_color, 1.0);
}
@end

/// Fragment shader
/// --------------------------------------------------

@program space vs fs
