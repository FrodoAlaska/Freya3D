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

@vs vs

// Inputs

in vec2 a_pos;
in vec2 a_tex_coords;

// Outputs

out vec2 o_tex_coords;
out flat int o_is_horizontal;

// Uniforms

layout(binding = 0) uniform BloomData {
  int u_is_horizontal; 
};

// Main

void main() {
  o_tex_coords    = a_tex_coords;
  o_is_horizontal = u_is_horizontal; 

  gl_Position = vec4(a_pos, 0.0, 1.0);
}
@end

/// Vertex shader
/// --------------------------------------------------

/// --------------------------------------------------
/// Fragment shader

@fs fs

// Inputs

in vec2 o_tex_coords;
in flat int o_is_horizontal;

// Textures 

layout(binding = 0) uniform texture2D u_texture; 
layout(binding = 0) uniform sampler u_sampler; 

// Outputs 
out vec4 frag_color;

// Private variables 
float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

// Main 

void main() {
  // Texel size 
  vec2 offset = 1.0 / textureSize(sampler2D(u_texture, u_sampler), 0);

  // Current fragment's contribution 
  
  vec3 result = texture(sampler2D(u_texture, u_sampler), o_tex_coords).rgb * weights[0];
  if(o_is_horizontal == 1) {
    // Horizontal blur

    for(int i = 0; i < 5; i++) {
      result += texture(sampler2D(u_texture, u_sampler), o_tex_coords + vec2(offset.x * i, 0.0)).rgb * weights[i];
      result += texture(sampler2D(u_texture, u_sampler), o_tex_coords - vec2(offset.x * i, 0.0)).rgb * weights[i];
    }
  }
  else {
    // Vertical blur 

    for(int i = 0; i < 5; i++) {
      result += texture(sampler2D(u_texture, u_sampler), o_tex_coords + vec2(0.0, offset.y * i)).rgb * weights[i];
      result += texture(sampler2D(u_texture, u_sampler), o_tex_coords - vec2(0.0, offset.y * i)).rgb * weights[i];
    }
  }

  // Done! 
  frag_color = vec4(result, 1.0);
}
@end

/// Fragment shader
/// --------------------------------------------------

@program bloom vs fs
