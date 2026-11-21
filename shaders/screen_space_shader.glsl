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

@module screen
@vs vs

// Outputs

out vec2 o_tex_coords;

out float o_gamma; 
out float o_exposure;
out float o_bloom_intensity;

layout(binding = 0) uniform FrameData {
  mat4 u_view;
  mat4 u_projection;
  mat4 u_light_space;
  
  vec3 u_camera_pos;
  vec3 u_ambient_color;

  float u_gamma;
  float u_exposure;
  float u_bloom_intensity;
};

// Main

void main() {
  // Generate a UV from the vertex 
  vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2); 
  
  // Carry the outputs over
  
  o_tex_coords = uv;

  o_gamma           = u_gamma;
  o_exposure        = u_exposure;
  o_bloom_intensity = u_bloom_intensity;

  // Done!
  gl_Position = vec4(vec2(uv * 2.0 - 1.0), 0.0, 1.0);
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
in float o_bloom_intensity;

// Textures 

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
  vec3 scene_texel = texture(sampler2D(u_scene_texture, u_scene_sampler), o_tex_coords).rgb;

  vec3 result = mix(scene_texel, bloom_texel, o_bloom_intensity) * exp2(o_exposure); // Mix the scene and the bloom textures
  
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
