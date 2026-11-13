#include "freya_gui.h"
#include "freya_logger.h"
#include "freya_memory.h"
#include "freya_input.h"
#include "freya_event.h"
#include "freya_render.h"

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_opengl3_loader.h>

#include <glm/gtc/quaternion.hpp>

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

///---------------------------------------------------------------------------------------------------------------------
/// GUIState
struct GUIState {
  Window* window          = nullptr; 
  GLFWwindow* glfw_window = nullptr;

  ImGuiIO io_config;
  GUIWindowFlags window_flags;
 
  HashMap<String, Vec3> rotations; // @TODO (GUI): Ew!

  f32 big_step   = 0.01f; 
  f32 small_step = 0.001f; 

  bool is_active = false;
};

static GUIState s_gui;
/// GUIState
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// Callbacks

static bool on_keyboard_action(const Event& event, const void* dispatcher, const void* listener) {
  if(!s_gui.is_active) {
    return true;
  }

  switch(event.type) {
    case EVENT_KEY_PRESSED:
      if(event.key_pressed == KEY_LEFT_SHIFT) {
        s_gui.big_step   = 0.1f;
        s_gui.small_step = 0.01f;
      }
      break;
    case EVENT_KEY_RELEASED:
      if(event.key_released == KEY_LEFT_SHIFT) {
        s_gui.big_step   = 0.01f;
        s_gui.small_step = 0.001f;
      }
      break;
  }

  return true; 
}

static bool on_mouse_action(const Event& event, const void* dispatcher, const void* listener) {
  if(!s_gui.is_active) {
    return true;
  }

  switch(event.type) {
    case EVENT_MOUSE_BUTTON_PRESSED:
      input_cursor_show(!(event.mouse_button_pressed == MOUSE_BUTTON_LEFT));
      break;
    case EVENT_MOUSE_BUTTON_RELEASED:
      input_cursor_show((event.mouse_button_released == MOUSE_BUTTON_LEFT));
      break;
  }

  return true; 
}

/// Callbacks
///---------------------------------------------------------------------------------------------------------------------

///---------------------------------------------------------------------------------------------------------------------
/// GUI functions

bool gui_init(Window* window) {
  // Setting default values for the GUI
  
  s_gui        = GUIState{};
  s_gui.window = window;

  // Set up ImGui context
  
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();

  // Setting context flags
  
  io.ConfigFlags     = ImGuiConfigFlags_DockingEnable;
  s_gui.window_flags = GUI_WINDOW_FLAGS_NONE;

  // Dark mode WOOOOOOOAH! 
  ImGui::StyleColorsDark();

  // Setting up the glfw backend
 
  s_gui.glfw_window = (GLFWwindow*)window_get_handle(window);

  if(!ImGui_ImplGlfw_InitForOpenGL(s_gui.glfw_window, true)) {
    FREYA_LOG_ERROR("Failed to initialize GLFW for ImGui");
    return false;
  }
  
  // Setting up the opengl backend

#if FREYA_PLATFORM_WEB == 1
  String glsl_version = "#version 300 es";
#else
  String glsl_version = "#version 460 core";
#endif

  if(!ImGui_ImplOpenGL3_Init(glsl_version.c_str())) {
    FREYA_LOG_ERROR("Failed to initialize OpenGL for ImGui");
    return false;
  }

  // Listen to events
  
  event_register(EVENT_KEY_PRESSED, on_keyboard_action);
  event_register(EVENT_KEY_RELEASED, on_keyboard_action);
  
  event_register(EVENT_MOUSE_BUTTON_PRESSED, on_mouse_action);
  event_register(EVENT_MOUSE_BUTTON_RELEASED, on_mouse_action);

  return true;
}

void gui_shutdown() {
  ImGui_ImplGlfw_Shutdown();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui::DestroyContext();
}

void gui_begin() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void gui_end() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void gui_set_window_flags(const i32 flags) {
  s_gui.window_flags = (GUIWindowFlags)flags;
}

bool gui_begin_panel(const char* name) {
  return ImGui::Begin(name, nullptr, (ImGuiWindowFlags)s_gui.window_flags);
}

