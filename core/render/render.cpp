#include "render.hpp"
#define min(a,b) (((a) < (b)) ? (a) : (b))

float edge_distance_calc(vertex start, vertex end) {
    // Calculate Euclidean distance between two vertices
    float mid_x = (start.x + end.x) / 2.0f;
    float mid_y = (start.y + end.y) / 2.0f;
    float mid_z = (start.z + end.z) / 2.0f;
    float distance = sqrt((mid_x - camera.x) * (mid_x - camera.x) + 
                         (mid_y - camera.y) * (mid_y - camera.y) + 
                         (mid_z - camera.z) * (mid_z - camera.z));
}

float dot_distance_calc(vertex v) {
    // Calculate Euclidean distance from camera to vertex
    float dx = v.x - camera.x;
    float dy = v.y - camera.y;
    float dz = v.z - camera.z;

    return sqrt(dx * dx + dy * dy + dz * dz);
}

char edge_ascii_depth(vertex start, vertex end) {
    float distance = edge_distance_calc(start, end);
    distance *= 5; // Scale distance for more color variation

    // Map distance to discrete intervals
    int range = (int)(distance / 10);

    // Lookup table for ASCII depth
    static const char depth_chars[] = {
        '#', '@', '&', '%', 'M', 'N', '*', '+', '|', '-', ';', ':', '~', '_'
    };
    static const int table_size = sizeof(depth_chars) / sizeof(depth_chars[0]);
    return (range >= 0 && range < table_size) ? depth_chars[range] : '.';
}

int edge_color_depth(vertex start, vertex end) {
    float distance = edge_distance_calc(start, end);
    distance *= 5; // Scale distance for more color variation

    // Map distance to discrete intervals
    int range = (int)(distance / 10);

    // Lookup table for color depth
    static const int color_codes[] = {
        31, 32, 33, 34, 35, 36, 37, 91, 92, 93, 94, 95, 96, 97
    };
    static const int color_table_size = sizeof(color_codes) / sizeof(color_codes[0]);
    return (range >= 0 && range < color_table_size) ? color_codes[range] : 90;
}
edge edge_shader(vertex v1, vertex v2) {
    // Create an edge with depth and color based on the vertices
    edge e;
    e.start = v1;
    e.end = v2;
    e.ascii = edge_ascii_depth(v1, v2);
    e.color = edge_color_depth(v1, v2);
    return e;
}

char dot_ascii_depth(vertex v) {
    float distance = dot_distance_calc(v);
    distance *= 2; // Scale distance for more color variation

    // Map distance to discrete intervals
    int range = (int)(distance / 20);

    // ASCII depth lookup table - much faster than if-else chain
    static const char depth_chars[] = {
        // Dense blocks (0-10)
        '#', '#', '#', '@', '@', '&', '&', '%', '%', '$', '$',
        // Dense letters (11-60)
        'M', 'M', 'W', 'W', 'B', 'B', 'H', 'H', 'R', 'R',
        'K', 'K', 'Q', 'Q', 'U', 'U', 'A', 'A', 'N', 'N',
        'G', 'G', 'D', 'D', 'O', 'O', 'P', 'P', 'S', 'S',
        'E', 'E', 'F', 'F', 'X', 'X', 'Y', 'Y', 'Z', 'Z',
        'V', 'V', 'T', 'T', 'C', 'C', 'I', 'I', 'L', 'L',
        'J', 'J',
        // Lowercase letters (62-114)
        'a', 'a', 'b', 'b', 'c', 'c', 'd', 'd', 'e', 'e',
        'f', 'f', 'g', 'g', 'h', 'h', 'i', 'i', 'j', 'j',
        'k', 'k', 'l', 'l', 'm', 'm', 'n', 'n', 'o', 'o',
        'p', 'p', 'q', 'q', 'r', 'r', 's', 's', 't', 't',
        'u', 'u', 'v', 'v', 'w', 'w', 'x', 'x', 'y', 'y',
        'z', 'z',
        // Numbers (116-134)
        '8', '8', '9', '9', '6', '6', '0', '0', '4', '4',
        '3', '3', '5', '5', '2', '2', '7', '7', '1', '1',
        // Dense symbols (136-140)
        '*', '*', '=', '=', '+', '+',
        // Line symbols (142-154)
        '|', '|', '\\', '\\', '/', '/', '-', '-', '_', '_',
        '^', '^', '~', '~',
        // Punctuation (156-180)
        '!', '!', '?', '?', '<', '<', '>', '>', '{', '{',
        '}', '}', '[', '[', ']', ']', '(', '(', ')', ')',
        ';', ';', ':', ':', ',', ',',
        // Light symbols (182-188)
        '`', '`', '\'', '\'', '"', '"', '.', '.'
    };
    
    // Calculate lookup table size
    static const int table_size = sizeof(depth_chars) / sizeof(depth_chars[0]);
    
    // Return character from lookup table or default for out-of-range
    return (range < table_size) ? depth_chars[range] : '.';
}

