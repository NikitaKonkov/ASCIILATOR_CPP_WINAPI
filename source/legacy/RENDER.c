// ANSI color palette (standard 16 colors)
// BLACK RED GRENN YELLOW BLUE MAGENTA CYAN GRAY 
// 30    31  32    33     34   35      36   37  DARKER
// 90    91  92    93     94   95      96   97  BRIGHTER

// BACKGROUND ANSI color palette (standard 16 colors)
// BLACK RED GRENN YELLOW BLUE MAGENTA CYAN GRAY 
// 40    41  42    43     44   45      46   47  DARKER
// 100   101 102   103    104  105     106  107 BRIGHTER

// FHD [1888 x 532]  [17.8 944 x 355]  [29.6 944 x 213] 
// 2k  [2528 x 712]  
// 4k  [3800 x 1070] 
#include <stdlib.h>
#include "windows.h"
#include "stdio.h"
#include <time.h>
#include <math.h>
#include "RENDER.h"
#include "FACE_TEXTURE.h"
#include "FACE_DRAWER.h"
#include "DOT_ANIMATION.h"
#include "EDGE_ANIMATION.h"
#include "EDGE_DRAWER.h"
#include "../camera/CAMERA.h"
#include "../input/INPUT.h"
#include "../shader/SHADER.h"
#include "../rasterizer/RASTERIZER.h"

// Flag to indicate if this is the first call
static int first_call = 1;

// Saves current console size to avoid infinite scrolling
unsigned int save_console_width = 0;
unsigned int save_console_height = 0;


// Initialize the rendering system (call once at startup)
void init_rendering_system() {
    // Clear screen once at startup
    system("cls");
    
    // Initialize buffers
    for (int y = 0; y < 2560; y++) {
        for (int x = 0; x < 2560; x++) {
            screen_buffer[y][x].valid = 0;
            screen_buffer[y][x].depth = 2560.0f;
            previous_screen_buffer[y][x].valid = 0;
            previous_screen_buffer[y][x].depth = 2560.0f;
        }
    }

    first_call = 0;
}

// Initialize the new frame buffer system
void init_frame_buffer() {
    screen_width = cmd_buffer_width;
    screen_height = cmd_buffer_height;
    
    // Copy current screen buffer to previous buffer
    for (int y = 0; y < screen_height && y < 2560; y++) {
        for (int x = 0; x < screen_width && x < 2560; x++) {
            previous_screen_buffer[y][x] = screen_buffer[y][x];
            screen_buffer[y][x].valid = 0;
            screen_buffer[y][x].depth = 2560.0f; // Far depth
        }
    }
    
    frame_buffer_pos = 0;
}

// Render the frame buffer to screen with differential updates
void render_frame_buffer() {
    frame_buffer_pos = 0;
    
    // Build output string with only changed pixels
    for (int y = 0; y < screen_height && y < 2560; y++) {
        for (int x = 0; x < screen_width && x < 2560; x++) {
            pixel current = screen_buffer[y][x];
            pixel previous = previous_screen_buffer[y][x];
            
            // Check if pixel has changed
            int pixel_changed = (current.valid != previous.valid) ||
                               (current.valid && (current.ascii != previous.ascii || 
                                                current.color != previous.color));
            
            if (pixel_changed) {
                if (frame_buffer_pos < sizeof(frame_buffer) - 50) {
                    // Draw new pixel or clear pixel using ternary operator inside snprintf
                    frame_buffer_pos += snprintf(&frame_buffer[frame_buffer_pos], 
                        sizeof(frame_buffer) - frame_buffer_pos,
                        current.valid ? "\x1b[%d;%dH\x1b[%dm%c" : "\x1b[%d;%dH ", 
                        y + 1, x + 1, 
                        current.valid ? current.color : 0, 
                        current.valid ? current.ascii : ' ');
                }
            }
        }
    }
    
    // Null terminate and output only if there are changes
    if (frame_buffer_pos > 0) {
        frame_buffer[frame_buffer_pos] = '\0';
        printf("%s", frame_buffer);
    }
}
