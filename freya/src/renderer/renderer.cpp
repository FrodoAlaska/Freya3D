#include "freya_render.h"
#include "freya_logger.h"
#include "freya_event.h"
#include "freya_entity.h"

#include "passes/render_passes.h"
#include "shaders/default_pass_shader.h"

#include "fontstash/fontstash.h"

#include "sokol/sokol_gp.h"
#include "sokol/sokol_gl.h"
#include "sokol/sokol_fontstash.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

///---------------------------------------------------------------------------------------------------------------------
/// ShaderID
enum ShaderID {
  SHADER_SHADOW,
  SHADER_LIGHTING,
  SHADER_BILLBOARD,
  SHADER_SCREEN_SPACE,
  SHADER_SKYBOX,

  SHADERS_COUNT,
};
/// ShaderID
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Renderer
struct Renderer {
  /// Defaults 

  RendererDefaults defaults;
  Array<AssetID, SHADERS_COUNT> shaders;

  /// Important state

  Window* window   = nullptr;
  IVec2 frame_size = IVec2(0);
  
  AssetGroupID group_id = ASSET_CACHE_ID;
  EntityWorld* world    = nullptr;
  
  /// Passes

  Array<RenderQueue, RENDER_QUEUES_COUNT> queues;
  Array<RenderPass, RENDER_PASSSES_COUNT> passes;
  
  /// Per-frame state

  Color color            = Color(1.0f);
  FrameData_t frame_data = {};

  PointLight_t point_lights = {};
  Light_t dir_light         = {};

  /// UI

  FONScontext* fons = nullptr;
};

static Renderer s_renderer;
/// Renderer
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Callbacks

static bool window_resized_callback(const Event& event, const void* dispatcher, const void* listener) {
  // @TODO (Renderer/resize) 
  return true;
}

static void sg_logger_func(const char* tag, 
                           u32 level, 
                           u32 item, 
                           const char* msg, 
                           u32 line, 
                           const char* filename, 
                           void* user_data) {
  switch(level) {
    case 0: // Panic
      FREYA_LOG_FATAL("%s - (%i, %s)", msg, line, filename); 
      FREYA_ASSERT_LOG(false, "Fatal SOKOL error encountered");
      break;
    case 1: // Error
      FREYA_LOG_ERROR("[SOKOL-ERROR]: %s - (%i, %s)", msg, line, filename); 
      break;
    case 2: // Warning
      FREYA_LOG_WARN("[SOKOL-WARN]: %s - (%i, %s)", msg, line, filename); 
      break;
    case 3: // Info
      FREYA_LOG_INFO("[SOKOL-INFO]: %s - (%i, %s)", msg, line, filename); 
      break;
  }
}

/// Callbacks
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Private functions

