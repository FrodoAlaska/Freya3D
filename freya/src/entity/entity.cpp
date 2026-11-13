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

  // SceneComponent
  {
    FREYA_PROFILE_FUNCTION_NAMED("entity_world_update(SceneComponent)");

    auto view = world.view<SceneComponent, Transform>();
    for(auto entt : view) {
      // Get the required components

      SceneComponent& scene = view.get<SceneComponent>(entt); 
      Transform& transform  = view.get<Transform>(entt);
      
      bool can_update = false;
      bool has_parent = false;

      // Decide when exactly to update the transform and how to update it 
      // depending if the entity has a parent or not

      if(scene.parent != ENTITY_NULL) {
        Transform& parent_transform = view.get<Transform>(scene.parent);

        can_update = (parent_transform._is_dirty || transform._is_dirty);
        has_parent = true;
      }
      else {
        can_update = transform._is_dirty;
      }

      // Update the transform if it's needed

      if(!can_update) {
        continue;
      }

      if(has_parent) {
        scene.transform = world.get<SceneComponent>(scene.parent).transform * transform_apply(transform);
      }
      else {
        scene.transform = transform_apply(transform);
      }
    }
  } 
  
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

      light.position = transform._position;
    }
  }
}

/// EntityWorld functions
/// ----------------------------------------------------------------------

/// ----------------------------------------------------------------------
/// EntityID functions

EntityID entity_create(EntityWorld& world, const EntityDesc& desc) {
  // Generate an entity ID
  EntityID entt_id = world.create();

  // Add a transform component  

  Transform& transform = entity_add_component<Transform>(world, entt_id); 
  transform_translate(transform, desc.position); 
  transform_rotate(transform, desc.rotation); 
  transform_scale(transform, desc.scale);

  // Add the main scene component 
  
  SceneComponent& scene = entity_add_component<SceneComponent>(world, entt_id);
  scene.tag             = desc.tag; 
  scene.parent          = desc.parent;

  if(scene.parent == ENTITY_NULL) {
    scene.transform = transform_apply(transform);
  }
  else {
    scene.transform = world.get<SceneComponent>(scene.parent).transform * scene.transform;
  }

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

  camera.position = transform._position;
  camera.rotation = transform._rotation;
  
  camera_create(camera, desc);
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

  desc.position      = transform._position; 
  desc.buffers[0]    = asset_group_get_audio_buffer(audio_buffer_id);
  desc.buffers_count = 1;

  return world.emplace<AudioSourceID>(entt, audio_source_create(desc));
}

NoiseGenerator* entity_add_noise_generator(EntityWorld& world, EntityID& entt, const NoiseGeneratorDesc& desc) {
  NoiseGenerator* gen = noise_generator_create(desc);
  return world.emplace<NoiseGenerator*>(entt, gen);
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
  return world.emplace<PointLightComponent>(entt, transform._position, color, radius, fall_off); 
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
  const Material& default_mat = renderer_get_defaults().material; 
  return world.emplace<MeshComponent>(entt, 
                                      asset_group_get_mesh(mesh_id), 
                                      (material_id.is_valid() ? asset_group_get_material(material_id) : default_mat));
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

ModelComponent& entity_add_model(EntityWorld& world, EntityID& entt, const AssetID& model_id) {
  // Queue each mesh entity

  Model* model         = asset_group_get_model(model_id);
  Transform& transform = entity_get_component<Transform>(world, entt); 

  DynamicArray<EntityID> meshes;
  meshes.reserve(model->meshes.size());

  for(sizei i = 0; i < model->meshes.size(); i++) {
    // Get the required material index
    i8 mat_index = model->material_remaps[i];

    // Create the entity 

    EntityDesc entt_desc = {
      .tag    = "Mesh", 
      .parent = entt, 
    };
    EntityID mesh_entt = entity_create(world, entt_desc);

    // Add the mesh component
    world.emplace<MeshComponent>(mesh_entt, model->meshes[i], model->materials[mat_index]); 

    // Add the mesh entity to the model 
    meshes.emplace_back(mesh_entt); 
  }

  // Done!
  return world.emplace<ModelComponent>(entt, model, meshes);
}

BillboardComponent& entity_add_billboard(EntityWorld& world, 
                                         EntityID& entt, 
                                         const AssetID& texture_id) {
  // Get the correct texture

  Texture texture = {};
  if(texture_id.get_id() != ASSET_ID_INVALID) {
    texture = asset_group_get_texture(texture_id);
  }
  
  // Done!
  return world.emplace<BillboardComponent>(entt, texture);
}

/// EntityID functions
/// ----------------------------------------------------------------------

} // End of freya

//////////////////////////////////////////////////////////////////////////
