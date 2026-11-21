#include "render_passes.h"

#include "../shaders/bloom_shader.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

///---------------------------------------------------------------------------------------------------------------------
/// BloomSubPass
struct BloomSubPass {
  sg_image image;

  sg_view color_view; 
  sg_view texture_view;

  sg_pass pass = {};
  IVec2 size;
};
/// BloomSubPass
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// BloomPass
struct BloomPass {
  DynamicArray<BloomSubPass> levels; 

  f32 threshold     = 0.0f;
  f32 knee          = 0.5f;
  f32 filter_radius = 0.0f;
};

static BloomPass s_bloom;
/// BloomPass
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Bloom pass functions

void bloom_pass_init(RenderPass& pass, Window* window) {
  // Resize the number of bloom levels 

  constexpr sizei BLOOM_LEVELS = 8; 
  s_bloom.levels.resize(BLOOM_LEVELS);

  //
  // Setup each bloom level 
  //

  // Setup the default size
  
  pass.frame_size = window_get_size(window);
  IVec2 size      = pass.frame_size / 2;

  // Setup the sampler 
  
  pass.sampler = sg_make_sampler(sg_sampler_desc{ 
    .min_filter = SG_FILTER_LINEAR,
    .mag_filter = SG_FILTER_LINEAR, 

    .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
    .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
  }); 

  // Go through each level and create the passes

  for(auto& level : s_bloom.levels) {
    // 
    // Setup the attachments 
    //

    level.size = size;

    // 
    // Create the color attachment 
    //
    
    // Set up the image 

    sg_image_desc image_desc = {};

    image_desc.width        = size.x;
    image_desc.height       = size.y;
    image_desc.sample_count = window_get_samples_count(window);

    image_desc.pixel_format           = SG_PIXELFORMAT_RGBA16F; 
    image_desc.usage.color_attachment = true;

    level.image = sg_make_image(image_desc);

    // Set up the view

    sg_view_desc view_desc           = {};
    view_desc.color_attachment.image = level.image;

    // Create the color view
    level.color_view = sg_make_view(view_desc);

    // Create the texture view to use later

    view_desc.color_attachment.image = {};
    view_desc.texture.image          = level.image;

    level.texture_view = sg_make_view(view_desc);

    // Set the main color attachment 
    level.pass.attachments.colors[0] = level.color_view;

    // Downscale the size 
    size /= 2;
  }

  // Setup the output 
  pass.attachments.emplace_back(s_bloom.levels[0].texture_view);
}

void bloom_pass_commit(RenderPass& pass, 
                       sg_view input_view,
                       RenderQueue& downscale_queue, 
                       RenderQueue& upscale_queue, 
                       Window* window) {
  // Set the defaults 

  sg_view input = input_view;
  IVec2 size    = pass.frame_size;

  // 
  // Downscale step 
  //

  for(sizei i = 0; i < s_bloom.levels.size(); i++) {
    // Begin the pass 
    
    BloomSubPass& level                     = s_bloom.levels[i];
    level.pass.action.colors[0].load_action = SG_LOADACTION_DONTCARE;

    sg_begin_pass(level.pass);
    sg_apply_viewport(0, 0, level.size.x, level.size.y, true);

    // Apply the pipeline 
    sg_apply_pipeline(downscale_queue.pipeline);

    // Apply the bindings 
    
    sg_bindings bind = {};

    bind.views[0]    = input;
    bind.samplers[0] = pass.sampler;

    sg_apply_bindings(bind);

    // Apply global uniforms 

    BloomData_t data = {};

    data.u_inv_size  = Vec2(1.0f / size.x, 1.0f / size.y);
    data.u_first     = (i == 0) ? 1.0f : 0.0f;
    data.u_threshold = s_bloom.threshold;
    data.u_knee      = s_bloom.knee;

    sg_apply_uniforms(0, SG_RANGE(data));

    // Draw 

    sg_draw(0, 3, 1);
    sg_end_pass();

    // Update the inputs 

    input = level.texture_view;
    size  = level.size;
  }

  // 
  // Upscale step 
  //

  for(sizei i = s_bloom.levels.size() - 1; i > 0; i--) {
    // Get the relavant bloom levels
    
    BloomSubPass& src_level = s_bloom.levels[i];
    BloomSubPass& dst_level = s_bloom.levels[i - 1];
    
    // Begin the pass 
    
    dst_level.pass.action.colors[0].load_action = SG_LOADACTION_LOAD;

    sg_begin_pass(dst_level.pass);
    sg_apply_viewport(0, 0, dst_level.size.x, dst_level.size.y, true);

    // Apply the pipeline 
    sg_apply_pipeline(upscale_queue.pipeline);

    // Apply the bindings 
    
    sg_bindings bind = {};

    bind.views[0]    = src_level.texture_view;
    bind.samplers[0] = pass.sampler;

    sg_apply_bindings(bind);

    // Apply global uniforms 

    BloomData_t data = {};

    data.u_inv_size = Vec2(1.0f / size.x, 1.0f / size.y);
    data.u_radius   = s_bloom.filter_radius;

    sg_apply_uniforms(0, SG_RANGE(data));

    // Draw 

    sg_draw(0, 3, 1);
    sg_end_pass();
  }
}

/// Bloom pass functions
///---------------------------------------------------------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