static void init_defaults() {
  //
  // Textures init
  //
  {
    AssetID texture_ids[] = {
      asset_group_push_texture(s_renderer.group_id, MATERIAL_TEXTURE_ALBEDO),
      asset_group_push_texture(s_renderer.group_id, MATERIAL_TEXTURE_ROUGHNESS),
      asset_group_push_texture(s_renderer.group_id, MATERIAL_TEXTURE_METALLIC),
      asset_group_push_texture(s_renderer.group_id, MATERIAL_TEXTURE_NORMAL),
    };

    s_renderer.defaults.albedo_map    = asset_group_get_texture(texture_ids[0]);
    s_renderer.defaults.roughness_map = asset_group_get_texture(texture_ids[1]);
    s_renderer.defaults.metallic_map  = asset_group_get_texture(texture_ids[3]);
    s_renderer.defaults.normal_map    = asset_group_get_texture(texture_ids[2]);

    // Material init

    MaterialDesc mat_desc = {
      .albedo_id    = texture_ids[0], 
      .roughness_id = texture_ids[1], 
      .metallic_id  = texture_ids[3], 
      .normal_id    = texture_ids[2], 
    };
    s_renderer.defaults.material = asset_group_get_material(asset_group_push_material(s_renderer.group_id, mat_desc));
  }

  //
  // Quad mesh init
  // 
  {
    f32 vertices[] = {
      -1.0f, -1.0f, 0.0f, 0.0f,
       1.0f, -1.0f, 1.0f, 0.0f,
       1.0f,  1.0f, 1.0f, 1.0f,
      -1.0f,  1.0f, 0.0f, 1.0f,
    };

    u16 indices[] = {
      0, 1, 2, 
      2, 3, 0
    };

    sg_buffer_desc vert_desc = {};
    vert_desc.data           = SG_RANGE(vertices);
    
    sg_buffer_desc idx_desc     = {};
    idx_desc.usage.index_buffer = true;
    idx_desc.data               = SG_RANGE(indices);

    s_renderer.defaults.quad_mesh = asset_group_get_mesh(asset_group_push_mesh(s_renderer.group_id, vert_desc, idx_desc, 6, {}));
  }
  
  //
  // Skybox mesh init
  // 
  {
    f32 vertices[] = {
      -1.0f, -1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,
       1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,

      -1.0f, -1.0f,  1.0f,
       1.0f, -1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
      -1.0f,  1.0f,  1.0f,

      -1.0f, -1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f,  1.0f,
      -1.0f, -1.0f,  1.0f,

       1.0f, -1.0f, -1.0f,
       1.0f,  1.0f, -1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f, -1.0f,  1.0f,

      -1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f,  1.0f,
       1.0f, -1.0f,  1.0f,
       1.0f, -1.0f, -1.0f,

      -1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f,  1.0f, -1.0f,
    };

    u16 indices[] = {
      0, 1, 2,  
      0, 2, 3,
      6, 5, 4, 
      7, 6, 4,
      8, 9, 10,  
      8, 10, 11,
      14, 13, 12,  
      15, 14, 12,
      16, 17, 18,  
      16, 18, 19,
      22, 21, 20,  
      23, 22, 20
    };
    
    sg_buffer_desc vert_desc = {};
    vert_desc.data           = SG_RANGE(vertices);
    
    sg_buffer_desc idx_desc     = {};
    idx_desc.usage.index_buffer = true;
    idx_desc.data               = SG_RANGE(indices);

    s_renderer.defaults.skybox_mesh = asset_group_get_mesh(asset_group_push_mesh(s_renderer.group_id, vert_desc, idx_desc, 36, {}));
  }

  // 
  // Billboard mesh init 
  //
  {
    f32 vertices[] = {
      // Position          Normal             Texture coords
      
      -1.0f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
      -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
       1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
       1.0f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
    };

    u16 indices[] = {
      0, 1, 2, 
      2, 3, 0
    };
    
    sg_buffer_desc vert_desc = {};
    vert_desc.data           = SG_RANGE(vertices);
    
    sg_buffer_desc idx_desc     = {};
    idx_desc.usage.index_buffer = true;
    idx_desc.data               = SG_RANGE(indices);

    AssetID mesh_id                    = asset_group_push_mesh(s_renderer.group_id, vert_desc, idx_desc, 6, {});
    s_renderer.defaults.billboard_mesh = asset_group_get_mesh(mesh_id);
  }

  // Sampler init 
  s_renderer.defaults.sampler = sg_make_sampler(sg_sampler_desc{});
  
  FREYA_LOG_TRACE("Renderer defaults initialized");
}

static void init_shaders() {
  sg_backend backend = sg_query_backend();

  const sg_shader_desc* shader_descs[SHADERS_COUNT] = {
    shadow_pass_shader_desc(backend),
    lighting_pass_shader_desc(backend),
    bb_billboard_pass_shader_desc(backend),
    screen_space_shader_desc(backend),
    sk_skybox_shader_desc(backend),
  };

  for(sizei i = 0; i < s_renderer.shaders.size(); i++) {
    s_renderer.shaders[i] = asset_group_push_shader(s_renderer.group_id, *shader_descs[i]);
  }
   
  FREYA_LOG_TRACE("Default shaders initialized");
}

