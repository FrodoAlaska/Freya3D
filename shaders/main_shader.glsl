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

@block basic_vertex

// Inputs

in vec3 a_pos;
in vec3 a_normal; 
in vec3 a_tangent; 
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
@end

/// Blocks
/// --------------------------------------------------

/// --------------------------------------------------
/// Light shader

@vs light_vs
@glsl_options fixup_clipspace // important: map clipspace z from -1..+1 to 0..+1 on GL

// Includes
@include_block basic_vertex

// Output 

out vec2 o_tex_coords;
out vec3 o_normal, o_tangent;
out vec3 o_pixel_pos;
out vec4 o_shadow_pos;

out vec3 o_camera_pos;
out vec3 o_ambient_color;
out float o_gamma;

out mat3 o_TBN;

// Main

void main() {
  // Pre-calculating important variables

  mat4 model = u_data[u_index + gl_InstanceIndex].model;

  vec4 model_space = model * vec4(a_pos, 1.0);
  mat3 model_m3    = transpose(inverse(mat3(model)));

  // Calculating the TBN matrix
  
  o_tangent = normalize(mat3(model) * a_tangent);
  o_normal  = normalize(model_m3 * a_normal);

  // @NOTE: Using the Gram-Schmidt process to re-othogonalize the tangent vector to make 
  // sure all vectors are perpendicular to each other
  o_tangent = normalize(o_tangent - dot(o_tangent, o_normal) * o_normal); 

  vec3 bitangent = cross(o_normal, o_tangent);
  o_TBN          = mat3(o_tangent, bitangent, o_normal);
  
  // Setting the outputs 

  o_tex_coords = a_tex_coords;
  o_pixel_pos  = vec3(model_space);
  o_shadow_pos = u_light_space * model_space;

  o_camera_pos    = u_camera_pos;
  o_ambient_color = u_ambient_color;
  o_gamma         = u_gamma;

  // Done!
  gl_Position = u_projection * u_view * model_space;
}
@end

@fs light_fs

// Inputs 

in vec2 o_tex_coords;
in vec3 o_normal, o_tangent;
in vec3 o_pixel_pos;
in vec4 o_shadow_pos;

in vec3 o_camera_pos;
in vec3 o_ambient_color;
in float o_gamma;

in mat3 o_TBN;

// Output
out vec4 frag_color;

// Consts 

const int POINT_LIGHTS_MAX = 16;
const float PI             = 3.14159265359f;

// Structures 

struct BRDFDesc {
  vec3 f0;

  vec3 normal; 
  vec3 view_dir; 
  vec3 light_dir; 

  float roughness; 
  float metallic;

  vec3 albedo_texel;
  vec3 radiance;
};

struct BRDFResult {
  vec3 specular; 
  vec3 diffuse;

  vec3 radiance_color;
};

// Uniforms 

layout (binding = 2) uniform Light {
  vec3 direction;
  vec3 color;
} dir_light;

layout (binding = 3) uniform PointLight {
  vec4 position_r[POINT_LIGHTS_MAX];
  vec4 color_fo[POINT_LIGHTS_MAX];

  int count;
} p_lights;

layout (binding = 4) uniform Material {
  vec4 color; 

  float roughness;
  float metallic; 
  float emissive;
} material;

// Samplers

layout(binding = 1) uniform texture2D u_albedo_map; 
layout(binding = 0) uniform sampler u_albedo_sampler; 

layout(binding = 2) uniform texture2D u_roughness_map; 
layout(binding = 1) uniform sampler u_roughness_sampler; 

layout(binding = 3) uniform texture2D u_metallic_map; 
layout(binding = 2) uniform sampler u_metallic_sampler; 

layout(binding = 4) uniform texture2D u_normal_map; 
layout(binding = 3) uniform sampler u_normal_sampler; 

layout(binding = 5) uniform texture2D u_emissive_map; 
layout(binding = 4) uniform sampler u_emissive_sampler; 

layout(binding = 6) uniform texture2D u_shadow_map; 
layout(binding = 5) uniform sampler u_shadow_sampler; 

// Functions 

vec3 calculate_normal(const vec3 texel);

float brdf_normal_distribution(const vec3 normal, const vec3 halfway, const float roughness);
float brdf_geo_schlick_ggx(const float nx_dot, const float roughness);
float brdf_shadowing_masking(const vec3 normal, const vec3 view_dir, const vec3 light_dir, const float roughness);
vec3 brdf_fersnel_schlick_reflectance(const float cos_theta, const vec3 f0);
BRDFResult brdf_calculate(const BRDFDesc desc);