void gui_end_panel() {
  ImGui::End();
}

void gui_toggle_active() {
  s_gui.is_active = !s_gui.is_active;
}

const bool gui_is_focused() {
  return ImGui::GetIO().WantCaptureMouse;
}

const bool gui_is_active() {
  return s_gui.is_active;
}

void gui_renderer_info() {
  if(!gui_begin_panel("Renderer Info")) {
    gui_end_panel();
    return;
  }
 
  // Clear color
 
  Vec4 clear_color = renderer_get_clear_color();
  bool is_picked   = ImGui::ColorPicker4("Clear color", &clear_color[0], ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);

  if(is_picked) {
    renderer_set_clear_color(clear_color);
  }

  // Gamma 

  f32 gamma = renderer_get_gamma();
  if(ImGui::DragFloat("Gamma", &gamma, s_gui.small_step, 1.0f, 2.6f)) {
    renderer_set_gamma(gamma);
  }

  gui_end_panel();
}

void gui_window_info() {
  if(!gui_begin_panel("Window")) {
    gui_end_panel();
    return;
  }
 
  // Title
 
  String window_title = window_get_title(s_gui.window);
  if(ImGui::InputText("Title", &window_title)) {
    window_set_title(s_gui.window, window_title.c_str());
  }
 
  // Size 
  
  IVec2 window_size = window_get_size(s_gui.window);
  if(ImGui::SliderInt2("Size", &window_size[0], 0, 1920)) {
    window_set_size(s_gui.window, window_size);
  }

  // Position 
  
  IVec2 window_pos = window_get_position(s_gui.window);
  if(ImGui::SliderInt2("Position", &window_pos[0], 0, 1920)) {
    window_set_position(s_gui.window, window_pos);
  }

  // Fullscreen 

  bool is_fullscreen = window_is_fullscreen(s_gui.window);
  if(ImGui::Checkbox("Fullscreen", &is_fullscreen)) {
    window_set_fullscreen(s_gui.window, is_fullscreen);
  }

  gui_end_panel();
}

void gui_debug_info() {
  if(!gui_begin_panel("Debug Info")) {
    gui_end_panel();
    return;
  }

  // FPS 
  {
    ImGui::Text("FPS: %.3lf", clock_get_fps());
    ImGui::Separator();
  }

  // Mouse
  { 
    if(ImGui::CollapsingHeader("Mouse")) {
      Vec2 mouse_pos    = input_mouse_position();
      Vec2 mouse_offset = input_mouse_offset();

      ImGui::Text("Position: %s", vec2_to_string(mouse_pos).c_str());
      ImGui::Text("Offset: %s", vec2_to_string(mouse_offset).c_str());
    } 
  }

  // Gamepad
  {
    if(ImGui::CollapsingHeader("Gamepads")) {
      for(sizei i = JOYSTICK_ID_0; i < JOYSTICK_ID_LAST; i++) {
        JoystickID joy_id = (JoystickID)i;

        if(!input_gamepad_connected(joy_id)) {
          continue;
        }

        // Name 

        String name = ("Gamepad" + String(input_gamepad_get_name(joy_id)));
        ImGui::SeparatorText(name.c_str());

        // Axises values

        Vec2 left_axis = input_gamepad_axis_value(joy_id, GAMEPAD_AXIS_LEFT); 
        ImGui::Text("Left axis: X = %0.3f, Y = %0.3f", left_axis.x, left_axis.y);

        Vec2 right_axis = input_gamepad_axis_value(joy_id, GAMEPAD_AXIS_RIGHT); 
        ImGui::Text("Right axis: X = %0.3f, Y = %0.3f", right_axis.x, right_axis.y);

        Vec2 triggers = input_gamepad_axis_value(joy_id, GAMEPAD_AXIS_TRIGGER); 
        ImGui::Text("Trigger: Left = %0.3f, Right = %0.3f", triggers.x, triggers.y);
      }
    }
  }

  // Memory
  { 
    if(ImGui::CollapsingHeader("Memory")) {
      ImGui::Text("Allocations: %zu", memory_get_allocations_count());
      ImGui::Text("Frees: %zu", memory_get_frees_count());

      sizei mbytes = memory_get_allocation_bytes() / MiB(1);
      ImGui::Text("Bytes allocated: %zuMiB", mbytes);
    } 
  }

  // Renderer
  {
    if(ImGui::CollapsingHeader("Renderer")) {
      // Clear color

      Vec4 clear_color = renderer_get_clear_color();
      bool is_picked   = ImGui::ColorEdit4("Clear color", &clear_color[0]);

      if(is_picked) {
        renderer_set_clear_color(clear_color);
      }

      // Gamma 

      f32 gamma = renderer_get_gamma();
      if(ImGui::DragFloat("Gamma", &gamma, s_gui.small_step, 1.0f, 2.6f)) {
        renderer_set_gamma(gamma);
      }
    }
  }

  gui_end_panel();
}

