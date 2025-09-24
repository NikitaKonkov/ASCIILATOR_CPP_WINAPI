#include "render.hpp"
#define min(a,b) (((a) < (b)) ? (a) : (b))

// Basic distance calculations - kept from original
float calculate_dot_distance(dot d) {
    float dx = d.position.x - camera.x;
    float dy = d.position.y - camera.y;
    float dz = d.position.z - camera.z;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

float calculate_edge_distance(edge e) {
    float mid_x = (e.start.x + e.end.x) / 2.0f;
    float mid_y = (e.start.y + e.end.y) / 2.0f;
    float mid_z = (e.start.z + e.end.z) / 2.0f;
    float dx = mid_x - camera.x;
    float dy = mid_y - camera.y;
    float dz = mid_z - camera.z;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

float calculate_face_distance(face f) {
    float center_x = 0.0f, center_y = 0.0f, center_z = 0.0f;
    for (int i = 0; i < f.vertex_count; i++) {
        center_x += f.vertices[i].x;
        center_y += f.vertices[i].y;
        center_z += f.vertices[i].z;
    }
    center_x /= f.vertex_count;
    center_y /= f.vertex_count;
    center_z /= f.vertex_count;
    
    float dx = center_x - camera.x;
    float dy = center_y - camera.y;
    float dz = center_z - camera.z;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

// Vertex projection function - essential for 3D rendering
vertex project_vertex(vertex v, float cam_x, float cam_y, float cam_z, float cam_yaw, float cam_pitch, float fov, float aspect_ratio, float near_plane) {
    // Transform to camera space
    float dx = v.x - cam_x;
    float dy = v.y - cam_y;
    float dz = v.z - cam_z;
    
    // Update cache if needed
    if (!is_camera_cache_valid()) {
        update_camera_cache();
    }
    
    // Apply yaw rotation
    float temp_x = dx * cached_transform.cos_yaw - dz * cached_transform.sin_yaw;
    float temp_z = dx * cached_transform.sin_yaw + dz * cached_transform.cos_yaw;
    dx = temp_x;
    dz = temp_z;
    
    // Apply pitch rotation
    float temp_y = dy * cached_transform.cos_pitch - dz * cached_transform.sin_pitch;
    temp_z = dy * cached_transform.sin_pitch + dz * cached_transform.cos_pitch;
    dy = temp_y;
    dz = temp_z;
    
    // Perspective projection
    vertex projected;
    if (dz > 0.01f) { // Prevent division by zero
        projected.x = (dx * near_plane / dz) / aspect_ratio;
        projected.y = dy * near_plane / dz;
        projected.z = dz; // Keep depth for z-buffer
    } else {
        projected.x = 0;
        projected.y = 0;
        projected.z = -1; // Behind camera
    }
    
    return projected;
}

// Depth calculation for rendering objects
float calculate_edge_depth(edge e) {
    return calculate_edge_distance(e);
}

float calculate_renderable_depth(renderable r) {
    switch (r.type) {
        case 0: return calculate_edge_distance(r.object.e);
        case 1: return calculate_dot_distance(r.object.d);
        case 2: return calculate_face_distance(r.object.f);
        default: return 1000000.0f;
    }
}

// Comparison function for depth sorting
int compare_renderables_by_depth(const void *a, const void *b) {
    const renderable *ra = (const renderable *)a;
    const renderable *rb = (const renderable *)b;
    float diff = rb->depth - ra->depth; // Sort back to front
    if (diff < 0) return -1;
    if (diff > 0) return 1;
    return 0;
}

// Camera system and movement - positioned to view cubes
camera3d camera = {0.0f, 0.0f, -10.0f, 0.0f, 0.0f}; 

// Aspect ratio correction for console character stretching
float aspect_ratio_width = 1.0f;  
float aspect_ratio_height = 2.0f; 

// Culling distance for 3D objects
const float culling_distance = 0.5f; 
const float view_distance = 100000.0f; 

// Movement vectors
float diagonal_x, diagonal_y, diagonal_z;
float horizontal_x, horizontal_y, horizontal_z;

// Movement and turning speeds
float camera_speed = 0.2f;
float camera_turn_speed = 0.1f;

// Camera transformation caching
camera_cache cached_transform = {0};

void update_camera_cache() {
    cached_transform.cam_x = camera.x;
    cached_transform.cam_y = camera.y;
    cached_transform.cam_z = camera.z;
    cached_transform.cam_yaw = camera.yaw;
    cached_transform.cam_pitch = camera.pitch;
    cached_transform.cos_yaw = cos(-camera.yaw);
    cached_transform.sin_yaw = sin(-camera.yaw);
    cached_transform.cos_pitch = cos(-camera.pitch);
    cached_transform.sin_pitch = sin(-camera.pitch);
    cached_transform.valid = 1;
}

int is_camera_cache_valid() {
    return cached_transform.valid &&
           cached_transform.cam_x == camera.x &&
           cached_transform.cam_y == camera.y &&
           cached_transform.cam_z == camera.z &&
           cached_transform.cam_yaw == camera.yaw &&
           cached_transform.cam_pitch == camera.pitch;
}

void camera_update() {
    float cos_yaw = cos(camera.yaw);
    float sin_yaw = sin(camera.yaw);
    float cos_pitch = cos(camera.pitch);
    float sin_pitch = sin(camera.pitch);
    
    // Forward vector (with pitch)
    diagonal_x = -sin_yaw * cos_pitch;
    diagonal_y = -sin_pitch;
    diagonal_z = cos_yaw * cos_pitch;
    
    // Right vector (horizontal only)
    horizontal_x = cos_yaw;
    horizontal_y = 0.0f;
    horizontal_z = sin_yaw;
    
    // Clamp pitch to prevent flipping
    const float MAX_PITCH = 1.5f;
    if (camera.pitch > MAX_PITCH) camera.pitch = MAX_PITCH;
    if (camera.pitch < -MAX_PITCH) camera.pitch = -MAX_PITCH;
}

// Drawing system - simplified frame buffer approach
pixel screen_buffer[2560][2560];
pixel previous_screen_buffer[2560][2560];
int screen_width = 120;
int screen_height = 60;

// Buffer dimensions
unsigned int cmd_buffer_width = 120; 
unsigned int cmd_buffer_height = 60;

// Buffer position tracking
int frame_buffer_pos = 0;

// Aspect ratio functions
void set_aspect_ratio(float width_scale, float height_scale) {
    aspect_ratio_width = width_scale;
    aspect_ratio_height = height_scale;
}

void get_aspect_ratio(float *width_scale, float *height_scale) {
    *width_scale = aspect_ratio_width;
    *height_scale = aspect_ratio_height;
}

// Basic pixel setting with depth testing
int set_pixel(int x, int y, char ascii, int color, float depth) {
    if (x < 0 || x >= screen_width || y < 0 || y >= screen_height) {
        return 0; // Out of bounds
    }
    
    // Depth test
    if (depth < screen_buffer[y][x].depth) {
        screen_buffer[y][x].ascii = ascii;
        screen_buffer[y][x].color = color;
        screen_buffer[y][x].depth = depth;
        screen_buffer[y][x].valid = 1;
        return 1;
    }
    
    return 0;
}

// Basic drawing functions - simplified without shaders
void draw_dot(dot d) {
    vertex projected = project_vertex(d.position, camera.x, camera.y, camera.z, 
                                    camera.yaw, camera.pitch, 90.0f, 
                                    (float)screen_width / (float)screen_height, 0.1f);
    
    if (projected.z > 0.1f) { // In front of camera
        // Convert to screen coordinates (scale from -1,1 to 0,screen_size)
        int screen_x = (int)((projected.x * 0.5f + 0.5f) * screen_width);
        int screen_y = (int)((0.5f - projected.y * 0.5f) * screen_height);
        
        // Clamp to screen bounds
        if (screen_x >= 0 && screen_x < screen_width && screen_y >= 0 && screen_y < screen_height) {
            set_pixel(screen_x, screen_y, d.ascii, d.color, projected.z);
        }
    }
}

void draw_edge(edge e) {
    vertex start_proj = project_vertex(e.start, camera.x, camera.y, camera.z, 
                                     camera.yaw, camera.pitch, 90.0f, 
                                     (float)screen_width / (float)screen_height, 0.1f);
    vertex end_proj = project_vertex(e.end, camera.x, camera.y, camera.z, 
                                   camera.yaw, camera.pitch, 90.0f, 
                                   (float)screen_width / (float)screen_height, 0.1f);
    
    if (start_proj.z > 0.1f && end_proj.z > 0.1f) { // Both points in front
        // Convert to screen coordinates
        int x1 = (int)((start_proj.x * 0.5f + 0.5f) * screen_width);
        int y1 = (int)((0.5f - start_proj.y * 0.5f) * screen_height);
        int x2 = (int)((end_proj.x * 0.5f + 0.5f) * screen_width);
        int y2 = (int)((0.5f - end_proj.y * 0.5f) * screen_height);
        
        // Bresenham's line algorithm
        int dx = abs(x2 - x1);
        int dy = abs(y2 - y1);
        int sx = x1 < x2 ? 1 : -1;
        int sy = y1 < y2 ? 1 : -1;
        int err = dx - dy;
        
        int x = x1, y = y1;
        float depth = (start_proj.z + end_proj.z) * 0.5f;
        
        while (1) {
            set_pixel(x, y, e.ascii, e.color, depth);
            
            if (x == x2 && y == y2) break;
            
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x += sx;
            }
            if (e2 < dx) {
                err += dx;
                y += sy;
            }
        }
    }
}

void draw_face(face f) {
    // Simple face drawing - just draw the vertices as dots
    // In a full implementation, this would be triangle rasterization
    for (int i = 0; i < f.vertex_count; i++) {
        dot d;
        d.position = f.vertices[i];
        d.ascii = f.ascii;
        d.color = f.color;
        draw_dot(d);
    }
    
    // Draw edges between vertices
    for (int i = 0; i < f.vertex_count; i++) {
        int next = (i + 1) % f.vertex_count;
        edge e;
        e.start = f.vertices[i];
        e.end = f.vertices[next];
        e.ascii = f.ascii;
        e.color = f.color;
        draw_edge(e);
    }
}

// Output buffer - simplified without differential rendering for now
void output_buffer() {
    
    // Output buffer contents
    for (int y = 0; y < screen_height; y++) {
        for (int x = 0; x < screen_width; x++) {
            if (screen_buffer[y][x].valid) {
                printf("\033[%dm%c\033[0m", screen_buffer[y][x].color, screen_buffer[y][x].ascii);
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
}

// Main geometry drawing function
void geometry_draw() {
    // Clear buffer
    for (int y = 0; y < screen_height; y++) {
        for (int x = 0; x < screen_width; x++) {
            screen_buffer[y][x].ascii = ' ';
            screen_buffer[y][x].color = 0;
            screen_buffer[y][x].depth = 1000000.0f;
            screen_buffer[y][x].valid = 0;
        }
    }
}

// Console size management
unsigned int save_console_width = 120;
unsigned int save_console_height = 60;

// Mouse sensitivity for camera control
float mouse_sensitivity = 0.003f;
static int last_mouse_x = 0;
static int last_mouse_y = 0;
static bool mouse_captured = false;

// Initialize mouse capture for camera control
void init_mouse_camera() {
    POINT pt;
    GetCursorPos(&pt);
    last_mouse_x = pt.x;
    last_mouse_y = pt.y;
    mouse_captured = true;
}

// Update camera based on mouse movement
void update_camera_mouse(int mouse_x, int mouse_y) {
    if (!mouse_captured) return;
    
    int delta_x = mouse_x - last_mouse_x;
    int delta_y = mouse_y - last_mouse_y;
    
    // Update camera yaw and pitch
    camera.yaw += delta_x * mouse_sensitivity;
    camera.pitch -= delta_y * mouse_sensitivity; // Invert Y axis
    
    // Clamp pitch
    const float MAX_PITCH = 1.5f;
    if (camera.pitch > MAX_PITCH) camera.pitch = MAX_PITCH;
    if (camera.pitch < -MAX_PITCH) camera.pitch = -MAX_PITCH;
    
    last_mouse_x = mouse_x;
    last_mouse_y = mouse_y;
    
    // Update camera vectors
    camera_update();
}

// Move camera based on keyboard input
void move_camera(bool forward, bool backward, bool left, bool right, bool up, bool down) {
    if (forward) {
        camera.x += diagonal_x * camera_speed;
        camera.y += diagonal_y * camera_speed;
        camera.z += diagonal_z * camera_speed;
    }
    if (backward) {
        camera.x -= diagonal_x * camera_speed;
        camera.y -= diagonal_y * camera_speed;
        camera.z -= diagonal_z * camera_speed;
    }
    if (right) {
        camera.x += horizontal_x * camera_speed;
        camera.y += horizontal_y * camera_speed;
        camera.z += horizontal_z * camera_speed;
    }
    if (left) {
        camera.x -= horizontal_x * camera_speed;
        camera.y -= horizontal_y * camera_speed;
        camera.z -= horizontal_z * camera_speed;
    }
    if (up) {
        camera.y += camera_speed;
    }
    if (down) {
        camera.y -= camera_speed;
    }
}

// Rotate vertex around center point
vertex rotate_vertex(vertex v, vertex center, float angle_x, float angle_y, float angle_z) {
    vertex result = v;
    
    // Translate to origin
    result.x -= center.x;
    result.y -= center.y;
    result.z -= center.z;
    
    // Rotate around X axis
    float cos_x = cos(angle_x);
    float sin_x = sin(angle_x);
    float temp_y = result.y * cos_x - result.z * sin_x;
    float temp_z = result.y * sin_x + result.z * cos_x;
    result.y = temp_y;
    result.z = temp_z;
    
    // Rotate around Y axis
    float cos_y = cos(angle_y);
    float sin_y = sin(angle_y);
    float temp_x = result.x * cos_y + result.z * sin_y;
    temp_z = -result.x * sin_y + result.z * cos_y;
    result.x = temp_x;
    result.z = temp_z;
    
    // Rotate around Z axis
    float cos_z = cos(angle_z);
    float sin_z = sin(angle_z);
    temp_x = result.x * cos_z - result.y * sin_z;
    temp_y = result.x * sin_z + result.y * cos_z;
    result.x = temp_x;
    result.y = temp_y;
    
    // Translate back
    result.x += center.x;
    result.y += center.y;
    result.z += center.z;
    
    return result;
}

// Create a colored cube with rotation
void draw_rotating_cube(vertex center, float size, float rotation_x, float rotation_y, float rotation_z) {
    // Define cube vertices (8 corners)
    vertex cube_vertices[8];
    float half_size = size * 0.5f;
    
    // Create base cube vertices
    cube_vertices[0] = {center.x - half_size, center.y - half_size, center.z - half_size}; // Front bottom left
    cube_vertices[1] = {center.x + half_size, center.y - half_size, center.z - half_size}; // Front bottom right
    cube_vertices[2] = {center.x + half_size, center.y + half_size, center.z - half_size}; // Front top right
    cube_vertices[3] = {center.x - half_size, center.y + half_size, center.z - half_size}; // Front top left
    cube_vertices[4] = {center.x - half_size, center.y - half_size, center.z + half_size}; // Back bottom left
    cube_vertices[5] = {center.x + half_size, center.y - half_size, center.z + half_size}; // Back bottom right
    cube_vertices[6] = {center.x + half_size, center.y + half_size, center.z + half_size}; // Back top right
    cube_vertices[7] = {center.x - half_size, center.y + half_size, center.z + half_size}; // Back top left
    
    // Rotate all vertices
    for (int i = 0; i < 8; i++) {
        cube_vertices[i] = rotate_vertex(cube_vertices[i], center, rotation_x, rotation_y, rotation_z);
    }
    
    // Define cube edges with different colors
    int cube_edges[][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Front face
        {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Back face
        {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Connecting edges
    };
    
    // Color scheme for different edge groups
    int colors[] = {31, 32, 33, 34, 35, 36}; // Red, Green, Yellow, Blue, Magenta, Cyan
    char ascii_chars[] = {'#', '@', '&', '%', '*', '+'};
    
    // Draw all edges
    for (int i = 0; i < 12; i++) {
        edge e;
        e.start = cube_vertices[cube_edges[i][0]];
        e.end = cube_vertices[cube_edges[i][1]];
        
        // Color edges differently based on which face/group they belong to
        if (i < 4) {
            e.color = colors[0]; // Front face - red
            e.ascii = ascii_chars[0];
        } else if (i < 8) {
            e.color = colors[1]; // Back face - green
            e.ascii = ascii_chars[1];
        } else {
            e.color = colors[2 + (i % 4)]; // Connecting edges - various colors
            e.ascii = ascii_chars[2 + (i % 4)];
        }
        
        draw_edge(e);
    }
    
    // Draw cube faces for a more solid look
    // Front face
    face front_face;
    front_face.vertices[0] = cube_vertices[0];
    front_face.vertices[1] = cube_vertices[1];
    front_face.vertices[2] = cube_vertices[2];
    front_face.vertices[3] = cube_vertices[3];
    front_face.vertex_count = 4;
    front_face.color = 91; // Bright red
    front_face.ascii = '.';
    draw_face(front_face);
    
    // Back face
    face back_face;
    back_face.vertices[0] = cube_vertices[4];
    back_face.vertices[1] = cube_vertices[7];
    back_face.vertices[2] = cube_vertices[6];
    back_face.vertices[3] = cube_vertices[5];
    back_face.vertex_count = 4;
    back_face.color = 92; // Bright green
    back_face.ascii = ':';
    draw_face(back_face);
}