int dot_color_depth(vertex v) {
    float distance = dot_distance_calc(v);
    distance *= 2; // Scale distance for more color variation

    // Map distance to discrete intervals
    int range = (int)(distance / 10);

    // Lookup table for color depth
    static const int color_codes[] = {
        31, 32, 33, 34, 35, 36, 37, 91, 92, 93, 94, 95, 96, 97
    };
    static const int color_table_size = sizeof(color_codes) / sizeof(color_codes[0]);
    return (range >= 0 && range < color_table_size) ? color_codes[range] : 90;
}

dot dot_shader(vertex v) {
    // Create a dot with depth and color based on the vertex
    dot d;
    d.position = v;
    d.ascii = dot_ascii_depth(v); // Use same vertex for depth calculation
    d.color = dot_color_depth(v); // Use same vertex for color calculation
    return d;
}

// Rotation shader - changes character based on viewing angle
char edge_rotation_shader(angle e) {
    // Calculate edge direction vector
    float edge_dx = e.a[1].x - e.a[0].x;
    float edge_dy = e.a[1].y - e.a[0].y;
    float edge_dz = e.a[1].z - e.a[0].z;
    
    // Calculate edge midpoint
    float mid_x = (e.a[0].x + e.a[1].x) * 0.5f;
    float mid_y = (e.a[0].y + e.a[1].y) * 0.5f;
    float mid_z = (e.a[0].z + e.a[1].z) * 0.5f;
    
    // Calculate view direction from camera to edge midpoint
    float view_dx = mid_x - camera.x;
    float view_dy = mid_y - camera.y;
    float view_dz = mid_z - camera.z;
    
    // Normalize vectors
    float edge_len = sqrt(edge_dx * edge_dx + edge_dy * edge_dy + edge_dz * edge_dz);
    float view_len = sqrt(view_dx * view_dx + view_dy * view_dy + view_dz * view_dz);
    
    if (edge_len < 0.001f || view_len < 0.001f) return '-'; // Fallback for zero-length vectors
    
    edge_dx /= edge_len;
    edge_dy /= edge_len;
    edge_dz /= edge_len;
    view_dx /= view_len;
    view_dy /= view_len;
    view_dz /= view_len;
    
    // Calculate dot product to get angle between edge and view direction
    float dot_product = edge_dx * view_dx + edge_dy * view_dy + edge_dz * view_dz;
    
    // Calculate cross product magnitude for perpendicular component
    float cross_x = edge_dy * view_dz - edge_dz * view_dy;
    float cross_y = edge_dz * view_dx - edge_dx * view_dz;
    float cross_z = edge_dx * view_dy - edge_dy * view_dx;
    float cross_magnitude = sqrt(cross_x * cross_x + cross_y * cross_y + cross_z * cross_z);
    
    // Calculate angle in radians
    float angle = atan2(cross_magnitude, fabs(dot_product));
    
    // Convert angle to degrees and normalize to 0-360
    float angle_degrees = angle * 180.0f / 3.14159265358979323846f;
    int angle_int = (int)(angle_degrees * 4) % 360; // Multiply by 4 for more sensitivity
    
    // Lookup table for angle-based ASCII characters
    static const char angle_chars[] = {
        '=', '\\', '|', '/', '-', '\\', '|', '/'
    };
    // Each range is 45 degrees, so index = angle_int / 45
    int idx = angle_int / 45;
    if (idx < 0) idx = 0;
    if (idx > 7) idx = 7;
    return angle_chars[idx];
}


edge create_edge_with_shader(vertex start, vertex end) {
    edge edge;
    edge.start = start;
    edge.end = end;
    angle ang;
    ang.a[0] = start;
    ang.a[1] = end;
    edge.ascii = edge_rotation_shader(ang); // Use rotation shader
    edge.color = (rand() % 7) + 31; // Random color between 31 and 37
    return edge;
}

// Calculate distance from camera to face center for depth calculation
float face_distance_calc(face f) {
    // Calculate face center
    float mouse_cursour_x = 0.0f, mouse_cursour_y = 0.0f, center_z = 0.0f;
    for (int i = 0; i < f.vertex_count; i++) {
        mouse_cursour_x += f.vertices[i].x;
        mouse_cursour_y += f.vertices[i].y;
        center_z += f.vertices[i].z;
    }
    mouse_cursour_x /= f.vertex_count;
    mouse_cursour_y /= f.vertex_count;
    center_z /= f.vertex_count;
    
    // Calculate distance from camera to face center
    float dx = mouse_cursour_x - camera.x;
    float dy = mouse_cursour_y - camera.y;
    float dz = center_z - camera.z;
    
    return sqrt(dx * dx + dy * dy + dz * dz);
}

