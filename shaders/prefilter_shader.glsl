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

@module pf
@vs vs

// Inputs

in vec2 a_pos;
in vec2 a_tex_coords;

// Outputs
out vec2 o_tex_coords;

// Main

void main() {
  o_tex_coords = a_tex_coords;
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

// Textures 

layout(binding = 0) uniform texture2D u_texture; 
layout(binding = 0) uniform sampler u_sampler; 

// Outputs 
out vec4 frag_color;

// Main 

void main() {
  // 
  // @NOTE: 
  // 
  // Borrowed this implementation from: 
  // https://github.com/Razakhel/RaZ/blob/master/src/RaZ/Render/BloomRenderProcess.cpp
  //

  // Thresholding pixels according to their luminance

  vec3 color      = texture(sampler2D(u_texture, u_sampler), o_tex_coords).rgb;
  float threshold = 0.75; // @TODO (Make it user-controlled)

  float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722)); 
  frag_color       = vec4(color * float(brightness >= threshold), 1.0);
}
@end

/// Fragment shader
/// --------------------------------------------------

@program prefilter vs fs