void gui_edit_color(const char* name, Vec4& color) {
  ImGui::SeparatorText(name); 
  ImGui::PushID(name); 

  ImGui::ColorEdit4(name, &color[0]);

  ImGui::PopID();
}

void gui_edit_transform(const char* name, Transform* transform) {
  // @TODO (GUI): Will remove this in the future... do not pay attention to it...

  if(s_gui.rotations.find(name) == s_gui.rotations.end()) {
    s_gui.rotations[name] = Vec3(0.0f);
  }

  ImGui::SeparatorText(name); 
  ImGui::PushID(name); 

  ///
  // SRT translation
  // 
 
  Vec3 position = transform->_position;
  if(ImGui::DragFloat3("Position", &position[0], s_gui.big_step)) {
    transform_translate(*transform, position);
  }

  Vec3 scale = transform->_scale;
  if(ImGui::DragFloat3("Scale", &scale[0], s_gui.big_step)) {
    transform_scale(*transform, scale);
  }

  if(ImGui::DragFloat3("Rotation", &s_gui.rotations[name][0], s_gui.big_step)) {
    Vec3 axis = vec3_normalize(s_gui.rotations[name]);
    transform_rotate(*transform, axis, s_gui.rotations[name].x);
  }
  
  ImGui::PopID(); 
}

void gui_edit_camera(const char* name, Camera* camera) {
  ImGui::SeparatorText(name); 
  ImGui::PushID(name); 

  // Editables
  
  ImGui::Text("Position: X = %0.3f, Y = %0.3f, Z = %0.3f", camera->position.x, camera->position.y, camera->position.z);
 
  ImGui::SliderFloat("FOV", &camera->fov, CAMERA_MAX_ZOOM, 0.0f, "FOV: %.3f");
  ImGui::SliderFloat("Exposure", &camera->exposure, -1.0f, 1.0f);
  ImGui::SliderFloat("Near", &camera->near_plane, 0.0001f, 1000.0f, "Near: %.3f");
  ImGui::SliderFloat("Far", &camera->far_plane, 0.0001f, 1000.0f, "Far: %.3f");
  
  ImGui::PopID(); 
}