static void init_queues() {
  //
  // Shadow queue init 
  //
  {
    RenderQueue& queue = s_renderer.queues[RENDER_QUEUE_SHADOW];

    // Pipeline init

    sg_pipeline_desc pipe_desc = {};

    pipe_desc.depth.pixel_format  = SG_PIXELFORMAT_DEPTH;
    pipe_desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    pipe_desc.depth.write_enabled = true;

    pipe_desc.shader       = asset_group_get_shader(s_renderer.shaders[SHADER_SHADOW]); 
    pipe_desc.sample_count = window_get_samples_count(s_renderer.window);

    pipe_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3; // Position
    pipe_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT3; // Normal
    pipe_desc.layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT3; // Tangant
    pipe_desc.layout.attrs[3].format = SG_VERTEXFORMAT_FLOAT2; // Texture coords

    pipe_desc.index_type             = SG_INDEXTYPE_UINT16;
    pipe_desc.colors[0].pixel_format = SG_PIXELFORMAT_NONE;

    queue.pipeline = sg_make_pipeline(pipe_desc);

    // Init transforms buffer 

    sg_buffer_desc buff_desc = {};

    buff_desc.size = INSTANCE_BUFFER_SIZE;
    
    buff_desc.usage.storage_buffer = true; 
    buff_desc.usage.stream_update  = true;

    sg_buffer inst_buffer = sg_make_buffer(buff_desc);
    
    // Init transforms buffer view 
  
    sg_view_desc view_desc          = {};
    view_desc.storage_buffer.buffer = inst_buffer;

    sg_view inst_view = sg_make_view(view_desc);

    // Add them to the queue
  
    queue.transforms_buffer = inst_buffer;
    queue.transforms_view   = inst_view;
  }
 
  //
  // Opaque queue init 
  //
  {
    RenderQueue& queue = s_renderer.queues[RENDER_QUEUE_OPAQUE];

    // Pipeline init

    sg_pipeline_desc pipe_desc = {};

    pipe_desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    pipe_desc.depth.write_enabled = true;

    pipe_desc.shader       = asset_group_get_shader(s_renderer.shaders[SHADER_LIGHTING]); 
    pipe_desc.sample_count = window_get_samples_count(s_renderer.window);

    pipe_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3; // Position
    pipe_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT3; // Normal
    pipe_desc.layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT3; // Tangant
    pipe_desc.layout.attrs[3].format = SG_VERTEXFORMAT_FLOAT2; // Texture coords
   
    pipe_desc.colors[0].pixel_format = SG_PIXELFORMAT_RGBA16F;
    pipe_desc.index_type             = SG_INDEXTYPE_UINT16;

    queue.pipeline = sg_make_pipeline(pipe_desc);

    // Init transforms buffer 

    sg_buffer_desc buff_desc = {};

    buff_desc.size = INSTANCE_BUFFER_SIZE;
    
    buff_desc.usage.storage_buffer = true; 
    buff_desc.usage.stream_update  = true;

    sg_buffer inst_buffer = sg_make_buffer(buff_desc);
    
    // Init transforms buffer view 
  
    sg_view_desc view_desc          = {};
    view_desc.storage_buffer.buffer = inst_buffer;

    sg_view inst_view = sg_make_view(view_desc);

    // Add them to the queue
  
    queue.transforms_buffer = inst_buffer;
    queue.transforms_view   = inst_view;
  }
  
  //
  // Billboard queue init 
  //
  {
    RenderQueue& queue = s_renderer.queues[RENDER_QUEUE_BILLBOARD];

    // Pipeline init

    sg_pipeline_desc pipe_desc = {};

    pipe_desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    pipe_desc.depth.write_enabled = true;

    pipe_desc.shader       = asset_group_get_shader(s_renderer.shaders[SHADER_BILLBOARD]); 
    pipe_desc.sample_count = window_get_samples_count(s_renderer.window);

    pipe_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3; // Position
    pipe_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT3; // Normal
    pipe_desc.layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT2; // Texture coords
    
    pipe_desc.colors[0].pixel_format = SG_PIXELFORMAT_RGBA16F;
    
    pipe_desc.index_type                = SG_INDEXTYPE_UINT16;
    pipe_desc.alpha_to_coverage_enabled = true;

    queue.pipeline = sg_make_pipeline(pipe_desc);

    // Init transforms buffer 

    sg_buffer_desc buff_desc = {};

    buff_desc.size = INSTANCE_BUFFER_SIZE;
    
    buff_desc.usage.storage_buffer = true; 
    buff_desc.usage.stream_update  = true;

    sg_buffer inst_buffer = sg_make_buffer(buff_desc);
    
    // Init transforms buffer view 
  
    sg_view_desc view_desc          = {};
    view_desc.storage_buffer.buffer = inst_buffer;

    sg_view inst_view = sg_make_view(view_desc);

    // Add them to the queue
  
    queue.transforms_buffer = inst_buffer;
    queue.transforms_view   = inst_view;
  }
 
  //
  // Screen-space queue init 
  //
  {
    sg_pipeline_desc pipe_desc = {};

    pipe_desc.shader       = asset_group_get_shader(s_renderer.shaders[SHADER_SCREEN_SPACE]); 
    pipe_desc.sample_count = window_get_samples_count(s_renderer.window);

    pipe_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2; // Position
    pipe_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2; // Texture coords

    pipe_desc.index_type = SG_INDEXTYPE_UINT16;

    s_renderer.queues[RENDER_QUEUE_SCREEN_SPACE].pipeline = sg_make_pipeline(pipe_desc);
  }
 
  //
  // Skybox queue init 
  //
  {
    sg_pipeline_desc pipe_desc = {};
    
    pipe_desc.depth.compare       = SG_COMPAREFUNC_LESS_EQUAL;
    pipe_desc.depth.write_enabled = true;

    pipe_desc.shader       = asset_group_get_shader(s_renderer.shaders[SHADER_SKYBOX]); 
    pipe_desc.sample_count = window_get_samples_count(s_renderer.window);
    
    pipe_desc.colors[0].pixel_format = SG_PIXELFORMAT_RGBA16F;

    pipe_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3; // Position
    pipe_desc.index_type             = SG_INDEXTYPE_UINT16;

    s_renderer.queues[RENDER_QUEUE_SKYBOX].pipeline = sg_make_pipeline(pipe_desc);
  }
  
  FREYA_LOG_TRACE("Render queues initialized");
}

