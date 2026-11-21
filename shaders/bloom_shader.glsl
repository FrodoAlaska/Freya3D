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

@block bloom_data 

// Uniforms

layout(binding = 0) uniform BloomData {
  vec2 u_inv_size;

  float u_first;
  float u_threshold;
  float u_knee;
  float u_radius;
};

@end

/// Blocks
/// --------------------------------------------------

/// --------------------------------------------------
/// Basic vertex shader

@vs vs

// Outputs
out vec2 o_tex_coords;

// Main

void main() {
  // Generate a UV from the vertex 
  vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2); 

  // Done!

  o_tex_coords = uv;
  gl_Position  = vec4(vec2(uv * 2.0 - 1.0), 0.0, 1.0);
}

@end

/// Basic vertex shader
/// --------------------------------------------------

/// --------------------------------------------------
/// Downscale shader

@fs downscale_fs

// Includes 
@include_block bloom_data

// Inputs
in vec2 o_tex_coords;

// Textures 

layout(binding = 0) uniform texture2D u_texture; 
layout(binding = 0) uniform sampler u_sampler; 

// Outputs 
out vec4 frag_color;

// Functions 

float luma(vec3 color) {
  return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

vec3 sanitize(vec3 color) {
  if(any(isnan(color)) || any(isinf(color))) {
    return vec3(0.0);
  }

  return min(color, vec3(65000.0));
}

vec3 tap(vec2 offset) {
  vec3 color = texture(sampler2D(u_texture, u_sampler), o_tex_coords + u_inv_size * offset).rgb;
  return (u_first > 0.5) ? sanitize(color) : color;
}

vec3 prefilter(vec3 color) {
  if(u_threshold <= 0.0) {
    return color;
  }

  float brightness = max(color.r, max(color.g, color.b));
  float knee       = u_threshold * u_knee;

  float soft = clamp(brightness - u_threshold + knee, 0.0, knee * 2.0);
  soft       = (soft * soft) / (knee * 4.0 + 1e-5);

  float contribution = max(soft, brightness - u_threshold) / max(brightness, 1e-5);
  return color * contribution;
}

// Main 

void main() {
  // ...

  vec3 a = tap(vec2(-2.0,  2.0));
  vec3 b = tap(vec2( 0.0,  2.0)); 
  vec3 c = tap(vec2( 2.0,  2.0));

  vec3 j = tap(vec2(-1.0,  1.0)); 
  vec3 k = tap(vec2( 1.0,  1.0));

  vec3 d = tap(vec2(-2.0,  0.0));
  vec3 e = tap(vec2( 0.0,  0.0));
  vec3 f = tap(vec2( 2.0,  0.0));

  vec3 l = tap(vec2(-1.0, -1.0)); 
  vec3 m = tap(vec2( 1.0, -1.0));

  vec3 g = tap(vec2(-2.0, -2.0));
  vec3 h = tap(vec2( 0.0, -2.0));
  vec3 i = tap(vec2( 2.0, -2.0));

  // ... 
 
  vec3 g0 = (j + k + l + m) * 0.25;
  vec3 g1 = (a + b + d + e) * 0.25;
  vec3 g2 = (b + c + e + f) * 0.25;
  vec3 g3 = (d + e + g + h) * 0.25;
  vec3 g4 = (e + f + h + i) * 0.25;

  // Applying karis average if possible 

  vec3 result; 
  if(u_first > 0.5) {
    // Karis average: bright groups count for less, so single bright pixels can't dominate
    
    float w0 = 0.5   / (1.0 + luma(g0));
    float w1 = 0.125 / (1.0 + luma(g1));
    float w2 = 0.125 / (1.0 + luma(g2));
    float w3 = 0.125 / (1.0 + luma(g3));
    float w4 = 0.125 / (1.0 + luma(g4));
    
    result = (g0 * w0 + g1 * w1 + g2 * w2 + g3 * w3 + g4 * w4) / (w0 + w1 + w2 + w3 + w4);
    result = prefilter(result);
  }
  else {
    result = g0 * 0.5 + (g1 + g2 + g3 + g4) * 0.125;
  }

  // Done!
  frag_color = vec4(result, 1.0);
}
@end

/// Downscale shader
/// --------------------------------------------------

/// --------------------------------------------------
/// Upcale shader

@fs upscale_fs

// Includes 
@include_block bloom_data

// Inputs
in vec2 o_tex_coords;

// Textures 

layout(binding = 0) uniform texture2D u_texture; 
layout(binding = 0) uniform sampler u_sampler; 

// Outputs 
out vec4 frag_color;

// Functions

vec3 tap(vec2 offset) {
  return texture(sampler2D(u_texture, u_sampler), o_tex_coords + u_inv_size * u_radius * offset).rgb;
}

// Main 

void main() { 
  vec3 result = tap(vec2(-1.0,  1.0)) + tap(vec2(0.0,  1.0)) * 2.0 + tap(vec2(1.0,  1.0));

  result += tap(vec2(-1.0,  0.0)) * 2.0 + tap(vec2(0.0,  0.0)) * 4.0 + tap(vec2(1.0,  0.0)) * 2.0;
  result += tap(vec2(-1.0, -1.0)) + tap(vec2(0.0, -1.0)) * 2.0 + tap(vec2(1.0, -1.0));

  frag_color = vec4(result / 16.0, 1.0);
}
@end


/// Upcale shader
/// --------------------------------------------------

/// --------------------------------------------------
/// Programs

@program downscale vs downscale_fs
@program upscale   vs upscale_fs

/// Programs
/// --------------------------------------------------