// Depth-based ASCII character selection for faces
char face_ascii_depth(face f) {
    float distance = face_distance_calc(f);
    distance *= 3; // Scale distance for more variation
    
    // Map distance to discrete intervals
    int range = (int)(distance / 10);
    
    // Lookup table for ASCII depth for faces
    static const char face_depth_chars[] = {
        '#', '@', '&', '%', 'M', 'N', '*', '+', '|', '-', ';', ':', '~', '_'
    };
    static const int face_table_size = sizeof(face_depth_chars) / sizeof(face_depth_chars[0]);
    return (range >= 0 && range < face_table_size) ? face_depth_chars[range] : '.';
}

// Depth-based color selection for faces
int face_color_depth(face f) {
    float distance = face_distance_calc(f);
    distance *= 3; // Scale distance for more color variation
    
    // Map distance to discrete intervals
    int range = (int)(distance / 10);
    
    // Lookup table for color depth for faces
    static const int face_color_codes[] = {
        31, 32, 33, 34, 35, 36, 37, 91, 92, 93, 94, 95, 96, 97
    };
    static const int face_color_table_size = sizeof(face_color_codes) / sizeof(face_color_codes[0]);
    return (range >= 0 && range < face_color_table_size) ? face_color_codes[range] : 90;
}

// Calculate face normal vector
void calculate_face_normal(face f, float *nx, float *ny, float *nz) {
    if (f.vertex_count < 3) {
        *nx = *ny = *nz = 0.0f;
        return;
    }
    
    // Use first three vertices to calculate normal
    vertex v0 = f.vertices[0];
    vertex v1 = f.vertices[1]; 
    vertex v2 = f.vertices[2];
    
    // Calculate two edge vectors
    float edge1_x = v1.x - v0.x;
    float edge1_y = v1.y - v0.y;
    float edge1_z = v1.z - v0.z;
    
    float edge2_x = v2.x - v0.x;
    float edge2_y = v2.y - v0.y;
    float edge2_z = v2.z - v0.z;
    
    // Calculate cross product (normal)
    *nx = edge1_y * edge2_z - edge1_z * edge2_y;
    *ny = edge1_z * edge2_x - edge1_x * edge2_z;
    *nz = edge1_x * edge2_y - edge1_y * edge2_x;
    
    // Normalize the normal vector
    float length = sqrt((*nx) * (*nx) + (*ny) * (*ny) + (*nz) * (*nz));
    if (length > 0.001f) {
        *nx /= length;
        *ny /= length;
        *nz /= length;
    }
}

// Rotation shader for faces - changes character based on viewing angle
char face_rotation_shader(face f) {
    // Calculate face normal
    float nx, ny, nz;
    calculate_face_normal(f, &nx, &ny, &nz);
    
    // Calculate face center
    float mouse_cursour_x = 0.0f, mouse_cursour_y = 0.0f, center_z = 0.0f;
    for (int i = 0; i < f.vertex_count; i++) {
        mouse_cursour_x += f.vertices[i].x;
        mouse_cursour_y += f.vertices[i].y;
        center_z += f.vertices[i].z;
    }
    mouse_cursour_x /= f.vertex_count;
    mouse_cursour_y /= f.vertex_count;
    center_z /= f.vertex_count;
    
    // Calculate view direction from camera to face center
    float view_dx = mouse_cursour_x - camera.x;
    float view_dy = mouse_cursour_y - camera.y;
    float view_dz = center_z - camera.z;
    
    // Normalize view direction
    float view_len = sqrt(view_dx * view_dx + view_dy * view_dy + view_dz * view_dz);
    if (view_len < 0.001f) return '#'; // Fallback for zero-length vector
    
    view_dx /= view_len;
    view_dy /= view_len;
    view_dz /= view_len;
    
    // Calculate dot product between face normal and view direction
    float dot_product = nx * view_dx + ny * view_dy + nz * view_dz;
    
    // Calculate angle between normal and view direction
    float angle = acos(fabs(dot_product)); // Use absolute value for angle
    float angle_degrees = angle * 180.0f / 3.14159265358979323846f;
    
    // Lookup table for angle-based ASCII characters for faces
    static const char face_angle_chars[] = {
        '@', '#', '&', '%', '$', 'M', 'N', '*', '+', '|', '-', ';', ':', '~', '_'
    };
    // Each range is 5 degrees, so index = angle_degrees / 5
    int idx = (int)(angle_degrees / 5.0f);
    int table_size = sizeof(face_angle_chars) / sizeof(face_angle_chars[0]);
    return (idx >= 0 && idx < table_size) ? face_angle_chars[idx] : '_';
}

// Combined face shader using both depth and rotation
face face_shader(face f) {
    // Create a new face with shaded properties
    face shaded_face = f; // Copy original face structure
    
    // Apply depth-based shading
    shaded_face.ascii = face_ascii_depth(f);
    shaded_face.color = face_color_depth(f);
    
    return shaded_face;
}