static void render_queue_mesh(RenderQueue& queue, MeshComponent& comp) {
  RenderQueueEntry entry;

  // Setup the bindings of the entry

  entry.bindings.vertex_buffers[0] = comp.mesh.vertex_buffer; 
  entry.bindings.index_buffer      = comp.mesh.index_buffer; 

  entry.bindings.views[0] = queue.transforms_view;
  entry.bindings.views[1] = comp.material.albedo_map.view;
  entry.bindings.views[2] = comp.material.roughness_map.view;
  entry.bindings.views[3] = comp.material.metallic_map.view;
  entry.bindings.views[4] = comp.material.normal_map.view;
  entry.bindings.views[5] = s_renderer.passes[RENDER_PASS_SHADOW].attachments[0];

  entry.bindings.samplers[0] = comp.material.albedo_map.sampler;
  entry.bindings.samplers[1] = comp.material.roughness_map.sampler;
  entry.bindings.samplers[2] = comp.material.metallic_map.sampler;
  entry.bindings.samplers[3] = comp.material.normal_map.sampler;
  entry.bindings.samplers[4] = s_renderer.passes[RENDER_PASS_SHADOW].sampler;

  // Setup the material 

  entry.material = {
    comp.material.color, 
    comp.material.roughness, 
    comp.material.metallic,
  };

  // Fill the rest of the entry

  entry.model.u_index = (i32)queue.entries.size();

  entry.elements_count  = comp.mesh.elements_count;
  entry.instances_count = 1;

  // Pushing into the queue

  queue.transforms.emplace_back(comp.transform);
  queue.entries.push_back(entry);

  // Pushing to the shadow queue if possible 

  if(comp.material.shadows_enabled) {
    RenderQueue& shadow = s_renderer.queues[RENDER_QUEUE_SHADOW];
    RenderQueueEntry shadow_entry; 

    // Setup the the entry

    shadow_entry.model.u_index = (i32)shadow.entries.size();

    shadow_entry.bindings                   = {0};
    shadow_entry.bindings.vertex_buffers[0] = comp.mesh.vertex_buffer; 
    shadow_entry.bindings.index_buffer      = comp.mesh.index_buffer; 

    shadow_entry.bindings.views[0] = shadow.transforms_view;

    shadow_entry.elements_count  = comp.mesh.elements_count;
    shadow_entry.instances_count = 1;

    // Pushing into the queue

    shadow.transforms.emplace_back(comp.transform);
    shadow.entries.push_back(shadow_entry);
  }
}

