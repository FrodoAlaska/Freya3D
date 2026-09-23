#include "freya_entity.h"
#include "freya_event.h"
#include "freya_logger.h"
#include "freya_render.h"
#include "freya_input.h"

//////////////////////////////////////////////////////////////////////////

namespace freya { // Start of freya

/// ----------------------------------------------------------------------
/// EntityWorld functions

void entity_world_clear(EntityWorld& world) {
  // Destroy each entity

  auto view = world.view<EntityID>();
  for(auto entt : view) {
    entity_destroy(world, entt);
  }

  // Goodbye, cruel world!
  world.clear();
}

void entity_world_update(EntityWorld& world, const f32 delta_time) {
  FREYA_PROFILE_FUNCTION();
  
  // Timers
  {
    FREYA_PROFILE_FUNCTION_NAMED("entity_world_update(TimerComponent)");

    auto view = world.view<TimerComponent>();
    for(auto entt : view) {
      TimerComponent& comp = view.get<TimerComponent>(entt);
      timer_update(comp.timer, delta_time);

      if(comp.timer.has_runout && comp.runout_func) {
        comp.runout_func(world, entt, comp.user_data);
      }
    }
  }
  
  // Camera
  {
    FREYA_PROFILE_FUNCTION_NAMED("entity_world_update(Camera)");

    auto view = world.view<Camera, Transform>();
    for(auto entt : view) {
      Camera& camera       = view.get<Camera>(entt);
      Transform& transform = view.get<Transform>(entt);
       
      camera.position = transform.position;
      camera_update(camera);
    }
  }
  
  // Point lights
  {
    FREYA_PROFILE_FUNCTION_NAMED("entity_world_update(PointLightComponent)");

    auto view = world.view<PointLightComponent, Transform>();
    for(auto entt : view) {
      PointLightComponent& light = view.get<PointLightComponent>(entt);
      Transform& transform       = view.get<Transform>(entt); 

      light.position = transform.position;
    }
  }
  
  // Mesh
  {
    FREYA_PROFILE_FUNCTION_NAMED("entity_world_update(MeshComponent)");

    auto view = world.view<MeshComponent, Transform>();
    for(auto entt : view) {
      MeshComponent& mesh  = view.get<MeshComponent>(entt);
      Transform& transform = view.get<Transform>(entt); 

      // @TODO (Mesh/better system than this)
      mesh.transform = transform_apply(transform);
    }
  }
}

/// EntityWorld functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// EntityID functions

EntityID entity_create(EntityWorld& world,
                       const Vec3& position, 
                       const Vec3& scale,
                       const Quat& rotation) {
  // Generate an entity ID
  EntityID entt_id = world.create();

  // Add a transform component  

  Transform transform; 
  transform.position = position; 
  transform.scale    = scale;
  transform.rotation = rotation; 

  transform_apply(transform);

  world.emplace<Transform>(entt_id, transform);

  // Dispatch an event

  Event event = {
    .type = EVENT_ENTITY_ADDED, 
    .entt = entt_id,
  };
  event_dispatch(event);

  // Done!
  return entt_id;
}

void entity_destroy(EntityWorld& world, EntityID& entt) {
  // Dispatch an event

  Event event = {
    .type = EVENT_ENTITY_DESTROYED, 
    .entt = entt,
  };
  event_dispatch(event);

  // Destroy any components that require it 

  if(entity_has_component<NoiseGenerator*>(world, entt)) {
    NoiseGenerator* gen = entity_get_component<NoiseGenerator*>(world, entt);
    noise_generator_destroy(gen);
  }

  // Destroy the entity in the world
  world.destroy(entt); 
}

Camera& entity_add_camera(EntityWorld& world, EntityID& entt, CameraDesc& desc) {
  Transform& transform = world.get<Transform>(entt);
  desc.position        = transform.position;

  Camera& camera = world.emplace<Camera>(entt); 
  camera_create(camera, desc);

  return camera;
}

AudioSourceID& entity_add_audio_source(EntityWorld& world, 
                                       EntityID& entt, 
                                       AudioSourceDesc& desc, 
                                       const AssetID& audio_buffer_id) {
  Transform& transform = world.get<Transform>(entt);

  desc.position      = transform.position; 
  desc.buffers[0]    = asset_group_get_audio_buffer(audio_buffer_id);
  desc.buffers_count = 1;

  return world.emplace<AudioSourceID>(entt, audio_source_create(desc));
}

NoiseGenerator* entity_add_noise_generator(EntityWorld& world, EntityID& entt, const NoiseGeneratorDesc& desc) {
  NoiseGenerator* gen = noise_generator_create(desc);
  return world.emplace<NoiseGenerator*>(entt, gen);
}

TagComponent& entity_add_tag(EntityWorld& world, EntityID& entt, const String& tag) {
  return world.emplace<TagComponent>(entt, tag);
}

TimerComponent& entity_add_timer(EntityWorld& world, 
                                 EntityID& entt, 
                                 const TimerDesc& desc, 
                                 const OnTimerRunoutFn& runout_func, 
                                 void* user_data) {
  Timer timer; 
  timer_create(timer, desc);

  return world.emplace<TimerComponent>(entt, timer, runout_func, user_data);
}

DirectionalLightComponent& entity_add_directional_light(EntityWorld& world, 
                                                        EntityID& entt, 
                                                        const Vec3& direction, 
                                                        const Vec3& color) {
  return world.emplace<DirectionalLightComponent>(entt, direction, color); 
}

PointLightComponent& entity_add_point_light(EntityWorld& world, 
                                            EntityID& entt, 
                                            const Vec3& color, 
                                            f32 radius, 
                                            f32 fall_off) {
  Transform& transform = world.get<Transform>(entt);
  return world.emplace<PointLightComponent>(entt, transform.position, color, radius, fall_off); 
}

SkyboxComponent& entity_add_skybox(EntityWorld& world, 
                                   EntityID& entt, 
                                   const AssetID& cubemap_id, 
                                   const Vec3& ambient_color) {
  return world.emplace<SkyboxComponent>(entt, asset_group_get_cubemap(cubemap_id), ambient_color);
}

MeshComponent& entity_add_mesh(EntityWorld& world, 
                               EntityID& entt, 
                               const AssetID& mesh_id, 
                               const AssetID& material_id) {
  Transform& transform        = world.get<Transform>(entt);
  const Material& default_mat = renderer_get_defaults().material; 

  return world.emplace<MeshComponent>(entt, 
                                      asset_group_get_mesh(mesh_id), 
                                      (material_id.is_valid() ? asset_group_get_material(material_id) : default_mat), 
                                      transform_apply(transform));
}

MeshInstancedComponent& entity_add_mesh_instanced(EntityWorld& world, 
                                                  EntityID& entt, 
                                                  const AssetID& mesh_id, 
                                                  const AssetID& material_id) {
  const Material& default_mat = renderer_get_defaults().material; 
  return world.emplace<MeshInstancedComponent>(
                                      entt, 
                                      asset_group_get_mesh(mesh_id), 
                                      (material_id.is_valid() ? asset_group_get_material(material_id) : default_mat));
}

BillboardComponent& entity_add_billboard(EntityWorld& world, 
                                         EntityID& entt, 
                                         const AssetID& texture_id) {
  Transform& transform = world.get<Transform>(entt);
  
  // Get the correct texture

  Texture texture = {};
  if(texture_id.get_id() != ASSET_ID_INVALID) {
    texture = asset_group_get_texture(texture_id);
  }
  
  // Done!
  return world.emplace<BillboardComponent>(entt, texture, transform_apply(transform));
}

void entity_camera_free_move(EntityWorld& world, EntityID& entt, f32 speed) {
  // Retrive the relevant components

  Camera& camera       = world.get<Camera>(entt);
  Transform& transform = world.get<Transform>(entt);
  
  Vec2 mouse_offset = input_mouse_offset(); 

  // Calculate the inner variables
  
  camera.yaw   = mouse_offset.x * camera.sensitivity;
  camera.pitch = mouse_offset.y * camera.sensitivity;

  camera.pitch = clamp_float(camera.pitch, -CAMERA_MAX_DEGREES, CAMERA_MAX_DEGREES);
  camera.zoom  = clamp_float(camera.zoom, 1.0f, CAMERA_MAX_ZOOM);

  // Take input
  
  if(input_key_down(KEY_W)) {
    transform.position += camera.front * speed;
  }
  else if(input_key_down(KEY_S)) {
    transform.position -= camera.front * speed;
  }
 
  if(input_key_down(KEY_D)) {
    transform.position += vec3_normalize(vec3_cross(camera.front, camera.up)) * speed;
  }
  else if(input_key_down(KEY_A)) {
    transform.position -= vec3_normalize(vec3_cross(camera.front, camera.up)) * speed;
  }
}

void entity_camera_fps_move(EntityWorld& world, EntityID& entt, f32 speed) {
  // Retrive the relevant components

  Camera& camera       = world.get<Camera>(entt);
  Transform& transform = world.get<Transform>(entt);
  
  Vec2 mouse_offset = input_mouse_offset(); 

  // Calculate the inner variables
  
  camera.yaw   = mouse_offset.x * camera.sensitivity;
  camera.pitch = mouse_offset.y * camera.sensitivity;

  camera.pitch = clamp_float(camera.pitch, -CAMERA_MAX_DEGREES, CAMERA_MAX_DEGREES);
  camera.zoom  = clamp_float(camera.zoom, 1.0f, CAMERA_MAX_ZOOM);

  // Take input 

  if(input_key_down(KEY_W)) {
    transform.position += Vec3(camera.front.x, 0.0f, camera.front.z) * speed;
  }
  else if(input_key_down(KEY_S)) {
    transform.position -= Vec3(camera.front.x, 0.0f, camera.front.z) * speed;
  }
 
  if(input_key_down(KEY_A)) {
    transform.position -= vec3_normalize(vec3_cross(camera.front, camera.up)) * speed;
  }
  else if(input_key_down(KEY_D)) {
    transform.position += vec3_normalize(vec3_cross(camera.front, camera.up)) * speed;
  }
}

/// EntityID functions
/// ----------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