void gui_edit_audio_source(const char* name, AudioSourceID& source) {
  ImGui::SeparatorText(name); 
  ImGui::PushID(name); 

  AudioSourceDesc source_desc = audio_source_get_desc(source);

  //
  // Source Info
  // 

  // Volume
  
  if(ImGui::SliderFloat("Volume", &source_desc.volume, 0.0f, 1.0f)) {
    audio_source_set_volume(source, source_desc.volume);
  }

  // Pitch
  
  if(ImGui::SliderFloat("Pitch", &source_desc.pitch, 0.0f, 1.0f)) {
    audio_source_set_pitch(source, source_desc.pitch);
  }

  // Position
  
  if(ImGui::DragFloat2("Position", &source_desc.position[0], s_gui.small_step, -100.0f, 100.0f)) {
    audio_source_set_position(source, source_desc.position);
  }

  // Velocity
  
  if(ImGui::DragFloat2("Velocity", &source_desc.velocity[0], s_gui.small_step, -1.0f, 1.0f)) {
    audio_source_set_velocity(source, source_desc.velocity);
  }
  
  // Direction
  
  if(ImGui::DragFloat2("Direction", &source_desc.direction[0], s_gui.small_step, -1.0f, 1.0f)) {
    audio_source_set_direction(source, source_desc.direction);
  }
  
  // Looping
  
  if(ImGui::Checkbox("Looping", &source_desc.is_looping)) {
    audio_source_set_looping(source, source_desc.is_looping);
  }

  // Queued buffers 
  ImGui::Text("Current queued buffers %zu", source_desc.buffers_count);

  //
  // Command buttons
  // 

  // Play button
  
  if(ImGui::Button("Play")) {
    audio_source_start(source);
  } 

  ImGui::SameLine();
  
  // Pause button
  
  if(ImGui::Button("Pause")) {
    audio_source_pause(source);
  } 
  
  ImGui::SameLine();
  
  // Stop button
  
  if(ImGui::Button("Stop")) {
    audio_source_stop(source);
  } 
  
  ImGui::SameLine();

  // Restart button
  
  if(ImGui::Button("Restart")) {
    audio_source_restart(source);
    audio_source_start(source);
  } 
  
  ImGui::SameLine();

  ImGui::NewLine();
  ImGui::PopID(); 
}

void gui_edit_audio_listener(const char* name) {
  ImGui::SeparatorText(name); 
  ImGui::PushID(name); 

  AudioListenerDesc listener = audio_listener_get_desc();

  // Volume
  
  if(ImGui::SliderFloat("Volume", &listener.volume, 0.0f, 1.0f)) {
    audio_listener_set_volume(listener.volume);
  }

  // Position
  
  if(ImGui::DragFloat2("Position", &listener.position[0], s_gui.small_step, -1.0f, 1.0f)) {
    audio_listener_set_position(listener.position);
  }

  // Velocity
  
  if(ImGui::DragFloat2("Velocity", &listener.velocity[0], s_gui.small_step, -1.0f, 1.0f)) {
    audio_listener_set_velocity(listener.velocity);
  }

  ImGui::PopID(); 
}

void gui_edit_timer(const char* name, Timer* timer) {
  ImGui::SeparatorText(name); 
  ImGui::PushID(name); 

  ImGui::DragFloat("Limit", &timer->limit, s_gui.big_step, 0.0f);

  ImGui::Checkbox("One shot", &timer->is_one_shot);
  ImGui::Checkbox("Active", &timer->is_active);
  
  ImGui::PopID(); 
}

void gui_edit_directional_light(const char* name, DirectionalLightComponent* light) {
  ImGui::SeparatorText(name); 
  ImGui::PushID(name); 
  
  ImGui::DragFloat3("Direction", &light->direction[0], s_gui.big_step);
  ImGui::ColorEdit3("Color", &light->color[0]);

  ImGui::PopID(); 
}

void gui_edit_point_light(const char* name, PointLightComponent* light) {
  ImGui::SeparatorText(name);
  ImGui::PushID(name); 

  ImGui::DragFloat3("Color", &light->color[0], s_gui.small_step, 0.0f);

  ImGui::SliderFloat("Radius", &light->radius, 0.0f, 32.0f);
  ImGui::SliderFloat("Fall off", &light->fall_off, 0.0f, 32.0f);

  ImGui::PopID(); 
}

void gui_edit_mesh_component(const char* name, MeshComponent* mesh) {
  ImGui::SeparatorText(name); 
  ImGui::PushID(name); 

  //
  // Material 
  //

  Material& material = mesh->material;

  ImGui::SeparatorText("Material");
  ImGui::Text("Name: %s", material.name.c_str());

  ImGui::ColorEdit4("Color", &material.color[0]);

  ImGui::SliderFloat("Roughness intensity", &material.roughness, 0.0f, 1.0f, "Roughness intensity: %.3f");
  ImGui::SliderFloat("Metallic intensity", &material.metallic, 0.01f, 1.0f, "Metallic intensity: %.3f");

  ImGui::Checkbox("Shadows enabled", &material.shadows_enabled);

  ImGui::PopID(); 
}