float attenuate(const float distance, const float radius, const float max_intensity, const float fall_off);

vec3 evaluate_directional_light(BRDFDesc desc);
vec3 evalute_point_light(const int index, BRDFDesc desc);

vec3 to_linear(vec3 value);

// Main

void main() {
  // Sample the textures 
  // 
  // @NOTE: We use the `pow` function with the albedo map since we assume that the albedo map is 
  // authored in sRGB space, and we want to convert it into linear space to calculate the values correctly.
  //

  vec3 f0_factor = vec3(0.04); // @TODO (PBR): Change this to be more dynamic and/or user controlled.

  vec3 albedo     = to_linear(texture(sampler2D(u_albedo_map, u_albedo_sampler), o_tex_coords).rgb);
  float roughness = texture(sampler2D(u_roughness_map, u_roughness_sampler), o_tex_coords).r;
  float metallic  = texture(sampler2D(u_metallic_map, u_metallic_sampler), o_tex_coords).r;
  vec3 normal     = texture(sampler2D(u_normal_map, u_normal_sampler), o_tex_coords).rgb; 
  vec3 emissive   = to_linear(texture(sampler2D(u_emissive_map, u_emissive_sampler), o_tex_coords).rgb * vec3(material.emissive));

  // Preparing the BRDF stage
  
  BRDFDesc desc;
  desc.normal       = calculate_normal(normal);
  desc.view_dir     = normalize(o_camera_pos - o_pixel_pos); 
  desc.roughness    = clamp(roughness * material.roughness, 0.04, 1.0);
  desc.metallic     = metallic  * material.metallic;
  desc.albedo_texel = albedo    * material.color.rgb;
  desc.f0           = mix(f0_factor, desc.albedo_texel, desc.metallic);

  // Accumulate all the lights 

  vec3 dir_light_factor = evaluate_directional_light(desc);

  vec3 point_lights_factor = vec3(0.0f);
  for(int i = 0; i < p_lights.count; i++) {
    point_lights_factor += evalute_point_light(i, desc);
  }

  //
  // Calculate the shadow factor
  // 

  // Calculate a good shadow bias 

  float n_dot_l = max(dot(normalize(o_normal), normalize(-dir_light.direction)), 0.0);
  float bias    = max(0.005 * (1.0 - n_dot_l), 0.0005);

  // Calculate the correct projected coordinates
  
  vec3 light_pos   = (o_shadow_pos.xyz / o_shadow_pos.w);
  vec3 proj_coords = vec3((light_pos.xy + 1.0) * 0.5, light_pos.z - bias);

  // Calculate the shadow factor with some bounds checking
  
  float shadow_factor = 1.0f;
  bool in_bounds      = (proj_coords.x >= 0.0 && proj_coords.x <= 1.0) && 
                        (proj_coords.y >= 0.0 && proj_coords.y <= 1.0) && 
                        (light_pos.z <= 1.0);

  if(in_bounds) {
    shadow_factor = texture(sampler2DShadow(u_shadow_map, u_shadow_sampler), proj_coords);
  }
  
  // Calculating the final factor
 
  vec3 ambient_term = (o_ambient_color * desc.albedo_texel);
  vec3 final_factor = ((dir_light_factor * shadow_factor) + point_lights_factor + emissive + ambient_term);

  // Done!
  frag_color = vec4(final_factor, material.color.a);
}

// Function implementations 

vec3 calculate_normal(const vec3 texel) {
  vec3 normal = 2.0 * texel - 1.0; // From [0, 1] to [-1, 1]
  return normalize(o_TBN * normal);
}

float brdf_normal_distribution(const vec3 normal, const vec3 halfway, const float roughness) {
  float roughness_sq  = (roughness * roughness);
  float roughness2_sq = (roughness_sq * roughness_sq);

  float nh_dot    = max(dot(normal, halfway), 0.0);
  float nh_dot_sq = (nh_dot * nh_dot);

  float denominator = (nh_dot_sq * (roughness2_sq - 1.0) + 1.0);
  return roughness2_sq / (PI * denominator * denominator);
}

float brdf_geo_schlick_ggx(const float nx_dot, const float roughness) {
  float r = (roughness + 1.0);
  float k = (r * r) / 8.0;

  float nom   = nx_dot;
  float denom = nx_dot * (1.0 - k) + k;

  return nom / denom;
}

