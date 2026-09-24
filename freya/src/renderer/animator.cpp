#include "freya_render.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

///---------------------------------------------------------------------------------------------------------------------
/// Animator functions

void animator_create(Animator& out_anim, const AnimatorDesc& desc) {
  // Frames init

  for(auto& frame : desc.frames_id) {
    out_anim.frames.emplace_back(asset_group_get_texture(frame));
  }

  out_anim.current = &out_anim.frames[0];

  // Other variables init

  out_anim.current_index = 0;
  out_anim.direction     = 1;

  out_anim.counter    = 0.0f;
  out_anim.flip_speed = desc.flip_speed;

  out_anim.is_active = true; 

  out_anim.can_loop      = desc.can_loop;
  out_anim.can_alternate = desc.can_alternate;
}

void animator_update(Animator& anim, f32 dt) {
  // Invactive animation... don't do anything

  if(!anim.is_active) {
    return;
  } 

  // Manage the timers

  anim.counter += dt; 
  if(anim.counter >= anim.flip_speed) {
    anim.counter        = 0.0f;
    anim.current_index += anim.direction;
  }

  // Manage the index
  // @NOTE: Shitiest code I've written in a while, but I'm tired lol

  if(anim.current_index >= anim.frames.size()) {
    if(anim.can_alternate) {
      anim.current_index = (anim.frames.size() - 1);
      anim.direction     = -1;
    }
    else if(anim.can_loop) {
      anim.current_index = 0;
    }
  }
  else if(anim.current_index < 0) {
    if(anim.can_alternate) {
      anim.current_index = 0;
      anim.direction     = 1;
    }
    else if(anim.can_loop) {
      anim.current_index = (anim.frames.size() - 1);
    }
  }

  // Change the current texture
  anim.current = &anim.frames[anim.current_index];
}

/// Animator functions
///---------------------------------------------------------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
