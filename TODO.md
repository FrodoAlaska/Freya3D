# TODO 

## Issues To Fix 

- Transforms 
    - Maybe we need to represent transforms differently. 
    - Instead of quats on the front end, we just use simple euler angles, and then calculate quats on the backend.

- Camera 
    - The camera needs some work 
    - We need to have a transform in the camera that will set its position, rotation, and scale 
    - So instead of a `zoom`, `yaw`, and `pitch` members, we can have the transform's scale be the zoom and the transform's quat as the yaw and pitch. 
    - Or maybe it's better to leave it this way since it is closer to the FPS spirit? We are going to only make FPS games after all.

- Asset file watchers

- 2D UI
    - Maybe have a completely seperate renderer to handle the 2D stuff 
    - We already the code for it. We just need to push it in
