# TODO 

## Important Features

- Renderer
    - [x] Shadows 
    - [] IBL (Image-Based Lighting)
    - [] Post-process effects (bloom, blur, etc...)
    - [] Ambient occlusion textures
    - [] Frustrum culling (and maybe look into acceleration spatial structures)
    - [] Decals 
    - [] Particles (CPU, but GPU-based is going to be better) 

- Physics 
    - [] Box3D or Jolt?

## Immediate 

- Empty for now...

## Issues To Fix 

- Camera 
    - Improve the camera's controller to make it more general a little bit more. Look into the camera controller functions and you'll understand what needs to be fixed.

- Entity/scenes 
    - Instancing an entity is currently very weird

- Transforms 
    - Maybe we need to represent transforms differently. 
        - Instead of quats on the front end, we just use simple euler angles, and then calculate quats on the backend.

- Asset file watchers
    - They are shit. Fix 'em 

- Renderer
    - We should think about the vertex situation. Should we have one unifying vertex type or multiple???

- 2D UI
    - Maybe have a completely seperate renderer to handle the 2D stuff 
    - We already the code for it. We just need to push it in

- Billboard sprites
    - The rotations of the billboard on the X-axis are flipped for some reason, and I cannot fix them
    - Mabye have a better system for animated billboards that we can use on the GPU side

## Future Plans

- Renderer 
    - Add cascaded shadow maps
    - Point light shadows
    - Global illumination

- Entity/scenes
    - Meshes and models should have the loaded names from the disk

- Animations 
    - Do we implement 3D skeletal animations, or just use billboards for now?
