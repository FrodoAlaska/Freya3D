#include "render_passes.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

///---------------------------------------------------------------------------------------------------------------------
/// Default pass functions

void default_pass_commit(RenderPass& pass, RenderQueue& queue, Window* window, const FrameData_t& data) {
  // Set up the swapchain 

  const Color& color = renderer_get_clear_color(); 

  pass.pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;
  pass.pass.action.colors[0].clear_value = {
    color.r, 
    color.g, 
    color.b, 
    color.a
  };

  pass.pass.swapchain = renderer_get_default_swapchain();

  // Begin the pass 

  pass.frame_size = window_get_size(window);

  sg_begin_pass(&pass.pass);
  sg_apply_viewport(0, 0, pass.frame_size.x, pass.frame_size.y, true);
  
  //
  // Draw the final image
  //

  // Flush the default queue  
  sg_apply_pipeline(queue.pipeline); 

  // Apply the global uniforms 
  sg_apply_uniforms(SHADER_BINDING_FRAME_DATA, SG_RANGE(data));

  // Render each entry

  for(auto& entry : queue.entries) {
    sg_apply_bindings(entry.bindings);
    sg_draw(0, entry.elements_count, 1);
  }

  // Done!
  sg_end_pass();  
}

/// Default pass functions
///---------------------------------------------------------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