// Face shader using rotation-based character selection
face face_rotation_shader_face(face f) {
    // Create a new face with rotation-based shading
    face shaded_face = f; // Copy original face structure
    
    // Apply rotation-based character selection
    shaded_face.ascii = face_rotation_shader(f);
    
    // Use distance-based color but keep rotation-based character
    shaded_face.color = face_color_depth(f);
    
    return shaded_face;
}

// Create face with combined depth and rotation shading
face create_face_with_shader(vertex *vertices, int vertex_count, int *texture, int texture_width, int texture_height) {
    face f;
    f.vertex_count = vertex_count;
    
    // Copy vertices
    for (int i = 0; i < vertex_count && i < 4; i++) {
        f.vertices[i] = vertices[i];
    }
    
    // Set texture properties
    f.texture = texture;
    f.texture_width = texture_width;
    f.texture_height = texture_height;
    
    // Apply rotation-based shading for character
    f.ascii = face_rotation_shader(f);
    
    // Apply depth-based shading for color
    f.color = face_color_depth(f);
    
    return f;
}





// Start camera back from origin
camera3d camera = {100.0f, -2.5f, 100.0f, 0.0f, -1.5f}; 

// Aspect ratio correction for console character stretching
float aspect_ratio_width = 1.0f;  // Width scaling factor (1.0 = no scaling)
float aspect_ratio_height = 2.0f; // Height scaling factor (2.0 = compensate for tall characters)

// Culling distance for 3D objects
const float culling_distance = .5f; // Distance behind camera to start culling
const float view_distance = 100000.0f; // Maximum view distance - objects beyond this distance are culled

// Diagonal vector for camera movement (forward/backward)
float diagonal_x, diagonal_y, diagonal_z;

// Horizontal vector for camera movement (left/right)
float horizontal_x, horizontal_y, horizontal_z;

// Camera movement controls (WASD) - aligned with mouse look direction
float camera_speed = 0.1f;

// Camera turning speed (Q/E for left/right, R/F for up/down)
float camera_turn_speed = 0.1f;


camera_cache cached_transform = {0};

// Update camera transformation cache
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

// Check if camera cache is valid
int is_camera_cache_valid() {
    return cached_transform.valid &&
           cached_transform.cam_x == camera.x &&
           cached_transform.cam_y == camera.y &&
           cached_transform.cam_z == camera.z &&
           cached_transform.cam_yaw == camera.yaw &&
           cached_transform.cam_pitch == camera.pitch;
}


void camera_update() {
    // Calculate movement directions based on camera orientation (mouse look direction)
    // Forward direction: exactly where the camera is looking (includes pitch)
    float cos_yaw = cos(camera.yaw);
    float sin_yaw = sin(camera.yaw);
    float cos_pitch = cos(camera.pitch);
    float sin_pitch = sin(camera.pitch);
    
    // Forward vector: direction the camera is actually looking (with pitch)
    // Fixed: Match the camera view matrix coordinate system (uses -camera.yaw)
    diagonal_x = -sin_yaw * cos_pitch;  // Negative sin to match view matrix
    diagonal_y = -sin_pitch; // Negative because we want W to move toward where we're looking
    diagonal_z = cos_yaw * cos_pitch;   // Positive cos to match view matrix
    
    // Right vector: perpendicular to forward, always horizontal (no pitch component)
    // Fixed: Use standard 3D camera right vector calculation
    horizontal_x = cos_yaw;    // Keep positive cos for right direction
    horizontal_y = 0.0f;       // Keep horizontal strafe movement
    horizontal_z = sin_yaw;    // Positive sin for correct right direction
    
    
    // Clamp pitch to prevent camera flipping
    const float MAX_PITCH = 1.5f; // About 85 degrees
    if (camera.pitch > MAX_PITCH) camera.pitch = MAX_PITCH;
    if (camera.pitch < -MAX_PITCH) camera.pitch = -MAX_PITCH;
}




// Global instance for compatibility
RenderManager* g_renderManager = nullptr;

// External variables from existing system
extern unsigned int cmd_buffer_width;
extern unsigned int cmd_buffer_height;
extern float aspect_ratio_width;
extern float aspect_ratio_height;
extern const float culling_distance;
extern const float view_distance;

RenderManager::RenderManager(int width, int height)
    : screenWidth(width), screenHeight(height), pixelsDrawn(0), objectsCulled(0), objectsClipped(0),
      fov(90.0f), nearPlane(0.1f), farPlane(1000.0f), aspectRatio(1.0f) {
    initialize();
}

RenderManager::~RenderManager() {
    clearObjects();
}

void RenderManager::initialize() {
    // Clear buffers
    clearBuffers();
    
    // Initialize clipping planes
    initializeClippingPlanes();
    
    // Set default aspect ratio
    aspectRatio = (float)screenWidth / (float)screenHeight;
}



void RenderManager::resize(int width, int height) {
    screenWidth = min(width, MAX_BUFFER_SIZE);
    screenHeight = min(height, MAX_BUFFER_SIZE);
    aspectRatio = (float)screenWidth / (float)screenHeight;
    
    // Update clipping planes for new dimensions
    updateClippingPlanes();
}

