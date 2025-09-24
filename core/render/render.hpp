#ifndef RENDER_HPP
#define RENDER_HPP

#include <vector>
#include <memory>
#include <string>
#include "SHADER.h"
#include "CAMERA.h"

// Forward declarations
struct RenderObject;
struct ClippingPlane;

// Pixel structure for frame buffer (compatible with existing RASTERIZER)
struct Pixel {
    char ascii;
    int color;
    float depth;
    bool valid;
    
    Pixel() : ascii(' '), color(0), depth(1000000.0f), valid(false) {}
};

// Clipping planes for view frustum
struct ClippingPlane {
    float a, b, c, d; // Plane equation: ax + by + cz + d = 0
    
    ClippingPlane(float a, float b, float c, float d) : a(a), b(b), c(c), d(d) {}
    
    // Calculate signed distance from point to plane
    float distanceToPoint(const vertex& v) const {
        return a * v.x + b * v.y + c * v.z + d;
    }
    
    // Check if point is inside (negative distance)
    bool isInside(const vertex& v) const {
        return distanceToPoint(v) <= 0.0f;
    }
};

// Renderable object types
enum class ObjectType {
    DOT,
    EDGE,
    FACE,
    MESH // For future .obj model support
};

// Base renderable object
struct RenderObject {
    ObjectType type;
    float depth;
    bool visible;
    
    union {
        dot dotData;
        edge edgeData;
        face faceData;
    };
    
    RenderObject(const dot& d) : type(ObjectType::DOT), depth(0.0f), visible(true) {
        dotData = d;
    }
    
    RenderObject(const edge& e) : type(ObjectType::EDGE), depth(0.0f), visible(true) {
        edgeData = e;
    }
    
    RenderObject(const face& f) : type(ObjectType::FACE), depth(0.0f), visible(true) {
        faceData = f;
    }
};

// Mesh object for future .obj support
struct Mesh {
    std::vector<vertex> vertices;
    std::vector<face> faces;
    std::vector<edge> wireframe;
    std::string name;
    bool hasTexture;
    int* textureData;
    int textureWidth, textureHeight;
    
    Mesh() : hasTexture(false), textureData(nullptr), textureWidth(0), textureHeight(0) {}
};

// Main render manager class
class RenderManager {
private:
    // Frame buffer system (maintains compatibility with existing system)
    static const int MAX_BUFFER_SIZE = 2560;
    Pixel screenBuffer[MAX_BUFFER_SIZE][MAX_BUFFER_SIZE];
    Pixel previousBuffer[MAX_BUFFER_SIZE][MAX_BUFFER_SIZE];
    
    int screenWidth;
    int screenHeight;
    
    // Rendering objects
    std::vector<std::unique_ptr<RenderObject>> renderObjects;
    std::vector<std::unique_ptr<Mesh>> meshes;
    
    // View frustum clipping planes
    std::vector<ClippingPlane> clippingPlanes;
    
    // Performance metrics
    int pixelsDrawn;
    int objectsCulled;
    int objectsClipped;
    
    // Projection parameters
    float fov;
    float nearPlane;
    float farPlane;
    float aspectRatio;
    
    // Private helper methods
    void initializeClippingPlanes();
    void updateClippingPlanes();
    bool isObjectVisible(const RenderObject& obj);
    bool clipLine(vertex& start, vertex& end);
    std::vector<vertex> clipPolygon(const std::vector<vertex>& polygon);
    vertex clipLineToPlane(const vertex& start, const vertex& end, const ClippingPlane& plane, float t);
    void calculateDepth(RenderObject& obj);
    void sortObjectsByDepth();
    
    // Drawing methods (maintains compatibility with existing system)
    void drawPixel(int x, int y, char ascii, int color, float depth);
    void drawDot(const dot& d);
    void drawEdge(const edge& e);
    void drawFace(const face& f);
    void drawClippedEdge(vertex start, vertex end, char ascii, int color);
    void drawClippedFace(const std::vector<vertex>& clippedVertices, char ascii, int color);
    
