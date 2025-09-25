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
    // Update cache if needed
    if (!is_camera_cache_valid()) {
        update_camera_cache();
    }
    
    // Translate vertex relative to camera
    float dx = v.x - cached_transform.cam_x;
    float dy = v.y - cached_transform.cam_y;
    float dz = v.z - cached_transform.cam_z;

    // Rotate around yaw (Y-axis) using cached values
    float temp_x = dx * cached_transform.cos_yaw - dz * cached_transform.sin_yaw;
    float temp_z = dx * cached_transform.sin_yaw + dz * cached_transform.cos_yaw;
    dx = temp_x;
    dz = temp_z;

    // Rotate around pitch (X-axis) using cached values
    float temp_y = dy * cached_transform.cos_pitch - dz * cached_transform.sin_pitch;
    temp_z = dy * cached_transform.sin_pitch + dz * cached_transform.cos_pitch;
    dy = temp_y;
    dz = temp_z;

    // Perspective projection with aspect ratio correction
    if (dz <= near_plane) dz = near_plane; // Avoid division by zero
    
    // Apply aspect ratio correction to compensate for console character stretching
    float screen_x = (dx / dz) * (screen_width / 2) / tan(fov * 0.5 * M_PI / 180.0f) * aspect_ratio_width + (screen_width / 2);
    float screen_y = (dy / dz) * (screen_height / 2) / tan(fov * 0.5 * M_PI / 180.0f) * aspect_ratio_height + (screen_height / 2);

    vertex projected = {screen_x, screen_y, dz};
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
camera3d camera = {100.0f, -2.5f, 100.0f, 0.0f, -1.5f}; 

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
    
    // Forward vector: direction the camera is actually looking (with pitch)
    diagonal_x = -sin_yaw * cos_pitch;  // Negative sin to match view matrix
    diagonal_y = -sin_pitch; // Negative because we want W to move toward where we're looking
    diagonal_z = cos_yaw * cos_pitch;   // Positive cos to match view matrix
    
    // Right vector: perpendicular to forward, always horizontal (no pitch component)
    horizontal_x = cos_yaw;    // Keep positive cos for right direction
    horizontal_y = 0.0f;       // Keep horizontal strafe movement
    horizontal_z = sin_yaw;    // Positive sin for correct right direction
    
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

// Console auto-size detection
void cmd_init() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    
    cmd_buffer_width = csbi.dwSize.X;
    cmd_buffer_height = csbi.dwSize.Y;
    
    // Check if the console buffer size changed
    if (cmd_buffer_width != save_console_width || cmd_buffer_height != save_console_height) {
        system("cls"); // Clear the console if resized
    }
    
    // Update screen dimensions
    screen_width = cmd_buffer_width;
    screen_height = cmd_buffer_height;
    
    save_console_width = cmd_buffer_width;
    save_console_height = cmd_buffer_height;
}

// Aspect ratio functions
void set_aspect_ratio(float width_scale, float height_scale) {
    aspect_ratio_width = width_scale;
    aspect_ratio_height = height_scale;
}

void get_aspect_ratio(float *width_scale, float *height_scale) {
    *width_scale = aspect_ratio_width;
    *height_scale = aspect_ratio_height;
}

