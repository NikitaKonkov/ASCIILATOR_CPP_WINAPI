#include "render.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

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

#define MAXV(a,b,c) ( ((a)>(b)) ? ( ((a)>(c)) ? (a) : (c) ) : ( ((b)>(c)) ? (b) : (c) ) )
#define MINV(a,b,c) ( ((a)<(b)) ? ( ((a)<(c)) ? (a) : (c) ) : ( ((b)<(c)) ? (b) : (c) ) )
int SimpleRenderer::RGBTo4Bit(int r, int g, int b, bool isBright) {
    // Normalize once using integer math where possible
    int maxVal = MAXV(r, g, b);
    int minVal = MINV(r, g, b);
    int sum    = r + g + b;

    double brightness = sum / (255.0 * 3.0); // [0,1]
    bool useBright = isBright || brightness > 0.4;

    double saturation = (maxVal > 0) ? (double)(maxVal - minVal) / maxVal : 0.0;

    // Handle grayscale (low saturation)
    if (saturation < 0.1) {
        if (brightness < 0.2) return useBright ? 90 : 30; // Black
        if (brightness > 0.8) return useBright ? 97 : 37; // White
        return useBright ? 90 : 30;                       // Mid gray → fallback to black
    }

    // Color classification
    int color;
    if (r >= g && r >= b) {
        // Red dominant
        if (g > b && g > r * 0.6)      color = 3; // Yellow
        else if (b > r * 0.6)          color = 5; // Magenta
        else                           color = 1; // Red
    } else if (g >= r && g >= b) {
        // Green dominant
        if (r > b && r > g * 0.6)      color = 3; // Yellow
        else if (b > g * 0.6)          color = 6; // Cyan
        else                           color = 2; // Green
    } else {
        // Blue dominant
        if (r > g && r > b * 0.6)      color = 5; // Magenta
        else if (g > b * 0.6)          color = 6; // Cyan
        else                           color = 4; // Blue
    }

    return (useBright ? 90 : 30) + color;
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

    // Update console size first
    UpdateConsoleSize();

    int renderWidth  = currentConsoleWidth - 1;
    int renderHeight = currentConsoleHeight - 3;

    static float angle = 0.0f;
    angle += 0.05f; // Rotate model slowly

    // Camera + lighting
    vec3 light{1, 1, 1};
    vec3 eye{2 * cos(angle), 1, 2 * sin(angle)};
    vec3 center{0, 0, 0};
    vec3 up{0, 1, 0};

    // Build matrices
    lookat(eye, center, up);
    init_perspective(norm(eye - center));
    init_viewport(renderWidth / 8, renderHeight / 8, renderWidth * 3 / 4, renderHeight * 3 / 4);
    init_zbuffer(renderWidth, renderHeight);

    // Create framebuffer
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

    // Sampling step sizes (avoid repeated division)
    int stepX = MAX(1, renderWidth / (currentConsoleWidth - 1));
    int stepY = MAX(1, renderHeight / (currentConsoleHeight - 3));

    // Clear console + move cursor
    console.MoveCursor(1, 1);

    std::string output;
    output.reserve(renderWidth * renderHeight); // pre-allocate

    // Header info
    output += "\033[1;36m3D Model Render (";
    output += std::to_string(stepX);
    output += "x";
    output += std::to_string(stepY);
    output += ") Internal:";
    output += std::to_string(renderWidth);
    output += "x";
    output += std::to_string(renderHeight);
    output += " Console:";
    output += std::to_string(currentConsoleWidth);
    output += "x";
    output += std::to_string(currentConsoleHeight);
    output += " Mode:";
    output += (currentColorMode == ColorMode::COLOR_4BIT ? "4bit" :
               currentColorMode == ColorMode::COLOR_8BIT ? "8bit" : "24bit");
    output += " Frame:";
    output += std::to_string(static_cast<int>(angle * 10));
    output += "\033[0m\n";

    // Framebuffer sampling + RLE
    for (int y = 0; y < renderHeight; y += stepY) {
        int x = 0;
        while (x < renderWidth) {
            TGAColor pixel = framebuffer.get(x, y);
            int r = pixel[2];
            int g = pixel[1];
            int b = pixel[0];

            // Precompute ANSI once
            std::string colorCode = ConvertToANSI(r, g, b, false);

            // Run-length encoding
            int repeatCount = 1;
            while (x + repeatCount * stepX < renderWidth) {
                TGAColor next = framebuffer.get(x + repeatCount * stepX, y);
                if (next[0] == pixel[0] && next[1] == pixel[1] && next[2] == pixel[2]) {
                    repeatCount++;
                } else {
                    break;
                }
            }

            // Output chunk
            output += colorCode;
            output.append(repeatCount, '#');

            x += repeatCount * stepX;
        }
        output += "\033[0m\n"; // reset only once per line
    }

    console.Print(output.c_str());
}
