#include "app.h"

#include <freya.h>
#include <imgui.h>

/// ----------------------------------------------------------------------
/// App
struct App {
  freya::Window* window;
  freya::AssetGroupID group_id;

  freya::EntityWorld world; 
  freya::EntityID camera_entt;
};

static App s_app;
/// App
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// App functions 

bool app_init(const freya::Args& args, freya::Window* window) {
  // Renderer init
  
  freya::renderer_set_clear_color(freya::Vec4(0.1f, 0.1f, 0.1f, 1.0f));
  freya::renderer_sumbit_world(&s_app.world);

  // Window init
  
  s_app.window = window;
  freya::input_cursor_show(false);

  // Editor init
  freya::gui_init(window);

  // Assets init
  
  s_app.group_id = freya::asset_group_create("app_assets");
  
  freya::asset_group_build(s_app.group_id, "../../assets/assets_list.lua", "assets.frpkg");
  freya::asset_group_load_package(s_app.group_id, "assets.frpkg");

  // Mesh asset init
  freya::AssetID mesh_id = freya::asset_group_push_mesh(s_app.group_id, freya::MESH_CUBE);

  // Material asset init

  freya::MaterialDesc mat_desc = {
    .name = "entity_material", 

    .diffuse_id  = freya::asset_group_get_id(s_app.group_id, "container_diffuse"),
    .specular_id = freya::asset_group_get_id(s_app.group_id, "container_specular"),

    .specular = 32.0f,
  };
  freya::AssetID material_id = freya::asset_group_push_material(s_app.group_id, mat_desc);

  // Camera entity init

  s_app.camera_entt = freya::entity_create(s_app.world, freya::Vec3(10.0f, 0.0f, 10.0f));

  freya::CameraDesc cam_desc = {
    .aspect_ratio = freya::window_get_aspect_ratio(window),
  };
  freya::entity_add_camera(s_app.world, s_app.camera_entt, cam_desc);

  freya::entity_add_directional_light(s_app.world, s_app.camera_entt, freya::Vec3(1.0f, -1.0f, 1.0f));
  freya::entity_add_skybox(s_app.world, s_app.camera_entt, freya::asset_group_get_id(s_app.group_id, "corona"));

  // Point light init

  freya::EntityID point_entt = freya::entity_create(s_app.world, freya::Vec3(10.0f, 0.0f, 10.0f));
  freya::entity_add_point_light(s_app.world, point_entt, freya::COLOR_RED);

  // Mesh 1 init

  freya::EntityID mesh_entt = freya::entity_create(s_app.world, freya::Vec3(10.0f, 0.0f, 10.0f)); 
  freya::entity_add_mesh(s_app.world, mesh_entt, mesh_id, material_id);

  // Mesh 2 init

  freya::EntityID mesh_entt2 = freya::entity_create(s_app.world, freya::Vec3(10.0f, 3.0f, 10.0f)); 
  freya::entity_add_mesh(s_app.world, mesh_entt2, mesh_id, material_id);

  // Plane init

  mesh_id = freya::asset_group_push_mesh(s_app.group_id, freya::MESH_CUBE);
  
  mat_desc = {
    .name = "plane_material", 

    .diffuse_id = freya::asset_group_get_id(s_app.group_id, "brickwall"),
    .normal_id  = freya::asset_group_get_id(s_app.group_id, "brickwall_normal"),

    .specular = 32.0f,
  };
  material_id = freya::asset_group_push_material(s_app.group_id, mat_desc);
  
  freya::EntityID plane_entt = freya::entity_create(s_app.world, freya::Vec3(10.0f, -1.0f, 10.0f), freya::Vec3(12.0f, 0.1f, 12.0f)); 
  freya::entity_add_mesh(s_app.world, plane_entt, mesh_id, material_id);

  // Billboard init

  freya::EntityID bill_entt = freya::entity_create(s_app.world, freya::Vec3(10.0f, 3.0f, 10.0f));
  freya::entity_add_billboard(s_app.world, bill_entt, freya::asset_group_get_id(s_app.group_id, "frogmon_stand"));

  freya::entity_add_tag(s_app.world, bill_entt, "Billboard");

  // Done!
  return true;
}

void app_shutdown() {
  freya::entity_world_clear(s_app.world);
  freya::asset_group_destroy(s_app.group_id);
  
  freya::gui_shutdown();
}

void app_update(freya::f32 dt) {
  // Quit the application when the specified exit key is pressed
  
  if(freya::input_key_pressed(freya::KEY_ESCAPE)) {
    freya::event_dispatch(freya::Event{.type = freya::EVENT_APP_QUIT});
    return;
  }

  // Enable/disable debug mode

  if(freya::input_key_pressed(freya::KEY_F1)) {
    freya::gui_toggle_active();
    freya::input_cursor_show(freya::gui_is_active());
  }

  // Move camera

  if(!freya::gui_is_active()) {
    freya::entity_camera_free_move(s_app.world, s_app.camera_entt, 20.0f * dt);
  }

  // Update
  freya::entity_world_update(s_app.world, dt);
}

void app_render_gui() {
  if(!freya::gui_is_active()) {
    return;
  }

  freya::gui_begin(); 
 
  // Debug
  freya::gui_debug_info();

  // Editor
   
  freya::gui_begin_panel("Editor");
  freya::gui_edit_entity_world("Main", s_app.world);
  freya::gui_end_panel();

  freya::gui_end();
}

/// App functions 
/// ----------------------------------------------------------------------