float brdf_shadowing_masking(const vec3 normal, const vec3 view_dir, const vec3 light_dir, const float roughness) {
  float nv_dot = max(dot(normal, view_dir), 0.0);
  float nl_dot = max(dot(normal, light_dir), 0.0);

  return brdf_geo_schlick_ggx(nv_dot, roughness) * brdf_geo_schlick_ggx(nl_dot, roughness);
}

vec3 brdf_fersnel_schlick_reflectance(const float cos_theta, const vec3 f0) {
  return f0 + (1 - f0) * pow(clamp((1.0 - cos_theta), 0.0, 1.0), 5.0);
}

BRDFResult brdf_calculate(const BRDFDesc desc) {
  // Default result

  BRDFResult res; 
  res.specular       = vec3(0.0);
  res.diffuse        = vec3(0.0f);
  res.radiance_color = vec3(0.0f);

  // Dot products
  
  vec3 halfway_dir = normalize(desc.light_dir + desc.view_dir);

  float hv_dot = max(dot(halfway_dir, desc.view_dir), 0.0);
  float nv_dot = max(dot(desc.normal, desc.view_dir), 0.0);
  float nl_dot = max(dot(desc.normal, desc.light_dir), 0.0);

  // Early out 

  if(nl_dot <= 0.0) {
    return res;
  }

  // Go through the BRDF terms

  float ndf = brdf_normal_distribution(desc.normal, halfway_dir, desc.roughness);
  float g   = brdf_shadowing_masking(desc.normal, desc.view_dir, desc.light_dir, desc.roughness);
  vec3 f    = brdf_fersnel_schlick_reflectance(hv_dot, desc.f0); 

  vec3 numerator    = ndf * g * f;
  float denominator = 4 * nv_dot * nl_dot + 0.0001;

  // Done!

  res.specular       = (numerator / denominator);
  res.diffuse        = (vec3(1.0) - f) * (1.0 - desc.metallic);
  res.radiance_color = (res.diffuse * desc.albedo_texel / PI + res.specular) * desc.radiance * nl_dot;

  return res;
}

float attenuate(const float distance, const float radius, const float max_intensity, const float fall_off) {
  /// @NOTE: 
  /// This attenuation function was taken from:
  /// https://lisyarus.github.io/blog/posts/point-light-attenuation.html
  /// 

  float s = distance / radius;
  if(s >= 1.0) {
    return 0.0;
  }

  float s2_sq  = (s * s);
  float inv_s2 = (1 - s2_sq);

  return max_intensity * (inv_s2 * inv_s2) / (1 + fall_off * s);
}

vec3 evaluate_directional_light(BRDFDesc desc) {
  desc.light_dir = normalize(-dir_light.direction);
  desc.radiance  = dir_light.color;

  BRDFResult res = brdf_calculate(desc);
  return res.radiance_color;
}

vec3 evalute_point_light(const int index, BRDFDesc desc) {
  vec3 light_pos   = p_lights.position_r[index].xyz;
  vec3 light_color = p_lights.color_fo[index].rgb;

  float light_radius  = p_lights.position_r[index].w;
  float light_falloff = p_lights.color_fo[index].a;

  vec3 to_light = light_pos - o_pixel_pos;
  float dist    = max(length(to_light), 0.0001);

  float atten = attenuate(dist, light_radius, 1.0, light_falloff);
  if(atten <= 0) {
    return vec3(0.0f);
  }

  desc.light_dir = to_light / dist;
  desc.radiance  = light_color * atten;

  BRDFResult res = brdf_calculate(desc);
  return res.radiance_color;
}

vec3 to_linear(vec3 value) {
  return pow(value, vec3(o_gamma));
}

@end

/// Light shader
/// --------------------------------------------------

/// --------------------------------------------------
/// Shadow shader

@vs shadow_vs
@glsl_options fixup_clipspace // important: map clipspace z from -1..+1 to 0..+1 on GL

// Includes
@include_block basic_vertex

// Main

void main() {
  gl_Position = u_light_space * u_data[u_index + gl_InstanceIndex].model * vec4(a_pos, 1.0);
}
@end

@fs shadow_fs

// Main

void main() {
}
@end

/// Shadow shader
/// --------------------------------------------------

/// --------------------------------------------------
/// Programs

@program lighting_pass light_vs light_fs 
@program shadow_pass shadow_vs shadow_fs

/// Programs
/// --------------------------------------------------
