#include "render.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

// External tinyrenderer globals
extern mat<4,4> ModelView, Perspective;
extern std::vector<double> zbuffer;

struct SimpleShader : IShader {
    const Model &model;
    vec4 l;              // light direction in eye coordinates
    vec2 varying_uv[3];  // triangle uv coordinates
    vec4 varying_nrm[3]; // normal per vertex
    vec4 tri[3];         // triangle in view coordinates

    SimpleShader(const vec3 light, const Model &m) : model(m) {
        l = normalized((ModelView * vec4{light.x, light.y, light.z, 0.}));
    }

    virtual vec4 vertex(const int face, const int vert) {
        varying_uv[vert] = model.uv(face, vert);
        varying_nrm[vert] = ModelView.invert_transpose() * model.normal(face, vert);
        vec4 gl_Position = ModelView * model.vert(face, vert);
        tri[vert] = gl_Position;
        return Perspective * gl_Position;
    }

    virtual std::pair<bool,TGAColor> fragment(const vec3 bar) const {
        vec2 uv = varying_uv[0] * bar[0] + varying_uv[1] * bar[1] + varying_uv[2] * bar[2];
        vec4 n = normalized(varying_nrm[0] * bar[0] + varying_nrm[1] * bar[1] + varying_nrm[2] * bar[2]);
        
        double ambient = 0.3;
        double diffuse = (n * l > 0.0) ? n * l : 0.0;
        
        TGAColor gl_FragColor = sample2D(model.diffuse(), uv);
        for (int channel = 0; channel < 3; channel++) {
            double intensity = static_cast<double>(gl_FragColor[channel]) * (ambient + diffuse);
            gl_FragColor[channel] = static_cast<uint8_t>((intensity > 255.0) ? 255 : intensity);
        }
        gl_FragColor[3] = 255; // Full alpha
        return {false, gl_FragColor};
    }
};

SimpleRenderer::SimpleRenderer(ConsoleManager& consoleManager) : console(consoleManager), model(nullptr) {
    // Initialize console size tracking
    savedConsoleWidth = 0;
    savedConsoleHeight = 0;
    currentConsoleWidth = 0;
    currentConsoleHeight = 0;
}

SimpleRenderer::~SimpleRenderer() {
    if (model) {
        delete model;
    }
}

bool SimpleRenderer::LoadModel(const std::string& filename) {
    try {
        if (model) {
            delete model;
        }
        model = new Model(filename);
        return true;
    } catch (...) {
        return false;
    }
}

void SimpleRenderer::UpdateConsoleSize() {
    // Get current console size
    console.GetConsoleSize(&currentConsoleWidth, &currentConsoleHeight);
    
    // Check if console size has changed
    if (currentConsoleWidth != savedConsoleWidth || currentConsoleHeight != savedConsoleHeight) {
        // Console size changed - clear the screen
        console.ClearScreen();
        
        // Update saved dimensions
        savedConsoleWidth = currentConsoleWidth;
        savedConsoleHeight = currentConsoleHeight;
    }
}

void SimpleRenderer::RenderFrame() {
    if (!model) {
        return;
    }

    // Update and check console size first
    UpdateConsoleSize();

    // Calculate dynamic render resolution based on console size
    int targetWidth = currentConsoleWidth - 2;   // Leave some margin
    int targetHeight = currentConsoleHeight - 3; // Leave margin for header and prompt
    
    // Ensure minimum values
    if (targetWidth < 10) targetWidth = 10;
    if (targetHeight < 5) targetHeight = 5;
    
    // Scale up the render resolution for better quality (2x scale factor)
    int renderWidth = targetWidth * 2;
    int renderHeight = targetHeight * 2;
    
    // Cap maximum resolution for performance
    if (renderWidth > 1920) renderWidth = 1920;
    if (renderHeight > 1080) renderHeight = 1080;

    static float angle = 0.0f;
    angle += 0.05f; // Rotate model slowly

    // Setup camera and lighting
    vec3 light{1, 1, 1};
    vec3 eye{2*cos(angle), 1, 2*sin(angle)}; // Rotating camera
    vec3 center{0, 0, 0};
    vec3 up{0, 1, 0};

    // Build matrices using dynamic resolution
    lookat(eye, center, up);
    init_perspective(norm(eye - center));
    init_viewport(renderWidth/8, renderHeight/8, renderWidth*3/4, renderHeight*3/4);
    init_zbuffer(renderWidth, renderHeight);

    // Create framebuffer with dynamic resolution
    TGAImage framebuffer(renderWidth, renderHeight, TGAImage::RGBA, {50, 50, 100, 255});

    // Render model
    SimpleShader shader(light, *model);
    for (int f = 0; f < model->nfaces(); f++) {
        Triangle clip = {
            shader.vertex(f, 0),
            shader.vertex(f, 1),
            shader.vertex(f, 2)
        };
        rasterize(clip, shader, framebuffer);
    }
    
    // Calculate sampling rates to fit the console (should be close to 1:1 now)
    int sampleX = renderWidth / targetWidth;
    int sampleY = renderHeight / targetHeight;
    
    // Ensure minimum sampling of 1
    if (sampleX < 1) sampleX = 1;
    if (sampleY < 1) sampleY = 1;
    
    int effectiveWidth = renderWidth / sampleX;
    int effectiveHeight = renderHeight / sampleY;
    
    // Clear and move to top
    console.MoveCursor(1, 1);
    
    std::stringstream output;
    output << "\033[1;36m3D Model Render (" << effectiveWidth << "x" << effectiveHeight 
           << ") Internal:" << renderWidth << "x" << renderHeight
           << " Console:" << currentConsoleWidth << "x" << currentConsoleHeight
           << " Frame:" << static_cast<int>(angle * 10) << "\033[0m\n";
    
    // Sample the framebuffer with dynamic sampling rates
    for (int y = 0; y < renderHeight; y += sampleY) {
        for (int x = 0; x < renderWidth; x += sampleX) {
            TGAColor pixel = framebuffer.get(x, y);
            
            // Extract RGB values (TGAColor stores as BGRA)
            int r = static_cast<int>(pixel[2]);  // Red
            int g = static_cast<int>(pixel[1]);  // Green  
            int b = static_cast<int>(pixel[0]);  // Blue
            
            // Calculate brightness for choosing character
            int brightness = (r + g + b) / 3;
            char displayChar;
            
            if (brightness > 200) displayChar = '*';
            else if (brightness > 150) displayChar = '+';
            else if (brightness > 100) displayChar = '#';
            else if (brightness > 50) displayChar = '@';
            else displayChar = ' ';
            
            // Use ANSI 24-bit truecolor for foreground color with character
            if (brightness > 30) {  // Only show color for non-black pixels
                output << "\033[38;2;" << r << ";" << g << ";" << b << "m" << displayChar << "\033[0m";
            } else {
                output << " ";  // One space for black/dark pixels
            }
        }
        output << "\n";
    }
    
    console.Print(output.str().c_str());
}

