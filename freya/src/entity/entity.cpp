#include "freya_entity.h"
#include "freya_event.h"
#include "freya_logger.h"
#include "freya_render.h"
#include "freya_input.h"
#include "freya_math.h"
#include "freya_gui.h"

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
  
  // CameraController
  {
    FREYA_PROFILE_FUNCTION_NAMED("entity_world_update(CameraController)");

    // Do not update the controllers when the GUI is active
    // @TODO (Camera): This is obviously terrible

    if(!gui_is_active()) {
      auto view = world.view<CameraController, Camera>();
      for(auto entt : view) {
        // Retrieve all the required components

        CameraController& controller = view.get<CameraController>(entt);
        Camera& camera               = view.get<Camera>(entt);

        // Move the controller according to its type 

        switch(controller.type) {
          case CAMERA_CONTROLLER_FREE:
            camera_controller_free(controller, camera, delta_time);
            break;
          case CAMERA_CONTROLLER_FPS:
            camera_controller_fps(controller, camera, delta_time);
            break;
        }
      }
    }
  }
  
  // Camera
  {
    FREYA_PROFILE_FUNCTION_NAMED("entity_world_update(Camera)");

    auto view = world.view<Camera>();
    for(auto entt : view) {
      // Update the view matrix
      
      Camera& camera = view.get<Camera>(entt);

      camera.transform = mat4_translate(camera.position) * quat_to_mat4(camera.rotation);
      camera.view      = mat4_inverse(camera.transform);
    }
  }
  
  // Animator
  {
    FREYA_PROFILE_FUNCTION_NAMED("entity_world_update(Animator)");

    auto view = world.view<Animator, BillboardComponent>();
    for(auto entt : view) {
      Animator& anim = view.get<Animator>(entt);
      animator_update(anim, delta_time);
      
      BillboardComponent& bill = view.get<BillboardComponent>(entt);
      bill.texture             = *anim.current;
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

Camera& entity_add_camera(EntityWorld& world, EntityID& entt, const CameraDesc& desc) {
  Transform& transform = world.get<Transform>(entt);
  Camera& camera       = world.emplace<Camera>(entt); 
  
  camera.fov = desc.fov;

  camera.near_plane = desc.near_plane;
  camera.far_plane  = desc.far_plane;

  camera.position = transform.position;
  camera.rotation = transform.rotation;

  camera.view = Mat4(1.0f);

  return camera;
}

CameraController& entity_add_camera_controller(EntityWorld& world, 
                                               EntityID& entt, 
                                                CameraControllerType type,
                                               f32 speed, 
                                               f32 sensitivity) {
  return world.emplace<CameraController>(entt, type, -90.0f, 0.0f, sensitivity, speed, Vec3(0.0f), Vec3(0.0f, 1.0f, 0.0f));
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

Animator& entity_add_animator(EntityWorld& world, EntityID& entt, const AnimatorDesc& desc) {
  Animator anim;
  animator_create(anim, desc);

  return world.emplace<Animator>(entt, anim);
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

/// EntityID functions
/// ----------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
