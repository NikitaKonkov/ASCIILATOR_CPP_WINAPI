# Render Thread Guide - 3D Graphics in Console

## 🎮 Real-Time 3D Rendering Thread

Your ASCIILATOR application now includes a dedicated render thread that displays 3D graphics in the console with full camera movement!

### 🕹️ **Movement Controls**

| Control | Action | Description |
|---------|--------|-------------|
| **W** | Move Forward | Move camera toward where you're looking |
| **S** | Move Backward | Move camera away from where you're looking |
| **A** | Strafe Left | Move camera left (perpendicular to view direction) |
| **D** | Strafe Right | Move camera right (perpendicular to view direction) |
| **Space** | Move Up | Move camera upward in world space |
| **Shift** | Move Down | Move camera downward in world space |
| **Mouse** | Look Around | Control camera pitch (up/down) and yaw (left/right) |
| **ESC** | Exit | Exit entire application |

### 🎯 **What You'll See**

The render thread displays **3 different test cubes**:

1. **Solid Cube (Position: 5, 0, 5)**
   - Rendered as colored faces with `#` characters
   - Each face has a different bright color (red, green, blue, yellow, magenta, cyan)
   - Demonstrates face-based rendering

2. **Wireframe Cube (Position: -5, 0, 5)**  
   - Rendered as colored edges with `=`, `|`, `+` characters
   - Each edge has a different color
   - Demonstrates edge-based rendering

3. **Point Cube (Position: 0, 0, -5)**
   - Rendered as colored vertices with `*` and `o` characters
   - Each vertex has a different color
   - Demonstrates dot/vertex-based rendering

### 🏗️ **Technical Features**

- **60 FPS Rendering**: Smooth real-time graphics using ClockManager
- **3D Projection**: Full perspective projection with depth testing
- **Mouse Look**: Delta-based mouse movement for smooth camera control
- **Frustum Culling**: Objects outside view are automatically clipped
- **Depth Buffer**: Proper Z-buffering for correct depth sorting
- **Console Graphics**: ANSI color codes and character-based rendering
- **Aspect Ratio Correction**: Compensates for console character stretching

### 🎛️ **Camera System**

- **Starting Position**: (100, -2.5, 100) - positioned to view all cubes
- **Mouse Sensitivity**: 0.003 (adjustable)
- **Movement Speed**: 0.2 units per frame (adjustable)
- **Pitch Clamping**: Limited to ±1.5 radians to prevent flipping
- **Auto-Centering**: Mouse automatically resets to center for delta calculation

### 🚀 **Thread Architecture**

The render thread:
1. **Initializes** the RenderManager and sets up the camera
2. **Updates** camera position from mouse and keyboard input
3. **Renders** at 60 FPS using ClockManager synchronization
4. **Draws** all test objects with proper depth sorting
5. **Presents** the frame to console using differential rendering

### 💡 **Usage Tips**

1. **Start the Application**: Run `engine.exe` to launch all threads
2. **Look Around**: Move your mouse to look in different directions
3. **Move Around**: Use WASD keys to navigate the 3D space
4. **Explore the Cubes**: Move close to see different rendering styles
5. **Height Control**: Use Space/Shift for vertical movement

### 🔧 **Integration Details**

```cpp
// The render thread is created in main.cpp:
threadManager.CreateThread("render_graphics", ThreadType::RENDER_THREAD);

// It uses these core components:
- InputManager: For keyboard and mouse input
- RenderManager: For 3D graphics and camera control
- ClockManager: For 60 FPS timing
- ConsoleManager: For status messages
```

### 🎨 **Rendering Pipeline**

1. **Input Processing**: Read mouse delta and keyboard states
2. **Camera Update**: Update camera position and orientation
3. **Frame Begin**: Clear depth buffer and screen buffer
4. **Object Drawing**: Render test objects with clipping and depth testing
5. **Frame End**: Present final image to console with ANSI codes
6. **Timing**: Maintain 60 FPS using ClockManager

### 🎮 **Real-Time Experience**

- **Immediate Response**: Camera movement responds instantly to input
- **Smooth Graphics**: 60 FPS rendering for fluid motion
- **3D Navigation**: Full freedom of movement in 3D space
- **Visual Feedback**: Different cube styles demonstrate various rendering techniques

The render thread showcases the power of your multi-threaded architecture - it runs independently while other threads handle sound, console input, and window management simultaneously! 🎮✨

### 🎪 **Fun Things to Try**

- Move close to the wireframe cube to see the edge details
- Look at the solid cube from different angles to see face culling
- Navigate to the point cube to see individual vertex rendering
- Combine movement keys (W+A, W+Space, etc.) for diagonal movement
- Use mouse to look up/down while moving forward for flight-like navigation