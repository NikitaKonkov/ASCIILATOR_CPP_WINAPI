#include <math.h>
#include <stdlib.h>
#include "SHADER.h"
#include "../camera/CAMERA.h"

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
    edge.ascii = edge_rotation_shader((angle){start, end}); // Use rotation shader
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

