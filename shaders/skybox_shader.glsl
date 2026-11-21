@module sk
@vs vs

// Inputs
in vec3 a_pos;

// Outputs
out vec3 o_tex_coords;

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

void main() {
  o_tex_coords = a_pos;

  mat4 model  = u_projection * mat4(mat3(u_view));
  gl_Position = vec4(model * vec4(a_pos, 1.0)).xyww;
}
@end

@fs fs

// Inputs
in vec3 o_tex_coords;

// Uniforms 

layout(binding = 0) uniform textureCube u_texture; 
layout(binding = 0) uniform sampler u_sampler; 

// Outputs 

out vec4 frag_color;

void main() {
  frag_color = texture(samplerCube(u_texture, u_sampler), o_tex_coords);
}
@end

@program skybox vs fs