static void render_queue_mesh_instanced(RenderQueue& queue, MeshInstancedComponent& comp) {
  RenderQueueEntry entry;

  // Setup the bindings of the entry

  entry.bindings.vertex_buffers[0] = comp.mesh.vertex_buffer; 
  entry.bindings.index_buffer      = comp.mesh.index_buffer; 

  entry.bindings.views[0] = queue.transforms_view;
  entry.bindings.views[1] = comp.material.albedo_map.view;
  entry.bindings.views[2] = comp.material.roughness_map.view;
  entry.bindings.views[3] = comp.material.metallic_map.view;
  entry.bindings.views[4] = comp.material.normal_map.view;
  entry.bindings.views[5] = s_renderer.passes[RENDER_PASS_SHADOW].attachments[0];

  entry.bindings.samplers[0] = comp.material.albedo_map.sampler;
  entry.bindings.samplers[1] = comp.material.roughness_map.sampler;
  entry.bindings.samplers[2] = comp.material.metallic_map.sampler;
  entry.bindings.samplers[3] = comp.material.normal_map.sampler;
  entry.bindings.samplers[4] = s_renderer.passes[RENDER_PASS_SHADOW].sampler;

  // Setup the material 

  entry.material = {
    comp.material.color, 
    comp.material.roughness, 
    comp.material.metallic,
  };

  // Fill the rest of the entry

  entry.model.u_index = (i32)queue.entries.size();

  entry.elements_count  = comp.mesh.elements_count;
  entry.instances_count = (u32)comp.transforms.size();

  // Pushing into the queue

  queue.transforms.insert(queue.transforms.end(), comp.transforms.begin(), comp.transforms.end());
  queue.entries.push_back(entry);

  // Pushing to the shadow queue if possible 

  if(comp.material.shadows_enabled) {
    RenderQueue& shadow = s_renderer.queues[RENDER_QUEUE_SHADOW];
    RenderQueueEntry shadow_entry; 

    // Setup the the entry

    shadow_entry.model.u_index = (i32)shadow.entries.size();

    shadow_entry.bindings                   = {0};
    shadow_entry.bindings.vertex_buffers[0] = comp.mesh.vertex_buffer; 
    shadow_entry.bindings.index_buffer      = comp.mesh.index_buffer; 

    shadow_entry.bindings.views[0] = shadow.transforms_view;

    shadow_entry.elements_count  = comp.mesh.elements_count;
    shadow_entry.instances_count = (u32)comp.transforms.size();

    // Pushing into the queue

    shadow.transforms.insert(shadow.transforms.end(), comp.transforms.begin(), comp.transforms.end());
    shadow.entries.push_back(shadow_entry);
  }
}

/// Private functions
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Renderer functions

