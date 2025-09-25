#if !defined(RENDER_HPP)
#define RENDER_HPP

#include "../tinyrenderer-master/our_gl.h"
#include "../tinyrenderer-master/model.h"
#include "../console/console.hpp"

class SimpleRenderer {
private:
    ConsoleManager& console;  // Changed from pointer to reference
    Model* model;
    static const int width = 960;
    static const int height = 240;
    
    // Console size tracking
    int savedConsoleWidth;
    int savedConsoleHeight;
    int currentConsoleWidth;
    int currentConsoleHeight;

public:
    SimpleRenderer(ConsoleManager& consoleManager);  // Changed parameter to reference
    ~SimpleRenderer();
    bool LoadModel(const std::string& filename);
    void RenderFrame();
    void UpdateConsoleSize();
};

#endif // RENDER_HPP