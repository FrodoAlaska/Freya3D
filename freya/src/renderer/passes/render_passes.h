#pragma once

#include "freya_render.h"

#include "../shaders/main_shader.h"
#include "../shaders/billboard_shader.h"
#include "../shaders/screen_space_shader.h"
#include "../shaders/skybox_shader.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

///---------------------------------------------------------------------------------------------------------------------
/// Consts

const i32 SHADER_BINDING_FRAME_DATA  = UB_FrameData;
const i32 SHADER_BINDING_MODEL       = UB_ModelIndex;
const i32 SHADER_BINDING_LIGHT       = UB_Light;
const i32 SHADER_BINDING_POINT_LIGHT = UB_PointLight;
const i32 SHADER_BINDING_MATERIAL    = UB_Material;

const sizei INSTANCE_BUFFER_SIZE = KiB(128);
const u32 POINT_LIGHTS_MAX       = 16;

/// Consts
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// RenderPassID 
enum RenderPassID {
  RENDER_PASS_SHADOW, 
  RENDER_PASS_GEOMETRY,
  RENDER_PASS_DEFAULT,

  RENDER_PASSSES_COUNT,
};
/// RenderPassID 
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// RenderQueueID 
enum RenderQueueID {
  RENDER_QUEUE_SHADOW,
  RENDER_QUEUE_OPAQUE, 
  RENDER_QUEUE_BILLBOARD,
  RENDER_QUEUE_SCREEN_SPACE,
  RENDER_QUEUE_SKYBOX,

  RENDER_QUEUES_COUNT,
};
/// RenderQueueID 
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// RenderPass
struct RenderPass {
  sg_pass pass       = {};
  sg_sampler sampler = {};

  IVec2 frame_size;
  DynamicArray<sg_view> attachments;
};
/// RenderPass
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// RenderQueueEntry
struct RenderQueueEntry {
  sg_bindings bindings = {};

  u32 elements_count  = 0;
  u32 instances_count = 1;
 
  ModelIndex_t model;
  Material_t material;
};
/// RenderQueueEntry 
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// RenderQueue
struct RenderQueue {
  sg_pipeline pipeline;
  DynamicArray<RenderQueueEntry> entries;
 
  sg_buffer transforms_buffer;
  sg_view transforms_view;

  DynamicArray<Mat4> transforms;
};
/// RenderQueue
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// GeometryPassData 
struct GeometryPassData {
  RenderQueue* skybox_queue    = nullptr;
  RenderQueue* opaque_queue    = nullptr;
  RenderQueue* billboard_queue = nullptr;

  FrameData_t frame_data = {};

  PointLight_t point_lights = {};
  Light_t dir_light         = {};
};
/// GeometryPassData 
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Shadow pass functions

void shadow_pass_init(RenderPass& pass, Window* window, const IVec2& shadow_size = IVec2(1024));
void shadow_pass_prepare(RenderPass& pass, const Camera& camera, const DirectionalLightComponent& light, Mat4& out_mat);
void shadow_pass_commit(RenderPass& pass, RenderQueue& queue, FrameData_t& frame_data);

/// Shadow pass functions
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Geometry pass functions

void geometry_pass_init(RenderPass& pass, Window* window);
void geometry_pass_commit(RenderPass& pass, const GeometryPassData& data);

/// Geometry pass functions
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Default pass functions

void default_pass_commit(RenderPass& pass, RenderQueue& queue, Window* window);

/// Default pass functions
///---------------------------------------------------------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