void renderer_init(Window* window) {
  // Window and frame init

  s_renderer.window     = window;
  s_renderer.frame_size = window_get_size(s_renderer.window); 

  // GFX init

  sg_desc gfx_desc     = {};
  gfx_desc.logger.func = sg_logger_func;

  gfx_desc.environment.defaults = sg_environment_defaults {
    .color_format = SG_PIXELFORMAT_RGBA8,
    .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
    .sample_count = window_get_samples_count(s_renderer.window),
  };

  sg_setup(&gfx_desc);
  FREYA_ASSERT_LOG(sg_isvalid(), "Failed to initialize the graphics context");
  
  // SGL init

  sgl_desc_t gl_desc = {
    .color_format = SG_PIXELFORMAT_RGBA8,
    .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
    .sample_count = window_get_samples_count(s_renderer.window),
  };

  gl_desc.logger.func = sg_logger_func;
  sgl_setup(gl_desc);

  // SGP init

  sgp_desc gp_desc = {};
  sgp_setup(&gp_desc);
  
  FREYA_ASSERT_LOG(sgp_is_valid(), "Failed to initialize the graphics painter");

  // FONS init
 
  sfons_desc_t fons_desc = {
    .width  = 512, 
    .height = 512, 
  };
  s_renderer.fons = sfons_create(&fons_desc);

  // Defaults init 

  init_defaults();
  init_shaders();
  init_queues();
  
  s_renderer.frame_data.u_ambient_color = Vec3(1.0f);
  s_renderer.frame_data.u_gamma         = 2.2f;
  s_renderer.frame_data.u_exposure      = 1.0f;

  s_renderer.dir_light.direction = Vec3(0.0f);
  s_renderer.dir_light.color     = Vec3(1.0f);

  s_renderer.point_lights.count = 0;

  // Render passes init

  shadow_pass_init(s_renderer.passes[RENDER_PASS_SHADOW], s_renderer.window, IVec2(1280));
  geometry_pass_init(s_renderer.passes[RENDER_PASS_GEOMETRY], s_renderer.window);

  // Listen to events
   
  event_register(EVENT_WINDOW_MAXIMIZED, window_resized_callback);
  event_register(EVENT_WINDOW_FRAMEBUFFER_RESIZED, window_resized_callback);
  event_register(EVENT_WINDOW_FULLSCREEN, window_resized_callback);

  // Done!
  FREYA_LOG_INFO("Successfully initialized the renderer context");
}

void renderer_shutdown() {
  // GFX shutdown

  sfons_destroy(s_renderer.fons);

  sgp_shutdown();
  sgl_shutdown();
  sg_shutdown();

  // Done!
  FREYA_LOG_INFO("Successfully shutdown the renderer context");
}

void renderer_apply_asset_group(const AssetGroupID& group_id) {
  s_renderer.group_id = group_id;
}

void renderer_sumbit_world(EntityWorld* world) {
  s_renderer.world = world;
}

void renderer_set_clear_color(const Color& color) {
  s_renderer.color = color;
}

void renderer_set_gamma(f32 gamma) {
  s_renderer.frame_data.u_gamma = gamma;
}

const Color& renderer_get_clear_color() {
  return s_renderer.color;
}

f32 renderer_get_gamma() {
  return s_renderer.frame_data.u_gamma;
}

sg_swapchain renderer_get_default_swapchain() {
  sg_swapchain swapchain = {};

  IVec2 frame_size = window_get_framebuffer_size(s_renderer.window);

  swapchain.width  = frame_size.x;
  swapchain.height = frame_size.y;

  swapchain.sample_count   = window_get_samples_count(s_renderer.window);
  swapchain.gl.framebuffer = 0; // The default framebuffer in GL is always 0

  return swapchain;
}

AssetGroupID& renderer_get_asset_group_id() {
  return s_renderer.group_id;
}

const RendererDefaults& renderer_get_defaults() {
  return s_renderer.defaults;
}

