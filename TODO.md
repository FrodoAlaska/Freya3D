# TODO 

## Issues To Fix 

- Transforms 
    - Maybe we need to represent transforms differently. 
    - Instead of quats on the front end, we just use simple euler angles, and then calculate quats on the backend.

- Camera 
    - The camera needs some work 
    - We need to have a transform in the camera that will set its position, rotation, and scale 
    - Or maybe it's better to leave it since it is closer to the FPS spirit? We are going to only make FPS games after all.

- Asset file watchers
    - They are shit. Fix 'em 

- Renderer
    - Normal maps

- 2D UI
    - Maybe have a completely seperate renderer to handle the 2D stuff 
    - We already the code for it. We just need to push it in
