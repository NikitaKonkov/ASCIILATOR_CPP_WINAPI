#ifndef RENDER_HPP
#define RENDER_HPP

#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdlib.h>
#include <math.h>
#include "windows.h"
#include "stdio.h"
#include <time.h>

#define M_PI 3.14159265358979323846

// Vertex structure for 3D coordinates
typedef struct {
    float x, y, z;
} vertex;

// Face structure for triangles and quads (removed texture support)
typedef struct {
    vertex vertices[4]; // Up to 4 vertices (3 for triangle, 4 for quad)
    int vertex_count;   // 3 for triangle, 4 for quad
    int color;          // Color code
    char ascii;         // ASCII character for face
} face;

// Edge structure for drawing lines between vertices
typedef struct {
    vertex start, end;
    char ascii;
    int color;
} edge;

// Dot structure for drawing single points
typedef struct {
    vertex position;
    char ascii;
    int color;
} dot;

// Basic distance calculations
float calculate_dot_distance(dot d);
float calculate_edge_distance(edge e);
float calculate_face_distance(face f);

// 3D Camera system
typedef struct {
    float x, y, z;        // Camera position
    float yaw, pitch;     // Camera rotation (yaw = left/right, pitch = up/down)
} camera3d;

// Global camera instance
extern camera3d camera; 

// Aspect ratio correction for console character stretching
extern float aspect_ratio_width;  // Width scaling factor (1.0 = no scaling)
extern float aspect_ratio_height; // Height scaling factor (2.0 = compensate for tall characters)

// Culling distance for 3D objects
extern const float culling_distance; // Distance behind camera to start culling
extern const float view_distance; // Maximum view distance - objects beyond this distance are culled

// Movement vectors
extern float diagonal_x, diagonal_y, diagonal_z;
extern float horizontal_x, horizontal_y, horizontal_z;

// Camera movement controls (WASD) - aligned with mouse look direction
extern float camera_speed;

// Camera turning speed (Q/E for left/right, R/F for up/down)
extern float camera_turn_speed;

// Camera transformation caching for optimization
typedef struct {
    float cos_yaw, sin_yaw, cos_pitch, sin_pitch;
    float cam_x, cam_y, cam_z, cam_yaw, cam_pitch;
    int valid;
} camera_cache;

extern camera_cache cached_transform;

// Camera functions
void update_camera_cache();
int is_camera_cache_valid();
void camera_update();

// Frame buffer pixel structure
typedef struct {
    char ascii;
    int color;
    float depth;
    int valid;  // 1 if pixel is drawn, 0 if empty
} pixel;

// 2D frame buffer for pixel-perfect rendering
extern pixel screen_buffer[2560][2560];
extern pixel previous_screen_buffer[2560][2560];
extern int screen_width;
extern int screen_height;

// Buffer dimensions
extern unsigned int cmd_buffer_width; 
extern unsigned int cmd_buffer_height;

// Buffer position for drawing
extern int frame_buffer_pos;

// Unified renderable object for depth sorting
typedef struct {
    int type; // 0 = edge, 1 = dot, 2 = face
    union {
        edge e;
        dot d;
        face f;
    } object;
    float depth;
} renderable;

// Core rendering functions
int set_pixel(int x, int y, char ascii, int color, float depth);
void set_aspect_ratio(float width_scale, float height_scale);
void get_aspect_ratio(float *width_scale, float *height_scale);

// Vertex projection
vertex project_vertex(vertex v, float cam_x, float cam_y, float cam_z, float cam_yaw, float cam_pitch, float fov, float aspect_ratio, float near_plane);

// Depth calculations
float calculate_edge_depth(edge e);
float calculate_renderable_depth(renderable r);
int compare_renderables_by_depth(const void *a, const void *b);

// Drawing functions
void draw_dot(dot d);
void draw_edge(edge e);
void draw_face(face f);

// Console size management
extern unsigned int save_console_width;
extern unsigned int save_console_height;

// Main rendering functions
void output_buffer();
void geometry_draw();

#endif // RENDER_HPP