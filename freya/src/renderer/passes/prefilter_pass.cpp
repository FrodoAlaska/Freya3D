#include "render_passes.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

///---------------------------------------------------------------------------------------------------------------------
/// Pre-filter pass functions

void prefilter_pass_init(RenderPass& pass, Window* window) {
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

    image_desc.pixel_format           = SG_PIXELFORMAT_RGBA16F; 
    image_desc.usage.color_attachment = true;

    sg_image color_img = sg_make_image(image_desc);

    // Set up the view

    sg_view_desc view_desc           = {};
    view_desc.color_attachment.image = color_img;

    // Create the view
    p.attachments.colors[0] = sg_make_view(view_desc);

    // Set up the action 

    p.action.colors[0].load_action = SG_LOADACTION_CLEAR;
    p.action.colors[0].clear_value = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Create the texture view to use later

    view_desc.color_attachment.image = {};
    view_desc.texture.image          = color_img;

    pass.attachments.emplace_back(sg_make_view(view_desc));
  }
}

void prefilter_pass_commit(RenderPass& pass, RenderQueue& queue, Window* window) {
  // Set up the swapchain 

  pass.pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;
  pass.pass.action.colors[0].clear_value = { 1.0f, 1.0f, 1.0f, 1.0f };

  // Begin the pass 

  pass.frame_size = window_get_size(window);

  sg_begin_pass(&pass.pass);
  sg_apply_viewport(0, 0, pass.frame_size.x, pass.frame_size.y, true);
  
  //
  // Draw the final image
  //

  // Flush the default queue  
  sg_apply_pipeline(queue.pipeline); 

  // Render the first entry

  RenderQueueEntry& entry = queue.entries[0];
  
  sg_apply_bindings(entry.bindings);
  sg_draw(0, entry.elements_count, 1);

  // Done!
  sg_end_pass();  
}

/// Pre-filter pass functions
///---------------------------------------------------------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