void renderer_prepare() {
  FREYA_DEBUG_ASSERT(s_renderer.world, "Invalid EntityWorld found in renderer");
  FREYA_PROFILE_FUNCTION();

  EntityWorld* world = s_renderer.world;

  // 
  // Gather the frame data 
  //

  // Update the camera data

  Camera* main_cam = nullptr; 
  {
    auto view = world->view<Camera>();
    for(auto& entt : view) {
      main_cam = &view.get<Camera>(entt);
      break;
    }
  }

  if(main_cam) {
    s_renderer.frame_data.u_view       = main_cam->view;
    s_renderer.frame_data.u_camera_pos = main_cam->position;
    s_renderer.frame_data.u_exposure   = main_cam->exposure;

    s_renderer.frame_data.u_projection = mat4_perspective(main_cam->fov * DEG2RAD, 
                                                          window_get_aspect_ratio(s_renderer.window),
                                                          main_cam->near_plane, 
                                                          main_cam->far_plane);
  }

  // Update the skybox component

  SkyboxComponent* skybox = nullptr;
  {
    auto view = world->view<SkyboxComponent>();
    for(auto& entt : view) {
      skybox = &view.get<SkyboxComponent>(entt);
      break;
    }
  }

  // Push a skybox draw entry if it exists

  if(skybox) {
    s_renderer.frame_data.u_ambient_color = skybox->ambient_color;

    Mesh& skybox_mesh = s_renderer.defaults.skybox_mesh; 
    sg_bindings bind  = {};

    bind.vertex_buffers[0] = skybox_mesh.vertex_buffer;
    bind.index_buffer      = skybox_mesh.index_buffer; 

    bind.views[0]    = skybox->cubemap.view;
    bind.samplers[0] = skybox->cubemap.sampler;

    s_renderer.queues[RENDER_QUEUE_SKYBOX].entries.emplace_back(bind, skybox_mesh.elements_count);
  }

  // Update the directional light

  DirectionalLightComponent* dir_light = nullptr; 
  {
    auto view = world->view<DirectionalLightComponent>();
    for(auto& entt : view) {
      dir_light = &view.get<DirectionalLightComponent>(entt);
      break;
    }
  }

  if(dir_light) {
    s_renderer.dir_light.direction = dir_light->direction;
    s_renderer.dir_light.color     = dir_light->color;
  }

  // Update the point lights 
  {
    auto view            = world->view<PointLightComponent>();
    PointLight_t& lights = s_renderer.point_lights;

    for(auto& entt : view) {
      // Make sure to not exceed the max

      PointLightComponent& point = view.get<PointLightComponent>(entt);
      if(lights.count >= POINT_LIGHTS_MAX) {
        break;
      }

      // Add the point light

      lights.position_r[lights.count] = Vec4(point.position, point.radius); 
      lights.color_fo[lights.count]   = Vec4(point.color, point.fall_off);
      lights.count++;
    }
  }

  // 
  // Fill the render queues 
  //

  // Meshes
  {
    auto view = world->view<MeshComponent>();
    for(auto& entt : view) {
      // Get the important components

      MeshComponent& comp = view.get<MeshComponent>(entt);
      RenderQueue& queue  = s_renderer.queues[RENDER_QUEUE_OPAQUE];

      // Queue the mesh 
      render_queue_mesh(queue, comp);
    }
  }

  //
  // Instanced meshes 
  //
  {
    auto view = world->view<MeshInstancedComponent>();
    for(auto& entt : view) {
      // Get the imporant components

      MeshInstancedComponent& comp = view.get<MeshInstancedComponent>(entt);
      RenderQueue& queue           = s_renderer.queues[RENDER_QUEUE_OPAQUE];

      // Queue the instanced mesh
      render_queue_mesh_instanced(queue, comp);
    }
  }

  // 
  // Models 
  //
  {
    auto view = world->view<ModelComponent>();
    for(auto& entt : view) {
      // Get the imporant components

      ModelComponent& comp = view.get<ModelComponent>(entt);
      RenderQueue& queue   = s_renderer.queues[RENDER_QUEUE_OPAQUE];

      // Queue the model

      for(sizei i = 0; i < comp.model->meshes.size(); i++) {
        // Get the required material index
        i8 mat_index = comp.model->material_remaps[i];

        // Queue the mesh

        MeshComponent mesh_comp = {
          .mesh     = comp.model->meshes[i], 
          .material = comp.model->materials[mat_index],

          .transform = comp.transform,
        };
        render_queue_mesh(queue, mesh_comp);
      }
    }
  }

  // 
  // Billboard
  //
  {
    auto view = world->view<BillboardComponent>();
    for(auto& entt : view) {
      // Retrieve the imporant components

      BillboardComponent& comp = view.get<BillboardComponent>(entt);
      RenderQueue& billboard   = s_renderer.queues[RENDER_QUEUE_BILLBOARD];

      RenderQueueEntry entry;

      // Setup the bindings 

      Mesh& mesh = s_renderer.defaults.billboard_mesh;

      entry.bindings.vertex_buffers[0] = mesh.vertex_buffer; 
      entry.bindings.index_buffer      = mesh.index_buffer; 

      entry.bindings.views[0] = billboard.transforms_view;
      entry.bindings.views[1] = comp.texture.view; 

      entry.bindings.samplers[0] = comp.texture.sampler;
      
      // Fill the rest of the entry
      
      entry.model.u_index = (i32)billboard.entries.size();
      
      entry.elements_count  = mesh.elements_count;
      entry.instances_count = 1;
     
      // Pushing into the queue
      
      billboard.transforms.emplace_back(comp.transform);
      billboard.entries.push_back(entry);
    }
  }

  // 
  // Fill the screen-space queue 
  //
  {
    RenderPass& current_pass = s_renderer.passes[RENDER_PASS_DEFAULT - 1];
    RenderQueue& queue       = s_renderer.queues[RENDER_QUEUE_SCREEN_SPACE];

    // Use the final texture image of the current pass

    sg_bindings bind = {};

    bind.views[0] = current_pass.attachments[0];

    bind.vertex_buffers[0] = s_renderer.defaults.quad_mesh.vertex_buffer;
    bind.index_buffer      = s_renderer.defaults.quad_mesh.index_buffer;
    
    bind.samplers[0] = current_pass.sampler;

    // Pushing into the queue
    queue.entries.emplace_back(bind, 6);
  } 

  // Prepare any passes
  // @TODO (Renderer/shadow pass)
  // shadow_pass_prepare(s_renderer.passes[RENDER_PASS_SHADOW], *main_cam, *dir_light, s_renderer.frame_data.u_light_space);
}