void RenderManager::setProjectionParameters(float fovDeg, float nearVal, float farVal) {
    fov = fovDeg;
    nearPlane = nearVal;
    farPlane = farVal;
    updateClippingPlanes();
}

void RenderManager::initializeClippingPlanes() {
    clippingPlanes.clear();
    updateClippingPlanes();
}

void RenderManager::updateClippingPlanes() {
    clippingPlanes.clear();
    
    // Calculate frustum parameters
    float fovRad = fov * M_PI / 180.0f;
    float halfHeight = tan(fovRad * 0.5f) * nearPlane;
    float halfWidth = halfHeight * aspectRatio;
    
    // Apply aspect ratio correction
    halfWidth *= aspect_ratio_width;
    halfHeight *= aspect_ratio_height;
    
    // Create clipping planes in view space
    // Near plane: z = -nearPlane
    clippingPlanes.emplace_back(0.0f, 0.0f, 1.0f, nearPlane);
    
    // Far plane: z = -farPlane  
    clippingPlanes.emplace_back(0.0f, 0.0f, -1.0f, farPlane);
    
    // Left plane: normalize(nearPlane, 0, halfWidth)
    float leftLen = sqrt(nearPlane * nearPlane + halfWidth * halfWidth);
    clippingPlanes.emplace_back(nearPlane / leftLen, 0.0f, halfWidth / leftLen, 0.0f);
    
    // Right plane: normalize(-nearPlane, 0, halfWidth)
    float rightLen = sqrt(nearPlane * nearPlane + halfWidth * halfWidth);
    clippingPlanes.emplace_back(-nearPlane / rightLen, 0.0f, halfWidth / rightLen, 0.0f);
    
    // Bottom plane: normalize(0, nearPlane, halfHeight)
    float bottomLen = sqrt(nearPlane * nearPlane + halfHeight * halfHeight);
    clippingPlanes.emplace_back(0.0f, nearPlane / bottomLen, halfHeight / bottomLen, 0.0f);
    
    // Top plane: normalize(0, -nearPlane, halfHeight)
    float topLen = sqrt(nearPlane * nearPlane + halfHeight * halfHeight);
    clippingPlanes.emplace_back(0.0f, -nearPlane / topLen, halfHeight / topLen, 0.0f);
}

void RenderManager::clearBuffers() {
    // Clear screen buffer
    for (int y = 0; y < MAX_BUFFER_SIZE; y++) {
        for (int x = 0; x < MAX_BUFFER_SIZE; x++) {
            screenBuffer[y][x] = Pixel();
        }
    }
}

void RenderManager::copyCurrentToPrevious() {
    // Copy current buffer to previous for differential rendering
    for (int y = 0; y < screenHeight && y < MAX_BUFFER_SIZE; y++) {
        for (int x = 0; x < screenWidth && x < MAX_BUFFER_SIZE; x++) {
            previousBuffer[y][x] = screenBuffer[y][x];
        }
    }
}

void RenderManager::addDot(const dot& d) {
    auto obj = std::make_unique<RenderObject>(d);
    calculateDepth(*obj);
    renderObjects.push_back(std::move(obj));
}

void RenderManager::addEdge(const edge& e) {
    auto obj = std::make_unique<RenderObject>(e);
    calculateDepth(*obj);
    renderObjects.push_back(std::move(obj));
}

void RenderManager::addFace(const face& f) {
    auto obj = std::make_unique<RenderObject>(f);
    calculateDepth(*obj);
    renderObjects.push_back(std::move(obj));
}

void RenderManager::addMesh(std::unique_ptr<Mesh> mesh) {
    // Convert mesh faces to render objects
    for (const auto& face : mesh->faces) {
        addFace(face);
    }
    
    // Store mesh for future reference
    meshes.push_back(std::move(mesh));
}

void RenderManager::clearObjects() {
    renderObjects.clear();
    meshes.clear();
}