    // Buffer management
    void clearBuffers();
    void copyCurrentToPrevious();
    
public:
    // Constructor/Destructor
    RenderManager(int width = 120, int height = 60);
    ~RenderManager();
    
    // Buffer management
    void initialize();
    void resize(int width, int height);
    void setProjectionParameters(float fov, float near, float far);
    
    // Object management
    void addDot(const dot& d);
    void addEdge(const edge& e);
    void addFace(const face& f);
    void addMesh(std::unique_ptr<Mesh> mesh);
    void clearObjects();
    
    // Rendering pipeline
    void beginFrame();
    void render();
    void endFrame();
    void present();
    
    // Camera integration (uses existing camera system)
    void updateCamera();
    
    // Utility methods
    vertex projectVertex(const vertex& v);
    bool isVertexInView(const vertex& v);
    float calculateDistance(const vertex& v);
    
    // Performance and debugging
    void getPerformanceStats(int& pixels, int& culled, int& clipped);
    void enableWireframe(bool enabled);
    void setViewDistance(float distance);
    
    // Future .obj model support preparation
    std::unique_ptr<Mesh> loadObjModel(const std::string& filename);
    void setMeshTexture(Mesh* mesh, int* textureData, int width, int height);
    
    // Compatibility with existing system
    void drawUnified(edge* edges, int edgeCount, dot* dots, int dotCount, face* faces, int faceCount);
    void setAspectRatio(float widthScale, float heightScale);
    void getAspectRatio(float* widthScale, float* heightScale);
};

// Global instance for compatibility with existing C code
extern RenderManager* g_renderManager;

// C-style wrapper functions for compatibility
extern "C" {
    void render_manager_init(int width, int height);
    void render_manager_cleanup();
    void render_manager_add_dot(dot d);
    void render_manager_add_edge(edge e);
    void render_manager_add_face(face f);
    void render_manager_render();
    void render_manager_draw_unified(edge* edges, int edgeCount, dot* dots, int dotCount, face* faces, int faceCount);
}


#include "SHADER.h"
#include <math.h>
#define M_PI 3.14159265358979323846
#define HASH_MAP_SIZE 256*256  // 512^2 Distribution for hash map

// Buffer positions for drawing
extern int frame_buffer_pos;

// Frame buffer pixel structure
typedef struct {
    char ascii;
    int color;
    float depth;
    int valid;  // 1 if pixel is drawn, 0 if empty
} pixel;

// 2D frame buffer for pixel-perfect rendering
extern pixel screen_buffer[2560][2560];
extern pixel previous_screen_buffer[2560][2560];  // Previous frame buffer for comparison
extern int screen_width;
extern int screen_height;

// Buffer dimensions
extern unsigned int cmd_buffer_width; 
extern unsigned int cmd_buffer_height;



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


int set_pixel(int x, int y, char ascii, int color, float depth);
void set_aspect_ratio(float width_scale, float height_scale);
void get_aspect_ratio(float *width_scale, float *height_scale);
float calculate_dot_distance(dot d);
float calculate_edge_distance(edge e);
float calculate_face_distance(face f);
vertex project_vertex(vertex v, float cam_x, float cam_y, float cam_z, float cam_yaw, float cam_pitch, float fov, float aspect_ratio, float near_plane);
float calculate_edge_depth(edge e);
float calculate_renderable_depth(renderable r);
int compare_renderables_by_depth(const void *a, const void *b);
void draw_dot(dot d);
void draw_edge(edge e);
void draw_face(face f);



// Saves current console size to avoid infinite scrolling
extern unsigned int save_console_width;
extern unsigned int save_console_height;

// Buffers for drawing - New frame buffer system
static char frame_buffer[2560*2560];     // Main frame buffer for ANSI codes
static float depth_buffer[2560*2560];    // Depth buffer for Z-testing
static int buffer_width, buffer_height;  // Buffer dimensions


// Function declarations
void output_buffer();
void geometry_draw();

#endif // RENDER_HPP
