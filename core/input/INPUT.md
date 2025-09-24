# Input System – Keyboard & Mouse Control with Virtual Key Support

## Overview

The input system provides:
- Comprehensive keyboard input detection with virtual key code support
- Real-time mouse position tracking and control
- Virtual key simulation for automated input
- Clean separation: input system handles only input detection/simulation; main program handles all logic and responses
- Unified, easy-to-use API for both keyboard and mouse operations

---

## Core API

### Keyboard Input Functions

```c
// Check if multiple keys are pressed simultaneously
bool get_pressed_keys(int count, ...);

// Check if a key was just pressed (LSB of key state)
bool get_key_lsb(char key);

// Check if a key is currently being held down (MSB of key state)
bool get_key_msb(char key);

// Print all currently pressed keys to console
void print_pressed_keys();

// Simulate pressing and releasing virtual keys
void press_virtual_keys(int count, ...);
```

### Mouse Input Functions

```c
// Get current mouse cursor position
void get_mouse_position(int *x, int *y);

// Set mouse cursor position
void set_mouse_position(int x, int y);

// Print current mouse position to console
void print_mouse_position();
```

---

## Virtual Key Codes

### Special Keys
```c
#define VK_ESCAPE        0x1B  // Escape key
#define VK_TAB           0x09  // Tab key
#define VK_CAPITAL       0x14  // Caps Lock key
#define VK_SHIFT         0x10  // Shift key
#define VK_CONTROL       0x11  // Control key
#define VK_MENU          0x12  // Alt key
#define VK_SPACE         0x20  // Spacebar
#define VK_RETURN        0x0D  // Enter key
#define VK_BACK          0x08  // Backspace key
```

### Arrow Keys
```c
#define VK_LEFT          0x25  // Left arrow key
#define VK_UP            0x26  // Up arrow key
#define VK_RIGHT         0x27  // Right arrow key
#define VK_DOWN          0x28  // Down arrow key
```

### Function Keys
```c
#define VK_F1            0x70  // F1 key
#define VK_F2            0x71  // F2 key
// ... F3 through F12 (0x72 - 0x7B)
```

### Number & Alphabet Keys
```c
#define VK_0             0x30  // '0' key
#define VK_1             0x31  // '1' key
// ... 2-9 (0x32 - 0x39)

#define VK_A             0x41  // 'A' key
#define VK_B             0x42  // 'B' key
// ... C-Z (0x43 - 0x5A)
```

### Numpad Keys
```c
#define VK_NUMPAD0       0x60  // Numpad '0' key
#define VK_NUMPAD1       0x61  // Numpad '1' key
// ... NUMPAD2-9 (0x62 - 0x69)
#define VK_MULTIPLY      0x6A  // Numpad '*' key
#define VK_ADD           0x6B  // Numpad '+' key
#define VK_SUBTRACT      0x6D  // Numpad '-' key
#define VK_DECIMAL       0x6E  // Numpad '.' key
#define VK_DIVIDE        0x6F  // Numpad '/' key
```

---

## Usage Examples

### Keyboard Input Detection

```c
// Check if WASD keys are pressed for movement
if (get_key_msb(VK_W)) {
    move_forward();
}
if (get_key_msb(VK_A)) {
    move_left();
}

// Check for key combination (Ctrl+C)
if (get_pressed_keys(2, VK_CONTROL, VK_C)) {
    copy_to_clipboard();
}

// Detect single key press event
if (get_key_lsb(VK_SPACE)) {
    jump();  // Only triggers once per press
}

// Debug: Show all pressed keys
print_pressed_keys();
```

### Virtual Key Simulation

```c
// Simulate typing "Hello"
press_virtual_keys(5, 'H', 'e', 'l', 'l', 'o');

// Simulate key combination (Alt+Tab)
press_virtual_keys(2, VK_MENU, VK_TAB);

// Simulate arrow key navigation
press_virtual_keys(1, VK_RIGHT);
```

### Mouse Control

```c
// Get current mouse position
int mouse_x, mouse_y;
get_mouse_position(&mouse_x, &mouse_y);

// Center mouse on screen (assuming 1920x1080)
set_mouse_position(960, 540);

// Debug: Show current position
print_mouse_position();

// Create mouse movement animation
for (int i = 0; i < 100; i++) {
    set_mouse_position(100 + i * 5, 100 + i * 2);
    Sleep(10);
}
```

### Combined Input Scenarios

```c
// Game-style input handling
while (game_running) {
    // Movement with WASD
    if (get_key_msb(VK_W)) player_move_forward();
    if (get_key_msb(VK_S)) player_move_backward();
    if (get_key_msb(VK_A)) player_move_left();
    if (get_key_msb(VK_D)) player_move_right();
    
    // Mouse look
    int mx, my;
    get_mouse_position(&mx, &my);
    update_camera_rotation(mx, my);
    
    // Action on space press
    if (get_key_lsb(VK_SPACE)) {
        player_jump();
    }
    
    // Exit on escape
    if (get_key_lsb(VK_ESCAPE)) {
        game_running = false;
    }
}
```

---

## Architecture

- `keyboard.h`: Virtual key definitions and keyboard function declarations
- `mouse.h`: Mouse function declarations
- `get_keyboard.c`: Key state detection and multi-key checking
- `get_mouse.c`: Mouse position retrieval
- `print_keyboard.c`: Debug output for pressed keys
- `print_mouse.c`: Debug output for mouse position
- `set_keyboard.c`: Virtual key simulation
- `set_mouse.c`: Mouse position control
- Main program: Handles all game logic, responses, and calls input API

---

## Key Features

- Real-time key state detection with MSB/LSB differentiation
- Multi-key combination detection
- Virtual key simulation for automation
- Mouse position tracking and control
- Comprehensive virtual key code definitions
- Clean separation of input detection and game logic
- Debug utilities for development and testing

---

This input system provides a complete solution for Windows-based keyboard and mouse interaction, suitable for games, automation tools, and interactive applications.
