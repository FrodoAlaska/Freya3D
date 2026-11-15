#include "render_passes.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

///---------------------------------------------------------------------------------------------------------------------
/// Shadow pass functions

void shadow_pass_init(RenderPass& pass, Window* window, const IVec2& shadow_size) {
  // 
  // Setup the attachments 
  //
  
  sg_pass& p = pass.pass;

  pass.frame_size = shadow_size;
  pass.sampler    = sg_make_sampler(sg_sampler_desc{ 
    .min_filter = SG_FILTER_LINEAR,
    .mag_filter = SG_FILTER_LINEAR, 

    .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
    .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
  }); 

  // 
  // Create the shadow texture image 
  // 
  {
    // Set up the image 
    
    sg_image_desc image_desc = {};

    image_desc.width        = pass.frame_size.x;
    image_desc.height       = pass.frame_size.y;
    image_desc.sample_count = window_get_samples_count(window);

    image_desc.pixel_format           = SG_PIXELFORMAT_RGBA8; 
    image_desc.usage.color_attachment = true;

    sg_image color_img = sg_make_image(image_desc);

    // Set up the view

    sg_view_desc view_desc           = {};
    view_desc.color_attachment.image = color_img;

    // Create the view
    p.attachments.colors[0] = sg_make_view(view_desc);

    // Set up the action 

    p.action.colors[0].load_action = SG_LOADACTION_CLEAR;
    p.action.colors[0].clear_value = {
      1.0f, 1.0f, 1.0f, 1.0f
    };

    // Create the texture view to use later

    view_desc.color_attachment.image = {};
    view_desc.texture.image          = color_img;

    pass.attachments.emplace_back(sg_make_view(view_desc));
  }

  //
  // Create the depth-stencil attachment
  //
  {
    // Set up the image 

    sg_image_desc image_desc = {};

    image_desc.width        = pass.frame_size.x;
    image_desc.height       = pass.frame_size.y;
    image_desc.sample_count = window_get_samples_count(window);

    image_desc.pixel_format                   = SG_PIXELFORMAT_DEPTH; 
    image_desc.usage.depth_stencil_attachment = true;

    sg_image depth_img = sg_make_image(image_desc);

    // Set up the view

    sg_view_desc view_desc = {};

    view_desc.depth_stencil_attachment.image = depth_img;
    p.attachments.depth_stencil              = sg_make_view(view_desc);

    // Set up the action 

    p.action.depth = {};

    p.action.depth.load_action  = SG_LOADACTION_CLEAR;
    p.action.depth.store_action = SG_STOREACTION_STORE;
    p.action.depth.clear_value  = 1.0f;

    p.action.stencil = {};

    // Create the texture view to use later

    view_desc.depth_stencil_attachment.image = {};
    view_desc.texture.image                  = depth_img;

    pass.attachments.emplace_back(sg_make_view(view_desc));
  }
}

void shadow_pass_prepare(RenderPass& pass, const Camera& camera, const DirectionalLightComponent& light, Mat4& out_mat) {
  //
  // Setup the light projection matrix
  //

  // Get the center of the frustrum by averaging the 
  // frustrum's corners.
  
  Vec3 center = Vec3(0.0f);
  for(u32 i = 0; i < CAMERA_FRUSTUM_CORNERS_MAX; i++) {
    center += camera.corners[i]; 
  }
  center /= (f32)CAMERA_FRUSTUM_CORNERS_MAX;

  // Calculate the light's view matrix for use later 
  Mat4 light_view = mat4_look_at(center - light.direction, center, Vec3(0.0f, 1.0f, 0.0f));

  // Calculate the extents of the frustrum

  Vec3 min = Vec3(FLOAT_MAX);
  Vec3 max = Vec3(FLOAT_MIN);

  for(sizei i = 0; i < CAMERA_FRUSTUM_CORNERS_MAX; i++) {
    Vec3 center_light_space = Vec3(light_view * Vec4(camera.corners[i], 1.0f));

    min = vec3_min(min, center_light_space);
    max = vec3_max(max, center_light_space);
  }

  // Calculate the projection and view matrices and 
  // send the final result to the shader.
  out_mat = mat4_ortho(min.x, max.x, min.y, max.y, min.z, max.z) * light_view;
}

void shadow_pass_commit(RenderPass& pass, RenderQueue& queue, FrameData_t& frame_data) {
  // Begin the pass
  
  sg_begin_pass(&pass.pass);
  sg_apply_viewport(0, 0, pass.frame_size.x, pass.frame_size.y, true);
  
  //
  // Flush the opaque queue
  //
  
  // Update the transforms buffer
  //
  // @NOTE: We need to do this check or we'll get a seg fault, 
  // if the same is empty.
  //

  if(!queue.transforms.empty()) {
    sg_range range = {
      .ptr  = queue.transforms.data(), 
      .size = queue.transforms.size() * sizeof(Mat4), 
    };
    sg_update_buffer(queue.transforms_buffer, range);
  }
  
  // Apply the global uniforms
  
  sg_apply_pipeline(queue.pipeline); 
  sg_apply_uniforms(SHADER_BINDING_FRAME_DATA, SG_RANGE(frame_data));
 
  // Render each entry

  for(auto& entry : queue.entries) {
    // Apply local uniforms
    sg_apply_uniforms(SHADER_BINDING_MODEL, SG_RANGE(entry.model));

    // Apply and draw

    sg_apply_bindings(entry.bindings);
    sg_draw(0, entry.elements_count, entry.instances_count);
  }
 
  // Done!
  sg_end_pass();  
}

/// Shadow pass functions
///---------------------------------------------------------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
