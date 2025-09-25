#if !defined(RENDER_HPP)
#define RENDER_HPP

#include "../tinyrenderer-master/our_gl.h"
#include "../tinyrenderer-master/model.h"
#include "../console/console.hpp"

class SimpleRenderer {
private:
    ConsoleManager* console;
    Model* model;
    static const int width = 960;
    static const int height = 240;

public:
    SimpleRenderer(ConsoleManager* consoleManager);
    ~SimpleRenderer();
    bool LoadModel(const std::string& filename);
    void RenderFrame();
};

#endif // RENDER_HPP