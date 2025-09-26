#if !defined(RENDER_HPP)
#define RENDER_HPP

#include "../tinyrenderer-master/our_gl.h"
#include "../tinyrenderer-master/model.h"
#include "../console/console.hpp"
#include <string>

// ANSI Color Modes
enum class ColorMode {
    COLOR_4BIT,   // 16 colors (30-37, 90-97 for fg; 40-47, 100-107 for bg)
    COLOR_8BIT,   // 256 colors (38;5;<n> and 48;5;<n>)
    COLOR_24BIT   // Truecolor (38;2;R;G;B and 48;2;R;G;B)
};

class SimpleRenderer {
private:
    ConsoleManager& console;  // Changed from pointer to reference
    Model* model;
    static const int width = 0;
    static const int height = 0;
    
    // Console size tracking
    int savedConsoleWidth;
    int savedConsoleHeight;
    int currentConsoleWidth;
    int currentConsoleHeight;
    
    // Color mode setting
    ColorMode currentColorMode;
    
    // Color conversion functions
    std::string ConvertToANSI(int r, int g, int b, bool isBackground = false);
    int RGBTo4Bit(int r, int g, int b, bool isBright = false);
    int RGBTo8Bit(int r, int g, int b);

public:
    SimpleRenderer(ConsoleManager& consoleManager);  // Changed parameter to reference
    ~SimpleRenderer();
    bool LoadModel(const std::string& filename);
    void RenderFrame();
    void UpdateConsoleSize();
    void SetColorMode(ColorMode mode);
    ColorMode GetColorMode() const;
};

#endif // RENDER_HPP