void RenderManager::calculateDepth(RenderObject& obj) {
    // Use existing depth calculation system for compatibility
    switch (obj.type) {
        case ObjectType::DOT: {
            // Transform dot position to camera space like existing system
            float dx = obj.dotData.position.x - camera.x;
            float dy = obj.dotData.position.y - camera.y;
            float dz = obj.dotData.position.z - camera.z;
            
            // Apply camera rotation (same as existing system)
            if (!is_camera_cache_valid()) {
                update_camera_cache();
            }
            
            float temp_x = dx * cached_transform.cos_yaw - dz * cached_transform.sin_yaw;
            float temp_z = dx * cached_transform.sin_yaw + dz * cached_transform.cos_yaw;
            dx = temp_x;
            dz = temp_z;
            
            float temp_y = dy * cached_transform.cos_pitch - dz * cached_transform.sin_pitch;
            temp_z = dy * cached_transform.sin_pitch + dz * cached_transform.cos_pitch;
            
            obj.depth = temp_z;
            break;
        }
        
        case ObjectType::EDGE: {
            // Use existing edge depth calculation
            obj.depth = calculate_edge_depth(obj.edgeData);
            break;
        }
        
        case ObjectType::FACE: {
            // Calculate face center depth (same approach as existing system)
            float total_x = 0.0f, total_y = 0.0f, total_z = 0.0f;
            for (int i = 0; i < obj.faceData.vertex_count; i++) {
                total_x += obj.faceData.vertices[i].x;
                total_y += obj.faceData.vertices[i].y;
                total_z += obj.faceData.vertices[i].z;
            }
            float avg_x = total_x / obj.faceData.vertex_count;
            float avg_y = total_y / obj.faceData.vertex_count;
            float avg_z = total_z / obj.faceData.vertex_count;
            
            // Transform to camera space
            float dx = avg_x - camera.x;
            float dy = avg_y - camera.y;
            float dz = avg_z - camera.z;
            
            if (!is_camera_cache_valid()) {
                update_camera_cache();
            }
            
            float temp_x = dx * cached_transform.cos_yaw - dz * cached_transform.sin_yaw;
            float temp_z = dx * cached_transform.sin_yaw + dz * cached_transform.cos_yaw;
            dx = temp_x;
            dz = temp_z;
            
            float temp_y = dy * cached_transform.cos_pitch - dz * cached_transform.sin_pitch;
            temp_z = dy * cached_transform.sin_pitch + dz * cached_transform.cos_pitch;
            
            obj.depth = temp_z;
            break;
        }
        
        default:
            obj.depth = 1000000.0f;
            break;
    }
}

bool RenderManager::isObjectVisible(const RenderObject& obj) {
    // Basic culling (maintains existing system's culling)
    if (obj.depth < culling_distance || obj.depth > view_distance) {
        return false;
    }
    
    // Additional clipping checks for vertices
    switch (obj.type) {
        case ObjectType::DOT: {
            vertex projected = projectVertex(obj.dotData.position);
            return isVertexInView(projected);
        }
        
        case ObjectType::EDGE: {
            vertex startProj = projectVertex(obj.edgeData.start);
            vertex endProj = projectVertex(obj.edgeData.end);
            return isVertexInView(startProj) || isVertexInView(endProj);
        }
        
        case ObjectType::FACE: {
            // Check if any vertex is visible
            for (int i = 0; i < obj.faceData.vertex_count; i++) {
                vertex projected = projectVertex(obj.faceData.vertices[i]);
                if (isVertexInView(projected)) {
                    return true;
                }
            }
            return false;
        }
        
        default:
            return false;
    }
}

vertex RenderManager::projectVertex(const vertex& v) {
    // Use existing projection system for compatibility
    return project_vertex(v, camera.x, camera.y, camera.z, camera.yaw, camera.pitch, 
                         fov, aspectRatio, nearPlane);
}

bool RenderManager::isVertexInView(const vertex& v) {
    // Check if projected vertex is within screen bounds
    return v.x >= 1 && v.x <= screenWidth && v.y >= 1 && v.y <= screenHeight && v.z > 0;
}