void gui_edit_skybox_component(const char* name, SkyboxComponent* skybox) {
  ImGui::SeparatorText(name); 
  ImGui::PushID(name); 

  ImGui::SliderFloat3("Ambient color", &skybox->ambient_color[0], 0.0f, 1.0f);

  ImGui::PopID(); 
}

FREYA_API void gui_edit_noise_generator(const char* name, NoiseGenerator* gen) {
  ImGui::SeparatorText(name); 
  ImGui::PushID(name); 

  NoiseGeneratorDesc& desc = noise_generator_get_desc(gen);

  // 
  // Enums
  //
 
  // Noise type

  i32 noise_type              = (i32)desc.noise_type;
  const char* noise_options[] = {"Open simplex 2", "Open simples 2S", "Cellular", "Perlin", "Value cubic", "Value"}; 

  if(ImGui::Combo("Noise type", &noise_type, noise_options, (i32)NOISE_TYPES_COUNT)) {
    desc.noise_type = (NoiseType)noise_type; 
  }

  // Rotation type
  
  i32 rotation_type              = (i32)desc.rotation_type;
  const char* rotation_options[] = {"None", "X-Y planes", "X-Z planes"}; 

  if(ImGui::Combo("Rotation type", &rotation_type, rotation_options, (i32)ROTATION_TYPES_COUNT)) {
    desc.rotation_type = (RotationType)rotation_type; 
  }

  // Fractal type
  
  i32 fractal_type              = (i32)desc.fractal_type;
  const char* fractal_options[] = {"None", "FBm", "Ridged", "Ping pong", "Domain warp (progressive)", "Domain warp (independent)"};

  if(ImGui::Combo("Fractal type", &fractal_type, fractal_options, (i32)FRACTAL_TYPES_COUNT)) {
    desc.fractal_type = (FractalType)fractal_type; 
  }

  // Cellular distance type
  
  i32 distance_type              = (i32)desc.distance_func;
  const char* distance_options[] = {"Euclidean", "Euclidean SQ", "Manhattan", "Hybrid"};

  if(ImGui::Combo("Cellular distance function", &distance_type, distance_options, (i32)CELLULAR_DISTANCE_TYPES_COUNT)) {
    desc.distance_func = (CellularDistanceType)distance_type; 
  }

  // Cellular return type
  
  i32 return_type              = (i32)desc.return_type;
  const char* return_options[] = {"Cell value", 
                                  "Distance", 
                                  "Distance 2", 
                                  "Distance add", 
                                  "Distance sub", 
                                  "Distance mul", 
                                  "Distance div"};

  if(ImGui::Combo("Cellular return type", &return_type, return_options, (i32)CELLULAR_RETURN_TYPES_COUNT)) {
    desc.return_type = (CellularReturnType)return_type; 
  }

  // Domain warp type
  
  i32 warp_type              = (i32)desc.warp_type;
  const char* warp_options[] = {"Open simplex 2", "Open simples 2 reduced", "Basic grid"};

  if(ImGui::Combo("Domain warp type", &warp_type, warp_options, (i32)DOMAIN_WARP_TYPES_COUNT)) {
    desc.warp_type = (DomainWarpType)warp_type; 
  }

  ImGui::NewLine();
  ImGui::NewLine();

  //
  // Fractals
  //

  ImGui::DragInt("Seed", &desc.seed);
  ImGui::DragInt("Octaves", &desc.octaves, s_gui.big_step, 0);

  ImGui::DragFloat("Frequency", &desc.frequency, 0.01f, 0.0f);
  ImGui::DragFloat("Lacunarity", &desc.lacunarity, s_gui.small_step, 0.0f);
  ImGui::DragFloat("Gain", &desc.gain, s_gui.small_step, 0.0f);

  ImGui::DragFloat("Weighted strength", &desc.strength, s_gui.small_step, 0.0f);
  ImGui::DragFloat("Ping pong strength", &desc.ping_pong_strength, s_gui.small_step, 0.0f);

  ImGui::DragFloat("Cellular jitter", &desc.cellular_jitter, s_gui.small_step, 0.0f, 1.0f);
  ImGui::DragFloat("Domain warp amp", &desc.domain_amp, s_gui.small_step, 0.0f);

  // 
  // Buttons  
  //

  if(ImGui::Button("Randomize seed")) {
    desc.seed = random_i32();
  }

  // Set the new values
  noise_generator_set_desc(gen, desc);

  ImGui::PopID(); 
}

