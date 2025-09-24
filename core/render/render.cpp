#include "render.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

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
    screenWidth = std::min(width, MAX_BUFFER_SIZE);
    screenHeight = std::min(height, MAX_BUFFER_SIZE);
    aspectRatio = (float)screenWidth / (float)screenHeight;
    
    // Update clipping planes for new dimensions
    updateClippingPlanes();
}

void RenderManager::setProjectionParameters(float fovDeg, float near, float far) {
    fov = fovDeg;
    nearPlane = near;
    farPlane = far;
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
        clippedFace.vertex_count = std::min((int)clippedVertices.size(), 4);
        
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