void renderer_commit() {
  FREYA_PROFILE_FUNCTION();

  //
  // Commit all of the passes in order
  // 

  // @TODO: Shadow...
  // shadow_pass_commit(s_renderer.passes[RENDER_PASS_SHADOW], s_renderer.queues[RENDER_QUEUE_SHADOW], s_renderer.frame_data);
 
  // Geometry...

  GeometryPassData geo_data = {
    .skybox_queue    = &s_renderer.queues[RENDER_QUEUE_SKYBOX],
    .opaque_queue    = &s_renderer.queues[RENDER_QUEUE_OPAQUE],
    .billboard_queue = &s_renderer.queues[RENDER_QUEUE_BILLBOARD],

    .frame_data = s_renderer.frame_data, 

    .point_lights = s_renderer.point_lights, 
    .dir_light    = s_renderer.dir_light,
  };
  geometry_pass_commit(s_renderer.passes[RENDER_PASS_GEOMETRY], geo_data);
  
  // Default...
  
  default_pass_commit(s_renderer.passes[RENDER_PASS_DEFAULT], 
                      s_renderer.queues[RENDER_QUEUE_SCREEN_SPACE], 
                      s_renderer.window, 
                      s_renderer.frame_data);

  // Done with this frame... 
  sg_commit();

  // Clean the slate

  for(auto& queue : s_renderer.queues) {
    queue.transforms.clear();
    queue.entries.clear();
  }

  // Clear any shader states 

  s_renderer.dir_light.direction = Vec3(0.0f);
  s_renderer.dir_light.color     = Vec3(1.0f);

  s_renderer.frame_data.u_view       = Mat4(1.0f);
  s_renderer.frame_data.u_projection = Mat4(1.0f);
  s_renderer.frame_data.u_camera_pos = Vec3(0.0f);
  s_renderer.frame_data.u_exposure   = 1.0f;

  s_renderer.point_lights.count = 0;
  
  s_renderer.frame_data.u_ambient_color = Vec3(1.0f);
}

void* renderer_get_font_context() {
  return s_renderer.fons;
}

/// Renderer functions
///---------------------------------------------------------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
