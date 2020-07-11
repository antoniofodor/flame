# Flame Game Engine
An ECS Game Engine Based On Reflection.

# Requirements:

- VisualStudio

- VulkanSDK     - https://vulkan.lunarg.com/sdk/home#windows

- Graphviz      - (optional)
  
# Libraries:

- pugixml       - https://github.com/zeux/pugixml or https://gitee.com/tkgamegroup/pugixml

- njson         - https://github.com/nlohmann/json or https://gitee.com/tkgamegroup/json

- STB           - https://github.com/nothings/stb or https://gitee.com/tkgamegroup/stb

- OpenAL        - http://www.openal.org/

# Build:

- cmake flame

- install Vulkan SDK

- install OpenAL SDK

- run ./set_env.bat (you need to restart your IDE to take that effect)

- regsvr32 msdiaXXX.dll in visual studio's dia sdk i.e. "vs_path/DIA SDK/bin/amd64"

- build
  
- enjoy

** if you want to build in release config, always build in RelWithDebInfo, because DebugInfo is always needed

### Example CMake Config:
![cmake_config](https://github.com/tkgamegroup/flame/blob/master/screenshots/cmake_config.png)

### UI Gallery:
![ui_dark](https://github.com/tkgamegroup/flame/blob/master/screenshots/ui_dark.png)

![ui_light](https://github.com/tkgamegroup/flame/blob/master/screenshots/ui_light.png)
### Render Path Editing In Blueprint:
![bp_editor](https://github.com/tkgamegroup/flame/blob/master/screenshots/bp_editor.png)
### Game Tetris:
![tetris](https://github.com/tkgamegroup/flame/blob/master/screenshots/tetris.png)
