# Iterate through every shader
# TODO: Perhaps have different shaders outputing to different directories

foreach ($file in Get-ChildItem -Path "..\assets\shaders\" -File) {
  $input  = $file.FullName
  $output = "..\freya\src\renderer\shaders\$($file.BaseName).h"

  sokol-shdc -i $input -o $output -l glsl430:glsl300es --no-log-cmdline
  Write-Host "Successfully reloaded shader $($input) and placed at $($output)..." -Foregroundcolor "Green"
}
