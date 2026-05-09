#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include "shader.h"
#include "camera.h"
#include "water_simulation.h"

class Renderer {
public:
    Renderer(int width, int height, const char* title);
    ~Renderer();

    bool init();
    void run();

    GLFWwindow* getWindow() const { return window; }

private:
    void processInput(float deltaTime);
    void render();
    void renderUI();
    void setupCallbacks();

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    GLFWwindow* window;
    int screenWidth;
    int screenHeight;

    std::unique_ptr<Shader> waterShader;
    std::unique_ptr<Shader> gridShader;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<WaterSimulation> waterSim;

    // 鼠标状态
    float lastX, lastY;
    bool firstMouse;
    bool mousePressed;

    // 时间
    float deltaTime;
    float lastFrame;

    // 渲染选项
    bool wireframeMode;
    bool showGrid;
};

#endif
