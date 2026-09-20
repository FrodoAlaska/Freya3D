#include "freya_render.h"
#include "freya_logger.h"
#include "freya_event.h"
#include "freya_entity.h"

#include "shaders/default_pass_shader.h"

#include "fontstash/fontstash.h"

#include "sokol/sokol_gp.h"
#include "sokol/sokol_gl.h"
#include "sokol/sokol_fontstash.h"

#include "shaders/main_shader.h"
#include "shaders/screen_space_shader.h"
#include "shaders/skybox_shader.h"

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
/// ShaderID
enum ShaderID {
  SHADER_SHADOW,
  SHADER_LIGHTING,
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

  /// Per-frame state

  Window* window = nullptr;
  Color color    = Color(1.0f);
  
  AssetGroupID group_id = ASSET_CACHE_ID;
  EntityWorld* world    = nullptr;

  Camera* main_cam = nullptr;
  bool can_sort    = false;

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
      asset_group_push_texture(s_renderer.group_id, MATERIAL_TEXTURE_DIFFUSE),
      asset_group_push_texture(s_renderer.group_id, MATERIAL_TEXTURE_SPECULAR),
      asset_group_push_texture(s_renderer.group_id, MATERIAL_TEXTURE_NORMAL),
      asset_group_push_texture(s_renderer.group_id, MATERIAL_TEXTURE_EMISSIVE),
    };

    s_renderer.defaults.diffuse_map  = asset_group_get_texture(texture_ids[0]);
    s_renderer.defaults.specular_map = asset_group_get_texture(texture_ids[1]);
    s_renderer.defaults.normal_map   = asset_group_get_texture(texture_ids[2]);
    s_renderer.defaults.emissive_map = asset_group_get_texture(texture_ids[3]);

    // Material init

    MaterialDesc mat_desc = {
      .diffuse_id  = texture_ids[0], 
      .specular_id = texture_ids[1], 
      .normal_id   = texture_ids[2], 
      .emissive_id = texture_ids[3], 
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

    s_renderer.defaults.quad_mesh = asset_group_get_mesh(asset_group_push_mesh(s_renderer.group_id, vert_desc, idx_desc, 6));
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

    s_renderer.defaults.skybox_mesh = asset_group_get_mesh(asset_group_push_mesh(s_renderer.group_id, vert_desc, idx_desc, 36));
  }

  // Sampler init 
  s_renderer.defaults.sampler = sg_make_sampler(sg_sampler_desc{});
}

static void init_shaders() {
  sg_backend backend = sg_query_backend();

  const sg_shader_desc* shader_descs[SHADERS_COUNT] = {
    shadow_pass_shader_desc(backend),
    lighting_pass_shader_desc(backend),
    screen_space_shader_desc(backend),
    sk_skybox_shader_desc(backend),
  };

  for(sizei i = 0; i < s_renderer.shaders.size(); i++) {
    s_renderer.shaders[i] = asset_group_push_shader(s_renderer.group_id, *shader_descs[i]);
  }
}

/// Private functions
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Renderer functions

void renderer_init(Window* window) {
  s_renderer.window = window;

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

void renderer_set_sort(bool sort) {
  s_renderer.can_sort = sort;
}

const Color& renderer_get_clear_color() {
  return s_renderer.color;
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

  // Reset the renderer's state 
  
  EntityWorld* world = s_renderer.world;

  IVec2 frame_size = window_get_framebuffer_size(s_renderer.window);
  IVec2 size       = window_get_size(s_renderer.window);

  // 
  // Gather the frame data 
  //

  // Update the camera data
  {
    auto view = world->view<Camera>();
    for(auto& entt : view) {
      s_renderer.main_cam = &view.get<Camera>(entt);
      break;
    }
  }

  // Projecting the scene using the camera if it was found 

  if(s_renderer.main_cam) {
    // @TODO (Renderer/camera)
  }

  // 
  // Prepare the frame 
  //
  // @TODO (Renderer/draw)
}

void renderer_commit() {
  FREYA_PROFILE_FUNCTION();
 
  // @TODO (Renderer/commit)
  
  // Done with this frame... 
  sg_commit();

  // Clean the slate
  s_renderer.main_cam = nullptr;
}

void* renderer_get_font_context() {
  return s_renderer.fons;
}

/// Renderer functions
///---------------------------------------------------------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
