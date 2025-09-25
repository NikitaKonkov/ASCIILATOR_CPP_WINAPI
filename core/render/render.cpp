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
    
    // Set default color mode to 24-bit
    currentColorMode = ColorMode::COLOR_24BIT;
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

// Color mode setter and getter
void SimpleRenderer::SetColorMode(ColorMode mode) {
    currentColorMode = mode;
}

ColorMode SimpleRenderer::GetColorMode() const {
    return currentColorMode;
}

// Convert RGB to 4-bit ANSI color (16 colors)
int SimpleRenderer::RGBTo4Bit(int r, int g, int b, bool isBright) {
    // Normalize RGB to 0-1 range
    double rd = r / 255.0;
    double gd = g / 255.0;
    double bd = b / 255.0;
    
    // Calculate brightness
    double brightness = (rd + gd + bd) / 3.0;
    
    // Determine if we should use bright colors
    bool useBright = isBright || brightness > 0.5;
    
    // Find closest basic color
    int color = 0; // Black
    
    if (rd > 0.5 && gd < 0.3 && bd < 0.3) color = 1; // Red
    else if (rd < 0.3 && gd > 0.5 && bd < 0.3) color = 2; // Green
    else if (rd > 0.5 && gd > 0.5 && bd < 0.3) color = 3; // Yellow
    else if (rd < 0.3 && gd < 0.3 && bd > 0.5) color = 4; // Blue
    else if (rd > 0.5 && gd < 0.3 && bd > 0.5) color = 5; // Magenta
    else if (rd < 0.3 && gd > 0.5 && bd > 0.5) color = 6; // Cyan
    else if (rd > 0.5 && gd > 0.5 && bd > 0.5) color = 7; // White
    
    // Return appropriate color code
    return useBright ? (90 + color) : (30 + color);
}

// Convert RGB to 8-bit ANSI color (256 colors)
int SimpleRenderer::RGBTo8Bit(int r, int g, int b) {
    // Convert RGB to 6x6x6 color cube (216 colors) + 16 basic colors + 24 grayscale
    if (r == g && g == b) {
        // Grayscale
        if (r < 8) return 16;
        if (r > 248) return 231;
        return 232 + (r - 8) / 10;
    }
    
    // Color cube: 16 + 36*r + 6*g + b
    int r6 = r * 5 / 255;
    int g6 = g * 5 / 255;
    int b6 = b * 5 / 255;
    
    return 16 + 36 * r6 + 6 * g6 + b6;
}

// Main color converter function
std::string SimpleRenderer::ConvertToANSI(int r, int g, int b, bool isBackground) {
    std::stringstream ansi;
    
    switch (currentColorMode) {
        case ColorMode::COLOR_4BIT: {
            int colorCode = RGBTo4Bit(r, g, b);
            if (isBackground) {
                // Convert foreground code to background (30-37 -> 40-47, 90-97 -> 100-107)
                if (colorCode >= 90) colorCode = colorCode - 90 + 100;
                else colorCode = colorCode - 30 + 40;
            }
            ansi << "\033[" << colorCode << "m";
            break;
        }
        
        case ColorMode::COLOR_8BIT: {
            int colorCode = RGBTo8Bit(r, g, b);
            if (isBackground) {
                ansi << "\033[48;5;" << colorCode << "m";
            } else {
                ansi << "\033[38;5;" << colorCode << "m";
            }
            break;
        }
        
        case ColorMode::COLOR_24BIT:
        default: {
            if (isBackground) {
                ansi << "\033[48;2;" << r << ";" << g << ";" << b << "m";
            } else {
                ansi << "\033[38;2;" << r << ";" << g << ";" << b << "m";
            }
            break;
        }
    }
    
    return ansi.str();
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
           << " Mode:" << (currentColorMode == ColorMode::COLOR_4BIT ? "4bit" : 
                          currentColorMode == ColorMode::COLOR_8BIT ? "8bit" : "24bit")
           << " Frame:" << static_cast<int>(angle * 10) << "\033[0m\n";
    
    // Sample the framebuffer with dynamic sampling rates using run-length encoding optimization
    for (int y = 0; y < renderHeight; y += sampleY) {
        int x = 0;
        while (x < renderWidth) {
            TGAColor pixel = framebuffer.get(x, y);
            
            // Extract RGB values (TGAColor stores as BGRA)
            int r = static_cast<int>(pixel[2]);  // Red
            int g = static_cast<int>(pixel[1]);  // Green  
            int b = static_cast<int>(pixel[0]);  // Blue
            
            // Calculate brightness for choosing character
            int brightness = (r + g + b) / 3;
            char displayChar;
            


            displayChar = '@';
            
            
            // Count consecutive pixels with same color (run-length encoding)
            int repeatCount = 1;
            std::string currentColorCode = ConvertToANSI(r, g, b, false);
            
            // Only count repeats within the same line and if brightness > 30
        while (x + repeatCount * sampleX < renderWidth) {
            TGAColor nextPixel = framebuffer.get(x + repeatCount * sampleX, y);
            int nextR = static_cast<int>(nextPixel[2]);
            int nextG = static_cast<int>(nextPixel[1]);
            int nextB = static_cast<int>(nextPixel[0]);
            int nextBrightness = (nextR + nextG + nextB) / 3;
            
            // Check if next pixel has same color and brightness > 30
            if (nextBrightness > 30 && ConvertToANSI(nextR, nextG, nextB, false) == currentColorCode) {
                repeatCount++;
            } else {
                break;
            }
        }
        
        // Output the color once and repeat the character
        output << currentColorCode;
        for (int rep = 0; rep < repeatCount; rep++) {
            output << displayChar;
        }
        output << "\033[0m";
        
        x += repeatCount * sampleX;
        }
        output << "\n";
    }
    
    console.Print(output.str().c_str());
}

