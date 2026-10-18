# TODO 

## Immediate 

- Renderer 
    - There's a weird black dot that appears for some fucking reason. Fix it!

- 3D Models (loading and rendering)
    - Load from a GLTF file 
    - Write the info into the FRPKG file
    - Model component
    - Render the 3D model
    - Use `meshoptimizer` to generate the normals and tangents of models, and compress the mesh files

- Physics 
    - Either Box3D or Jolt... I'm not sure yet...

## Issues To Fix 

- Transforms 
    - Maybe we need to represent transforms differently. 
    - Instead of quats on the front end, we just use simple euler angles, and then calculate quats on the backend.

- Camera 
    - We need to have a transform in the camera that will set its position, rotation, and scale 
    - Or maybe it's better to leave it since it is closer to the FPS spirit? We are going to only make FPS games after all.

- Asset file watchers
    - They are shit. Fix 'em 

- Renderer
    - Normal maps
    - HDR colors, exposure, and gamma correction. The colors in general are fucked. We need some tone-mapping, maybe?
    - We should think about the vertex situation. Should we have one unifying vertex type or multiple???
    - Implement acceleration spatial structures to improve the renderer's performance

- 2D UI
    - Maybe have a completely seperate renderer to handle the 2D stuff 
    - We already the code for it. We just need to push it in

- Billboard sprites
    - The rotations of the billboard on the X-axis are flipped for some reason, and I cannot fix them
    - Mabye have a better system for animated billboards that we can use on the GPU side