void gui_edit_entity(const char* name, EntityWorld& world, EntityID& entt) {
  ImGui::SeparatorText(name); 
  ImGui::PushID(name); 
 
  // Transform
 
  if(ImGui::TreeNode("Transform")) {
    Transform& transform = entity_get_component<Transform>(world, entt);
    gui_edit_transform("", &transform);

    ImGui::TreePop();
  }
 
  // Camera

  if(entity_has_component<Camera>(world, entt)) {
    if(ImGui::TreeNode("Camera")) {
      Camera& cam = entity_get_component<Camera>(world, entt);
      gui_edit_camera("", &cam);

      ImGui::TreePop();
    }
  }

  // Audio 

  if(entity_has_component<AudioSourceID>(world, entt)) {
    if(ImGui::TreeNode("Audio source")) {
      AudioSourceID& source = entity_get_component<AudioSourceID>(world, entt);
      gui_edit_audio_source("", source);

      ImGui::TreePop();
    }
  }
  
  // Timer 

  if(entity_has_component<Timer>(world, entt)) {
    if(ImGui::TreeNode("Timer")) {
      Timer& timer = entity_get_component<Timer>(world, entt);
      gui_edit_timer("", &timer);

      ImGui::TreePop();
    }
  }
  
  // Directional light
 
  if(entity_has_component<DirectionalLightComponent>(world, entt)) {
    if(ImGui::TreeNode("Directional light")) {
      DirectionalLightComponent& light = entity_get_component<DirectionalLightComponent>(world, entt);
      gui_edit_directional_light("", &light);

      ImGui::TreePop();
    }
  }

  // Point light
 
  if(entity_has_component<PointLightComponent>(world, entt)) {
    if(ImGui::TreeNode("Point light")) {
      PointLightComponent& light = entity_get_component<PointLightComponent>(world, entt);
      gui_edit_point_light("", &light);

      ImGui::TreePop();
    }
  }

  // Mesh
 
  if(entity_has_component<MeshComponent>(world, entt)) {
    if(ImGui::TreeNode("Mesh component")) {
      MeshComponent& mesh = entity_get_component<MeshComponent>(world, entt);
      gui_edit_mesh_component("", &mesh);

      ImGui::TreePop();
    }
  }

  // Skybox
 
  if(entity_has_component<SkyboxComponent>(world, entt)) {
    if(ImGui::TreeNode("Skybox component")) {
      SkyboxComponent& skybox = entity_get_component<SkyboxComponent>(world, entt);
      gui_edit_skybox_component("", &skybox);

      ImGui::TreePop();
    }
  }
  
  // Noise generator 

  if(entity_has_component<NoiseGenerator*>(world, entt)) {
    if(ImGui::TreeNode("Noise generator")) {
      NoiseGenerator* gen = entity_get_component<NoiseGenerator*>(world, entt);
      gui_edit_noise_generator("", gen);

      ImGui::TreePop();
    }
  }

  ImGui::PopID(); 
}

void gui_edit_entity_world(const char* name, EntityWorld& world) {
  ImGui::SeparatorText(name); 
  ImGui::PushID(name); 

  auto view = world.view<EntityID>();
  u32 count = 0;

  for(auto& entt : view) {
    EntityID entt_id = entt;
    
    SceneComponent& scene = entity_get_component<SceneComponent>(world, entt_id);
    String name           = scene.tag == "_" ? ("Entity " + std::to_string(count)) : scene.tag;
    
    count++;
    gui_edit_entity(name.c_str(), world, entt_id);
  }

  ImGui::PopID(); 
}

/// Editor functions
///---------------------------------------------------------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
