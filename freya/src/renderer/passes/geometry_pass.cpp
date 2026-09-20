#include "render_passes.h"
#include "freya_logger.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

///---------------------------------------------------------------------------------------------------------------------
/// Geometry pass functions

void geometry_pass_init(RenderPass& pass, Window* window) {
  // 
  // Setup the attachments 
  //

  pass.frame_size = window_get_size(window);
  pass.sampler    = renderer_get_defaults().sampler;

  sg_pass& p = pass.pass;

  // 
  // Create the color attachment 
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

    const Color& color = renderer_get_clear_color();

    p.action.colors[0].load_action = SG_LOADACTION_CLEAR;
    p.action.colors[0].clear_value = {
      color.r, 
      color.g, 
      color.b, 
      color.a
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

    image_desc.pixel_format                   = SG_PIXELFORMAT_DEPTH_STENCIL; 
    image_desc.usage.depth_stencil_attachment = true;

    sg_image depth_img = sg_make_image(image_desc);

    // Set up the view

    sg_view_desc view_desc = {};

    view_desc.depth_stencil_attachment.image = depth_img;
    p.attachments.depth_stencil              = sg_make_view(view_desc);

    // Set up the action 

    p.action.depth             = {};
    p.action.depth.clear_value = 1.0f;

    p.action.stencil = {};

    // Create the texture view to use later

    view_desc.depth_stencil_attachment.image = {};
    view_desc.texture.image                  = depth_img;

    pass.attachments.emplace_back(sg_make_view(view_desc));
  }
}

void geometry_pass_commit(RenderPass& pass, const GeometryPassData& data) {
  FREYA_DEBUG_ASSERT(data.skybox_queue, "Skybox queue must be valid");
  FREYA_DEBUG_ASSERT(data.opaque_queue, "Opaque queue must be valid");

  // Set up the action 

  const Color& color = renderer_get_clear_color(); 

  pass.pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;
  pass.pass.action.colors[0].clear_value = {
    color.r, 
    color.g, 
    color.b, 
    color.a
  };

  // Begin the pass
  
  sg_begin_pass(&pass.pass);
  sg_apply_viewport(0, 0, pass.frame_size.x, pass.frame_size.y, true);
  
  //
  // Flush the skybox queue (if there is one)
  //
 
  RenderQueue* skybox_queue = data.skybox_queue;
  if(!skybox_queue->entries.empty()) { 

    // Apply the global uniforms

    sg_apply_pipeline(skybox_queue->pipeline); 
    sg_apply_uniforms(SHADER_BINDING_FRAME_DATA, SG_RANGE(data.frame_data));
    
    // Render the first (and probably only) skybox entry 
    
    RenderQueueEntry& entry = skybox_queue->entries[0];

    sg_apply_bindings(entry.bindings);
    sg_draw(0, entry.elements_count, 1);
  }
  
  //
  // Flush the opaque queue
  //
  
  RenderQueue* opaque_queue = data.opaque_queue;
  { 
    // Update the transforms buffer
    // @NOTE: We need to do this check or we'll get a seg fault, 
    // if the same is empty.

    if(!opaque_queue->transforms.empty()) {
      sg_range range = {
        .ptr  = opaque_queue->transforms.data(), 
        .size = opaque_queue->transforms.size() * sizeof(Mat4), 
      };
      sg_update_buffer(opaque_queue->transforms_buffer, range);
    }
    
    // Apply the global uniforms

    sg_apply_pipeline(opaque_queue->pipeline); 
    
    sg_apply_uniforms(SHADER_BINDING_FRAME_DATA, SG_RANGE(data.frame_data));
    sg_apply_uniforms(SHADER_BINDING_LIGHT, SG_RANGE(data.dir_light));
    sg_apply_uniforms(SHADER_BINDING_POINT_LIGHT, SG_RANGE(data.point_lights));

    // Render each entry

    for(auto& entry : opaque_queue->entries) {
      // Apply local uniforms
      
      sg_apply_uniforms(SHADER_BINDING_MODEL, SG_RANGE(entry.model));
      sg_apply_uniforms(SHADER_BINDING_MATERIAL, SG_RANGE(entry.material));

      // Apply and draw

      sg_apply_bindings(entry.bindings);
      sg_draw(0, entry.elements_count, entry.instances_count);
    }
  }
 
  // Done!
  sg_end_pass();  
}

/// Geometry pass functions
///---------------------------------------------------------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
