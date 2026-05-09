#include "renderer.h"
#include <iostream>

int main() {
    std::cout << "Starting Water Simulation..." << std::endl;
    
    Renderer renderer(1280, 720, "Water Simulation - OpenGL");
    
    if (!renderer.init()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return -1;
    }
    
    renderer.run();
    
    std::cout << "Water Simulation ended." << std::endl;
    return 0;
}