float RenderManager::calculateDistance(const vertex& v) {
    float dx = v.x - camera.x;
    float dy = v.y - camera.y;  
    float dz = v.z - camera.z;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

void RenderManager::sortObjectsByDepth() {
    // Sort objects back to front for painter's algorithm (same as existing system)
    std::sort(renderObjects.begin(), renderObjects.end(), 
        [](const std::unique_ptr<RenderObject>& a, const std::unique_ptr<RenderObject>& b) {
            return a->depth > b->depth; // Farther objects first
        });
}

bool RenderManager::clipLine(vertex& start, vertex& end) {
    // Cohen-Sutherland line clipping algorithm adapted for 3D
    vertex originalStart = start;
    vertex originalEnd = end;
    
    // Transform to camera space first
    vertex camStart, camEnd;
    
    // Apply camera transformation (same as existing system)
    float dx1 = start.x - camera.x;
    float dy1 = start.y - camera.y;
    float dz1 = start.z - camera.z;
    
    float dx2 = end.x - camera.x;
    float dy2 = end.y - camera.y;
    float dz2 = end.z - camera.z;
    
    if (!is_camera_cache_valid()) {
        update_camera_cache();
    }
    
    // Rotate start point
    float temp_x = dx1 * cached_transform.cos_yaw - dz1 * cached_transform.sin_yaw;
    float temp_z = dx1 * cached_transform.sin_yaw + dz1 * cached_transform.cos_yaw;
    dx1 = temp_x;
    dz1 = temp_z;
    
    float temp_y = dy1 * cached_transform.cos_pitch - dz1 * cached_transform.sin_pitch;
    temp_z = dy1 * cached_transform.sin_pitch + dz1 * cached_transform.cos_pitch;
    dy1 = temp_y;
    dz1 = temp_z;
    
    // Rotate end point
    temp_x = dx2 * cached_transform.cos_yaw - dz2 * cached_transform.sin_yaw;
    temp_z = dx2 * cached_transform.sin_yaw + dz2 * cached_transform.cos_yaw;
    dx2 = temp_x;
    dz2 = temp_z;
    
    temp_y = dy2 * cached_transform.cos_pitch - dz2 * cached_transform.sin_pitch;
    temp_z = dy2 * cached_transform.sin_pitch + dz2 * cached_transform.cos_pitch;
    dy2 = temp_y;
    dz2 = temp_z;
    
    camStart.x = dx1; camStart.y = dy1; camStart.z = dz1;
    camEnd.x = dx2; camEnd.y = dy2; camEnd.z = dz2;
    
    // Clip against each plane
    for (const auto& plane : clippingPlanes) {
        float startDist = plane.distanceToPoint(camStart);
        float endDist = plane.distanceToPoint(camEnd);
        
        // Both points outside
        if (startDist > 0 && endDist > 0) {
            return false;
        }
        
        // Need to clip
        if (startDist * endDist < 0) {
            float t = startDist / (startDist - endDist);
            
            if (startDist > 0) {
                // Start point is outside, clip it
                camStart.x = camStart.x + t * (camEnd.x - camStart.x);
                camStart.y = camStart.y + t * (camEnd.y - camStart.y);
                camStart.z = camStart.z + t * (camEnd.z - camStart.z);
            } else {
                // End point is outside, clip it
                camEnd.x = camStart.x + t * (camEnd.x - camStart.x);
                camEnd.y = camStart.y + t * (camEnd.y - camStart.y);
                camEnd.z = camStart.z + t * (camEnd.z - camStart.z);
            }
        }
    }
    
    // Transform back to world space (inverse transformation)
    // For now, just project the clipped camera space points
    start = projectVertex(camStart);
    end = projectVertex(camEnd);
    
    return true;
}

std::vector<vertex> RenderManager::clipPolygon(const std::vector<vertex>& polygon) {
    if (polygon.empty()) return {};
    
    std::vector<vertex> clipped = polygon;
    
    // Sutherland-Hodgman polygon clipping against each plane
    for (const auto& plane : clippingPlanes) {
        if (clipped.empty()) break;
        
        std::vector<vertex> input = clipped;
        clipped.clear();
        
        if (input.empty()) continue;
        
        vertex prev = input.back();
        
        for (const auto& curr : input) {
            bool currInside = plane.isInside(curr);
            bool prevInside = plane.isInside(prev);
            
            if (currInside) {
                if (!prevInside) {
                    // Entering - add intersection
                    float prevDist = plane.distanceToPoint(prev);
                    float currDist = plane.distanceToPoint(curr);
                    float t = prevDist / (prevDist - currDist);
                    
                    vertex intersection = clipLineToPlane(prev, curr, plane, t);
                    clipped.push_back(intersection);
                }
                // Add current point
                clipped.push_back(curr);
            } else if (prevInside) {
                // Exiting - add intersection only
                float prevDist = plane.distanceToPoint(prev);
                float currDist = plane.distanceToPoint(curr);
                float t = prevDist / (prevDist - currDist);
                
                vertex intersection = clipLineToPlane(prev, curr, plane, t);
                clipped.push_back(intersection);
            }
            
            prev = curr;
        }
    }
    
    return clipped;
}

vertex RenderManager::clipLineToPlane(const vertex& start, const vertex& end, 
                                     const ClippingPlane& plane, float t) {
    vertex result;
    result.x = start.x + t * (end.x - start.x);
    result.y = start.y + t * (end.y - start.y);
    result.z = start.z + t * (end.z - start.z);
    return result;
}

void RenderManager::drawPixel(int x, int y, char ascii, int color, float depth) {
    // Use existing set_pixel function for compatibility and performance
    set_pixel(x, y, ascii, color, depth);
}

void RenderManager::drawDot(const dot& d) {
    // Use existing draw_dot function for compatibility
    draw_dot(d);
}

void RenderManager::drawEdge(const edge& e) {
    // Check if edge needs clipping
    vertex start = e.start;
    vertex end = e.end;
    
    if (clipLine(start, end)) {
        // Create clipped edge and draw it
        edge clippedEdge = e;
        clippedEdge.start = start;
        clippedEdge.end = end;
        draw_edge(clippedEdge);
    } else {
        objectsClipped++;
    }
}

void RenderManager::drawFace(const face& f) {
    // Convert face vertices to vector for clipping
    std::vector<vertex> vertices;
    for (int i = 0; i < f.vertex_count; i++) {
        vertices.push_back(f.vertices[i]);
    }
    
    // Clip the polygon
    std::vector<vertex> clippedVertices = clipPolygon(vertices);
    
    if (clippedVertices.size() >= 3) {
        // Create new face with clipped vertices
        face clippedFace = f;
        clippedFace.vertex_count = min((int)clippedVertices.size(), 4);
        
        for (int i = 0; i < clippedFace.vertex_count; i++) {
            clippedFace.vertices[i] = clippedVertices[i];
        }
        
        // Use existing draw_face function
        draw_face(clippedFace);
    } else {
        objectsClipped++;
    }
}

void RenderManager::beginFrame() {
    // Copy current buffer to previous
    copyCurrentToPrevious();
    
    // Clear current buffer
    clearBuffers();
    
    // Reset performance counters
    pixelsDrawn = 0;
    objectsCulled = 0;
    objectsClipped = 0;
    
    // Update camera cache if needed
    if (!is_camera_cache_valid()) {
        update_camera_cache();
    }
    
    // Update clipping planes based on current camera
    updateClippingPlanes();
}

void RenderManager::render() {
    // Update depths for all objects
    for (auto& obj : renderObjects) {
        calculateDepth(*obj);
        obj->visible = isObjectVisible(*obj);
    }
    
    // Sort objects by depth (back to front)
    sortObjectsByDepth();
    
    // Draw all visible objects
    for (const auto& obj : renderObjects) {
        if (!obj->visible) {
            objectsCulled++;
            continue;
        }
        
        switch (obj->type) {
            case ObjectType::DOT:
                drawDot(obj->dotData);
                break;
            case ObjectType::EDGE:
                drawEdge(obj->edgeData);
                break;
            case ObjectType::FACE:
                drawFace(obj->faceData);
                break;
        }
    }
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

void RenderManager::endFrame() {
    // Use existing render_frame_buffer function for output
    render_frame_buffer();
}

void RenderManager::present() {
    // This would be called after endFrame() if needed
    // Currently handled by render_frame_buffer()
}

void RenderManager::updateCamera() {
    // Use existing camera_update function
    camera_update();
}

void RenderManager::getPerformanceStats(int& pixels, int& culled, int& clipped) {
    pixels = pixelsDrawn;
    culled = objectsCulled;  
    clipped = objectsClipped;
}

void RenderManager::enableWireframe(bool enabled) {
    // Future implementation for wireframe mode
}

void RenderManager::setViewDistance(float distance) {
    farPlane = distance;
    updateClippingPlanes();
}

std::unique_ptr<Mesh> RenderManager::loadObjModel(const std::string& filename) {
    // Future .obj loader implementation
    return std::make_unique<Mesh>();
}

void RenderManager::setMeshTexture(Mesh* mesh, int* textureData, int width, int height) {
    if (mesh) {
        mesh->textureData = textureData;
        mesh->textureWidth = width;
        mesh->textureHeight = height;
        mesh->hasTexture = true;
    }
}

void RenderManager::drawUnified(edge* edges, int edgeCount, dot* dots, int dotCount, 
                               face* faces, int faceCount) {
    // Clear current objects
    clearObjects();
    
    // Add all objects to render queue
    for (int i = 0; i < dotCount; i++) {
        addDot(dots[i]);
    }
    
    for (int i = 0; i < edgeCount; i++) {
        addEdge(edges[i]);
    }
    
    for (int i = 0; i < faceCount; i++) {
        addFace(faces[i]);
    }
    
    // Render frame
    beginFrame();
    render();
    endFrame();
}

void RenderManager::setAspectRatio(float widthScale, float heightScale) {
    aspect_ratio_width = widthScale;
    aspect_ratio_height = heightScale;
    updateClippingPlanes();
}

void RenderManager::getAspectRatio(float* widthScale, float* heightScale) {
    *widthScale = aspect_ratio_width;
    *heightScale = aspect_ratio_height;
}

// C-style wrapper functions for compatibility
extern "C" {
    void render_manager_init(int width, int height) {
        if (g_renderManager) {
            delete g_renderManager;
        }
        g_renderManager = new RenderManager(width, height);
    }
    
    void render_manager_cleanup() {
        if (g_renderManager) {
            delete g_renderManager;
            g_renderManager = nullptr;
        }
    }
    
    void render_manager_add_dot(dot d) {
        if (g_renderManager) {
            g_renderManager->addDot(d);
        }
    }
    
    void render_manager_add_edge(edge e) {
        if (g_renderManager) {
            g_renderManager->addEdge(e);
        }
    }
    
    void render_manager_add_face(face f) {
        if (g_renderManager) {
            g_renderManager->addFace(f);
        }
    }
    
    void render_manager_render() {
        if (g_renderManager) {
            g_renderManager->beginFrame();
            g_renderManager->render();
            g_renderManager->endFrame();
        }
    }
    
    void render_manager_draw_unified(edge* edges, int edgeCount, dot* dots, int dotCount, 
                                   face* faces, int faceCount) {
        if (g_renderManager) {
            g_renderManager->drawUnified(edges, edgeCount, dots, dotCount, faces, faceCount);
        }
    }
}