// Basic pixel setting with depth testing (1-based console coordinates)
int set_pixel(int x, int y, char ascii, int color, float depth) {
    // Bounds checking for 1-based console coordinates
    if (x < 1 || y < 1 || x > screen_width || y > screen_height || 
        x >= 2560 || y >= 2560) {
        return 0;
    }
    
    // Convert to 0-based indexing for buffer
    int buf_x = x - 1;
    int buf_y = y - 1;
    
    // Depth test - only draw if closer or position is empty
    if (!screen_buffer[buf_y][buf_x].valid || depth < screen_buffer[buf_y][buf_x].depth) {
        screen_buffer[buf_y][buf_x].ascii = ascii;
        screen_buffer[buf_y][buf_x].color = color;
        screen_buffer[buf_y][buf_x].depth = depth;
        screen_buffer[buf_y][buf_x].valid = 1;
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
        // Use projected screen coordinates directly (already in console coordinates)
        int screen_x = (int)(projected.x + 0.5f); // Round to nearest integer
        int screen_y = (int)(projected.y + 0.5f);
        
        set_pixel(screen_x, screen_y, d.ascii, d.color, projected.z);
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
        // Use projected screen coordinates directly
        int x1 = (int)(start_proj.x + 0.5f);
        int y1 = (int)(start_proj.y + 0.5f);
        int x2 = (int)(end_proj.x + 0.5f);
        int y2 = (int)(end_proj.y + 0.5f);
        
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

// Output buffer - using differential rendering like original
static char frame_buffer[2560*2560]; // Main frame buffer for ANSI codes
static int first_call = 1;

void output_buffer() {
    // Auto-detect console size
    cmd_init();
    
    frame_buffer_pos = 0;
    
    // Copy current screen buffer to previous for next frame comparison
    // and build output string with only changed pixels
    for (int y = 0; y < screen_height && y < 2560; y++) {
        for (int x = 0; x < screen_width && x < 2560; x++) {
            pixel current = screen_buffer[y][x];
            pixel previous = previous_screen_buffer[y][x];
            
            // Check if pixel has changed or this is first call
            int pixel_changed = first_call || (current.valid != previous.valid) ||
                               (current.valid && (current.ascii != previous.ascii || 
                                                current.color != previous.color));
            
            if (pixel_changed) {
                if (frame_buffer_pos < sizeof(frame_buffer) - 50) {
                    // Draw new pixel or clear pixel (1-based console coordinates)
                    if (current.valid) {
                        frame_buffer_pos += snprintf(&frame_buffer[frame_buffer_pos], 
                            sizeof(frame_buffer) - frame_buffer_pos,
                            "\x1b[%d;%dH\x1b[%dm%c", y + 1, x + 1, current.color, current.ascii);
                    } else {
                        frame_buffer_pos += snprintf(&frame_buffer[frame_buffer_pos], 
                            sizeof(frame_buffer) - frame_buffer_pos,
                            "\x1b[%d;%dH ", y + 1, x + 1);
                    }
                }
            }
            
            // Copy current to previous for next frame
            previous_screen_buffer[y][x] = current;
        }
    }
    
    // Null terminate and output only if there are changes
    if (frame_buffer_pos > 0 || first_call) {
        if (first_call) {
            system("cls"); // Clear screen on first call
            first_call = 0;
        }
        frame_buffer[frame_buffer_pos] = '\0';
        printf("%s", frame_buffer);
        fflush(stdout); // Ensure immediate output
    }
}

// Main geometry drawing function
void geometry_draw() {
    // Clear buffer with proper depth values
    for (int y = 0; y < screen_height && y < 2560; y++) {
        for (int x = 0; x < screen_width && x < 2560; x++) {
            screen_buffer[y][x].ascii = ' ';
            screen_buffer[y][x].color = 0;
            screen_buffer[y][x].depth = 2560.0f; // Far depth
            screen_buffer[y][x].valid = 0;
        }
    }
    
    // Draw test objects
    draw_test_objects();
}

// Clipping functions
bool is_vertex_in_view_frustum(const vertex& v, float near_plane, float far_plane) {
    // Simple frustum culling - check if vertex is within reasonable bounds
    vertex projected = project_vertex(v, camera.x, camera.y, camera.z, 
                                    camera.yaw, camera.pitch, 90.0f,
                                    (float)screen_width / (float)screen_height, near_plane);
    
    // Check depth bounds
    if (projected.z < near_plane || projected.z > far_plane) return false;
    
    // Check screen bounds with some margin
    if (projected.x < -50 || projected.x > screen_width + 50) return false;
    if (projected.y < -50 || projected.y > screen_height + 50) return false;
    
    return true;
}

bool should_draw_edge(const edge& e) {
    // Check if at least one vertex is in view
    return is_vertex_in_view_frustum(e.start, 0.1f, 100.0f) || is_vertex_in_view_frustum(e.end, 0.1f, 100.0f);
}

bool should_draw_face(const face& f) {
    // Check if any vertex is in view
    for (int i = 0; i < f.vertex_count; i++) {
        if (is_vertex_in_view_frustum(f.vertices[i], 0.1f, 100.0f)) return true;
    }
    return false;
}

// Test objects creation
void create_test_cubes() {
    // We'll create the cubes in the draw function to avoid global state
}

// Draw test objects function
void draw_test_objects() {
    // Cube 1: Face-based cube (colored faces) at position (5, 0, 5)
    vertex cube1_center = {5.0f, 0.0f, 5.0f};
    float size = 2.0f;
    
    // Define cube vertices relative to center
    vertex cube1_vertices[8] = {
        {cube1_center.x - size, cube1_center.y - size, cube1_center.z - size}, // 0: left-bottom-back
        {cube1_center.x + size, cube1_center.y - size, cube1_center.z - size}, // 1: right-bottom-back
        {cube1_center.x + size, cube1_center.y + size, cube1_center.z - size}, // 2: right-top-back
        {cube1_center.x - size, cube1_center.y + size, cube1_center.z - size}, // 3: left-top-back
        {cube1_center.x - size, cube1_center.y - size, cube1_center.z + size}, // 4: left-bottom-front
        {cube1_center.x + size, cube1_center.y - size, cube1_center.z + size}, // 5: right-bottom-front
        {cube1_center.x + size, cube1_center.y + size, cube1_center.z + size}, // 6: right-top-front
        {cube1_center.x - size, cube1_center.y + size, cube1_center.z + size}  // 7: left-top-front
    };
    
    // Draw 6 faces of cube 1 (with different colors)
    face faces[6];
    
    // Front face (red)
    faces[0].vertices[0] = cube1_vertices[4]; faces[0].vertices[1] = cube1_vertices[5];
    faces[0].vertices[2] = cube1_vertices[6]; faces[0].vertices[3] = cube1_vertices[7];
    faces[0].vertex_count = 4; faces[0].color = 91; faces[0].ascii = '#'; // Bright red
    
    // Back face (green)
    faces[1].vertices[0] = cube1_vertices[1]; faces[1].vertices[1] = cube1_vertices[0];
    faces[1].vertices[2] = cube1_vertices[3]; faces[1].vertices[3] = cube1_vertices[2];
    faces[1].vertex_count = 4; faces[1].color = 92; faces[1].ascii = '#'; // Bright green
    
    // Left face (blue)
    faces[2].vertices[0] = cube1_vertices[0]; faces[2].vertices[1] = cube1_vertices[4];
    faces[2].vertices[2] = cube1_vertices[7]; faces[2].vertices[3] = cube1_vertices[3];
    faces[2].vertex_count = 4; faces[2].color = 94; faces[2].ascii = '#'; // Bright blue
    
    // Right face (yellow)
    faces[3].vertices[0] = cube1_vertices[5]; faces[3].vertices[1] = cube1_vertices[1];
    faces[3].vertices[2] = cube1_vertices[2]; faces[3].vertices[3] = cube1_vertices[6];
    faces[3].vertex_count = 4; faces[3].color = 93; faces[3].ascii = '#'; // Bright yellow
    
    // Bottom face (magenta)
    faces[4].vertices[0] = cube1_vertices[0]; faces[4].vertices[1] = cube1_vertices[1];
    faces[4].vertices[2] = cube1_vertices[5]; faces[4].vertices[3] = cube1_vertices[4];
    faces[4].vertex_count = 4; faces[4].color = 95; faces[4].ascii = '#'; // Bright magenta
    
    // Top face (cyan)
    faces[5].vertices[0] = cube1_vertices[3]; faces[5].vertices[1] = cube1_vertices[7];
    faces[5].vertices[2] = cube1_vertices[6]; faces[5].vertices[3] = cube1_vertices[2];
    faces[5].vertex_count = 4; faces[5].color = 96; faces[5].ascii = '#'; // Bright cyan
    
    // Draw faces with clipping
    for (int i = 0; i < 6; i++) {
        if (should_draw_face(faces[i])) {
            draw_face(faces[i]);
        }
    }
    
    // Cube 2: Edge-based cube at position (-5, 0, 5)
    vertex cube2_center = {-5.0f, 0.0f, 5.0f};
    vertex cube2_vertices[8] = {
        {cube2_center.x - size, cube2_center.y - size, cube2_center.z - size}, // 0
        {cube2_center.x + size, cube2_center.y - size, cube2_center.z - size}, // 1
        {cube2_center.x + size, cube2_center.y + size, cube2_center.z - size}, // 2
        {cube2_center.x - size, cube2_center.y + size, cube2_center.z - size}, // 3
        {cube2_center.x - size, cube2_center.y - size, cube2_center.z + size}, // 4
        {cube2_center.x + size, cube2_center.y - size, cube2_center.z + size}, // 5
        {cube2_center.x + size, cube2_center.y + size, cube2_center.z + size}, // 6
        {cube2_center.x - size, cube2_center.y + size, cube2_center.z + size}  // 7
    };
    
    // Define 12 edges with different colors
    edge edges[12] = {
        // Bottom face edges
        {{cube2_vertices[0]}, {cube2_vertices[1]}, '=', 91}, // Red
        {{cube2_vertices[1]}, {cube2_vertices[5]}, '|', 92}, // Green
        {{cube2_vertices[5]}, {cube2_vertices[4]}, '=', 94}, // Blue
        {{cube2_vertices[4]}, {cube2_vertices[0]}, '|', 93}, // Yellow
        // Top face edges
        {{cube2_vertices[3]}, {cube2_vertices[2]}, '=', 95}, // Magenta
        {{cube2_vertices[2]}, {cube2_vertices[6]}, '|', 96}, // Cyan
        {{cube2_vertices[6]}, {cube2_vertices[7]}, '=', 97}, // White
        {{cube2_vertices[7]}, {cube2_vertices[3]}, '|', 90}, // Dark gray
        // Vertical edges
        {{cube2_vertices[0]}, {cube2_vertices[3]}, '+', 91}, // Red
        {{cube2_vertices[1]}, {cube2_vertices[2]}, '+', 92}, // Green
        {{cube2_vertices[5]}, {cube2_vertices[6]}, '+', 94}, // Blue
        {{cube2_vertices[4]}, {cube2_vertices[7]}, '+', 93}  // Yellow
    };
    
    // Draw edges with clipping
    for (int i = 0; i < 12; i++) {
        if (should_draw_edge(edges[i])) {
            draw_edge(edges[i]);
        }
    }
    
    // Cube 3: Dot-based cube at position (0, 0, -5)
    vertex cube3_center = {0.0f, 0.0f, -5.0f};
    vertex cube3_vertices[8] = {
        {cube3_center.x - size, cube3_center.y - size, cube3_center.z - size}, // 0
        {cube3_center.x + size, cube3_center.y - size, cube3_center.z - size}, // 1
        {cube3_center.x + size, cube3_center.y + size, cube3_center.z - size}, // 2
        {cube3_center.x - size, cube3_center.y + size, cube3_center.z - size}, // 3
        {cube3_center.x - size, cube3_center.y - size, cube3_center.z + size}, // 4
        {cube3_center.x + size, cube3_center.y - size, cube3_center.z + size}, // 5
        {cube3_center.x + size, cube3_center.y + size, cube3_center.z + size}, // 6
        {cube3_center.x - size, cube3_center.y + size, cube3_center.z + size}  // 7
    };
    
    // Define 8 dots with different colors and symbols
    dot dots[8] = {
        {{cube3_vertices[0]}, '*', 91}, // Red
        {{cube3_vertices[1]}, '*', 92}, // Green
        {{cube3_vertices[2]}, '*', 94}, // Blue
        {{cube3_vertices[3]}, '*', 93}, // Yellow
        {{cube3_vertices[4]}, 'o', 95}, // Magenta
        {{cube3_vertices[5]}, 'o', 96}, // Cyan
        {{cube3_vertices[6]}, 'o', 97}, // White
        {{cube3_vertices[7]}, 'o', 90}  // Dark gray
    };
    
    // Draw dots with clipping
    for (int i = 0; i < 8; i++) {
        if (is_vertex_in_view_frustum(dots[i].position, 0.1f, 100.0f)) {
            draw_dot(dots[i]);
        }
    }
}

// Console size management
unsigned int save_console_width = 120;
unsigned int save_console_height = 60;

// Mouse sensitivity for camera control
float mouse_sensitivity = 0.003f;
static int center_mouse_x = 200; // Fixed center position
static int center_mouse_y = 200; // Fixed center position
static bool mouse_captured = false;

// Initialize mouse capture for camera control
void init_mouse_camera() {
    // Set mouse to center position
    SetCursorPos(center_mouse_x, center_mouse_y);
    mouse_captured = true;
}

// Set mouse center position for delta calculations
void set_mouse_center(int x, int y) {
    center_mouse_x = x;
    center_mouse_y = y;
    if (mouse_captured) {
        SetCursorPos(center_mouse_x, center_mouse_y);
    }
}

// Update camera based on current mouse position (delta method)
void update_camera_mouse() {
    if (!mouse_captured) return;
    
    // Get current mouse position
    POINT current_pos;
    GetCursorPos(&current_pos);
    
    // Calculate delta from center position
    int delta_x = current_pos.x - center_mouse_x;
    int delta_y = current_pos.y - center_mouse_y;
    
    // Only process if there's actual movement
    if (delta_x != 0 || delta_y != 0) {
        // Update camera yaw and pitch
        camera.yaw -= delta_x * mouse_sensitivity; // Fixed: invert X axis for correct left/right movement
        camera.pitch -= delta_y * mouse_sensitivity; // Invert Y axis
        
        // Clamp pitch
        const float MAX_PITCH = 1.5f;
        if (camera.pitch > MAX_PITCH) camera.pitch = MAX_PITCH;
        if (camera.pitch < -MAX_PITCH) camera.pitch = -MAX_PITCH;
        
        // Reset mouse to center position for next frame
        SetCursorPos(center_mouse_x, center_mouse_y);
        
        // Update camera vectors
        camera_update();
    }
}

// Update camera based on mouse movement with delta and reset
void update_camera_mouse(int mouse_x, int mouse_y) {
    if (!mouse_captured) return;
    
    // Get current mouse position
    POINT current_pos;
    GetCursorPos(&current_pos);
    
    // Calculate delta from center position
    int delta_x = current_pos.x - center_mouse_x;
    int delta_y = current_pos.y - center_mouse_y;
    
    // Only process if there's actual movement
    if (delta_x != 0 || delta_y != 0) {
        // Update camera yaw and pitch
        camera.yaw -= delta_x * mouse_sensitivity; // Fixed: invert X axis for correct left/right movement
        camera.pitch -= delta_y * mouse_sensitivity; // Invert Y axis
        
        // Clamp pitch
        const float MAX_PITCH = 1.5f;
        if (camera.pitch > MAX_PITCH) camera.pitch = MAX_PITCH;
        if (camera.pitch < -MAX_PITCH) camera.pitch = -MAX_PITCH;
        
        // Reset mouse to center position for next frame
        SetCursorPos(center_mouse_x, center_mouse_y);
        
        // Update camera vectors
        camera_update();
    }
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
        camera.y -= camera_speed; // Fixed: up should decrease Y (move up in world space)
    }
    if (down) {
        camera.y += camera_speed; // Fixed: down should increase Y (move down in world space)
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

////////////////////// RenderManager Class Implementation

// Constructor
RenderManager::RenderManager() : m_initialized(false) {
    // Initialize with default values - actual initialization happens in Initialize()
}

// Destructor
RenderManager::~RenderManager() {
    // Cleanup if needed
}

// Initialize the render system
void RenderManager::Initialize() {
    if (m_initialized) return;
    
    cmd_init();
    init_mouse_camera();
    camera_update();
    
    m_initialized = true;
}

// Begin frame - clear buffers
void RenderManager::BeginFrame() {
    if (!m_initialized) return;
    
    geometry_draw(); // This clears the buffer
}

// End frame - present to screen
void RenderManager::EndFrame() {
    if (!m_initialized) return;
    
    output_buffer();
}

// Clear the render buffer
void RenderManager::ClearBuffer() {
    if (!m_initialized) return;
    
    geometry_draw();
}

// Present the frame to screen
void RenderManager::Present() {
    if (!m_initialized) return;
    
    output_buffer();
}

// Drawing functions
void RenderManager::DrawDot(const dot& d) {
    if (!m_initialized) return;
    
    draw_dot(d);
}

void RenderManager::DrawEdge(const edge& e) {
    if (!m_initialized) return;
    
    draw_edge(e);
}

void RenderManager::DrawFace(const face& f) {
    if (!m_initialized) return;
    
    draw_face(f);
}

void RenderManager::DrawRenderable(const renderable& r) {
    if (!m_initialized) return;
    
    switch (r.type) {
        case 0: DrawEdge(r.object.e); break;
        case 1: DrawDot(r.object.d); break;
        case 2: DrawFace(r.object.f); break;
    }
}

// Camera management
void RenderManager::InitializeCamera() {
    if (!m_initialized) return;
    
    init_mouse_camera();
}

void RenderManager::UpdateCamera() {
    if (!m_initialized) return;
    
    camera_update();
}

void RenderManager::SetCameraPosition(float x, float y, float z) {
    camera.x = x;
    camera.y = y;
    camera.z = z;
    UpdateCamera();
}

void RenderManager::SetCameraRotation(float yaw, float pitch) {
    camera.yaw = yaw;
    camera.pitch = pitch;
    UpdateCamera();
}

void RenderManager::GetCameraPosition(float* x, float* y, float* z) {
    if (x) *x = camera.x;
    if (y) *y = camera.y;
    if (z) *z = camera.z;
}

void RenderManager::GetCameraRotation(float* yaw, float* pitch) {
    if (yaw) *yaw = camera.yaw;
    if (pitch) *pitch = camera.pitch;
}

void RenderManager::UpdateCameraFromMouse(int mouse_x, int mouse_y) {
    if (!m_initialized) return;
    
    update_camera_mouse(mouse_x, mouse_y);
}

void RenderManager::UpdateCameraFromMouse() {
    if (!m_initialized) return;
    
    update_camera_mouse(); // Use the new delta version
}

void RenderManager::SetMouseCenter(int x, int y) {
    set_mouse_center(x, y);
}

void RenderManager::MoveCameraKeyboard(bool forward, bool backward, bool left, bool right, bool up, bool down) {
    if (!m_initialized) return;
    
    move_camera(forward, backward, left, right, up, down);
}

// Aspect ratio and screen management
void RenderManager::SetAspectRatio(float width_scale, float height_scale) {
    set_aspect_ratio(width_scale, height_scale);
}

void RenderManager::GetAspectRatio(float* width_scale, float* height_scale) {
    get_aspect_ratio(width_scale, height_scale);
}

void RenderManager::GetScreenDimensions(int* width, int* height) {
    if (width) *width = screen_width;
    if (height) *height = screen_height;
}

// Utility functions
void RenderManager::SetPixel(int x, int y, char ascii, int color, float depth) {
    if (!m_initialized) return;
    
    set_pixel(x, y, ascii, color, depth);
}

vertex RenderManager::ProjectVertex(const vertex& v, float fov, float near_plane) {
    return project_vertex(v, camera.x, camera.y, camera.z, camera.yaw, camera.pitch, 
                         fov, (float)screen_width / (float)screen_height, near_plane);
}

float RenderManager::CalculateDepth(const renderable& r) {
    return calculate_renderable_depth(r);
}

// Configuration
void RenderManager::SetCameraSpeed(float speed) {
    camera_speed = speed;
}

void RenderManager::SetCameraTurnSpeed(float turn_speed) {
    camera_turn_speed = turn_speed;
}

void RenderManager::SetMouseSensitivity(float sensitivity) {
    mouse_sensitivity = sensitivity;
}

// Clipping functions for RenderManager
bool RenderManager::IsVertexInViewFrustum(const vertex& v, float near_plane, float far_plane) {
    return is_vertex_in_view_frustum(v, near_plane, far_plane);
}

bool RenderManager::ShouldDrawEdge(const edge& e) {
    return should_draw_edge(e);
}

bool RenderManager::ShouldDrawFace(const face& f) {
    return should_draw_face(f);
}

// Test objects for RenderManager
void RenderManager::DrawTestObjects() {
    if (!m_initialized) return;
    
    draw_test_objects();
}

// Private helper functions
void RenderManager::UpdateMovementVectors() {
    camera_update();
}