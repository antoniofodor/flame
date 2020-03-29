# Flame Game Engine
An ECS Game Engine Based On Reflection.

# Requirements:

- VisualStudio

- VulkanSDK     - https://vulkan.lunarg.com/sdk/home#windows

- Graphviz      - (optional)
  
# Libraries:

- pugixml       - https://github.com/zeux/pugixml

- njson         - https://github.com/nlohmann/json

- STB           - https://github.com/nothings/stb
  
- freetype      - https://github.com/ubawurinna/freetype-windows-binaries

- msdfgen       - (must use my forked version, changed a little bit) https://github.com/tkgamegroup/msdfgen

- SPIRV-Cross   - https://github.com/KhronosGroup/SPIRV-Cross

- OpenAL        - http://www.openal.org/

# Build:

- cmake flame

- run ./set_env.bat (you need to restart your IDE to take that affect)

- regsvr32 msdiaXXX.dll in visual studio's dia sdk i.e. "vs_path/DIA SDK/bin/amd64"

- build
  
- enjoy

PS1: if you want to build in release config, always build in RelWithDebInfo, because DebugInfo is always needed

PS2: if any strange errors were generated by typeinfogen, you can try to delete and remake that pdb and try again

### Example CMake Config:
![cmake_config](https://github.com/tkgamegroup/flame/blob/master/screenshots/cmake_config.png)

### UI Gallery:
![ui_dark](https://github.com/tkgamegroup/flame/blob/master/screenshots/ui_dark.png)

![ui_light](https://github.com/tkgamegroup/flame/blob/master/screenshots/ui_light.png)
### Render Path Editing In Blueprint:
![bp_editor](https://github.com/tkgamegroup/flame/blob/master/screenshots/bp_editor.png)
### Game Tetris:
![tetris](https://github.com/tkgamegroup/flame/blob/master/screenshots/tetris.png)
