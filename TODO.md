# TODO 

## Features We Definitely Need

- Physics 
    - [] Box3D or Jolt?

- Animations 
    - [] Do we implement 3D skeletal animations, or just use billboards for now?

- Renderer
    - [] Shadows 
    - [] Post-process effects (bloom, blur, etc...)
    - [] Emissive textures (remember to convert them from sRGB)
    - [] IBL (Image-Based Lighting)
    - [] Decals 
    - [] Particles (CPU, but GPU-based is going to be better) 
    - [] Frustrum culling (and maybe look into acceleration spatial structures)
    - [] Global illumination

## Immediate 

- Wouldn't be better if we looped over the mesh components with the transforms in the renderer rather than attaching the transform into the component itself and having to update the transform every frame in the entity world?

- Maybe we should just use an sRGB format for color textures instead of decoding and encoding the values in the shader?

## Issues To Fix 

- Camera 
    - Improve the camera's controller to make it more general a little bit more. Look into the camera controller functions and you'll understand what needs to be fixed.

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
