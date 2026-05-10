#include "renderer.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

// 静态成员用于回调
static Renderer* g_renderer = nullptr;

Renderer::Renderer(int width, int height, const char* title)
    : screenWidth(width), screenHeight(height), window(nullptr),
      lastX(width / 2.0f), lastY(height / 2.0f), firstMouse(true),
      mousePressed(false), deltaTime(0.0f), lastFrame(0.0f),
      wireframeMode(false), showGrid(true) {
    g_renderer = this;
}

Renderer::~Renderer() {
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
    g_renderer = nullptr;
}

bool Renderer::init() {
    // 初始化GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 创建窗口
    window = glfwCreateWindow(screenWidth, screenHeight, "Water Simulation", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    
    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // 设置回调
    setupCallbacks();

    // OpenGL设置
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 创建着色器
    waterShader = std::make_unique<Shader>("shaders/water.vert", "shaders/water.frag");
    gridShader = std::make_unique<Shader>("shaders/grid.vert", "shaders/grid.frag");

    // 创建相机
    camera = std::make_unique<Camera>(glm::vec3(0.0f, 5.0f, 10.0f));

    // 创建水面模拟
    waterSim = std::make_unique<WaterSimulation>(100, 10.0f);

    std::cout << "=== Water Simulation Controls ===" << std::endl;
    std::cout << "WASD: Move camera" << std::endl;
    std::cout << "Mouse: Look around (hold right button)" << std::endl;
    std::cout << "Scroll: Zoom" << std::endl;
    std::cout << "Space/Q: Move up/down" << std::endl;
    std::cout << "1: Physics simulation mode" << std::endl;
    std::cout << "2: Preset animation mode" << std::endl;
    std::cout << "3: Toggle rain drops mode (physics mode only)" << std::endl;
    std::cout << "Left Click: Add disturbance (physics mode)" << std::endl;
    std::cout << "R: Reset water" << std::endl;
    std::cout << "F: Toggle wireframe" << std::endl;
    std::cout << "ESC: Exit" << std::endl;
    std::cout << "=================================" << std::endl;

    return true;
}

void Renderer::setupCallbacks() {
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
}

void Renderer::run() {
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(deltaTime);
        
        waterSim->update(deltaTime);
        
        render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void Renderer::processInput(float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera->ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera->ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera->ProcessKeyboard(DOWN, deltaTime);
}

void Renderer::render() {
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 设置变换矩阵
    glm::mat4 projection = glm::perspective(
        glm::radians(camera->Zoom),
        static_cast<float>(screenWidth) / static_cast<float>(screenHeight),
        0.1f, 100.0f
    );
    glm::mat4 view = camera->GetViewMatrix();
    glm::mat4 model = glm::mat4(1.0f);

    // 渲染水面
    if (wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    waterShader->use();
    waterShader->setMat4("projection", projection);
    waterShader->setMat4("view", view);
    waterShader->setMat4("model", model);
    waterShader->setVec3("viewPos", camera->Position);
    waterShader->setVec3("lightPos", glm::vec3(5.0f, 10.0f, 5.0f));
    waterShader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 0.95f));
    waterShader->setFloat("time", static_cast<float>(glfwGetTime()));
    
    // 水的颜色
    waterShader->setVec3("waterColorDeep", glm::vec3(0.0f, 0.1f, 0.3f));
    waterShader->setVec3("waterColorShallow", glm::vec3(0.0f, 0.4f, 0.6f));

    waterSim->render();

    if (wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // 渲染参考网格
    if (showGrid) {
        gridShader->use();
        gridShader->setMat4("projection", projection);
        gridShader->setMat4("view", view);
        
        // 简单的地面网格
        glm::mat4 gridModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
        gridShader->setMat4("model", gridModel);
    }
}

void Renderer::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    if (g_renderer) {
        g_renderer->screenWidth = width;
        g_renderer->screenHeight = height;
    }
}

void Renderer::mouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
    if (!g_renderer) return;
    
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (g_renderer->firstMouse) {
        g_renderer->lastX = xpos;
        g_renderer->lastY = ypos;
        g_renderer->firstMouse = false;
    }

    float xoffset = xpos - g_renderer->lastX;
    float yoffset = g_renderer->lastY - ypos;

    g_renderer->lastX = xpos;
    g_renderer->lastY = ypos;

    // 只有按住右键时才旋转相机
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        g_renderer->camera->ProcessMouseMovement(xoffset, yoffset);
    }
}

void Renderer::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (g_renderer) {
        g_renderer->camera->ProcessMouseScroll(static_cast<float>(yoffset));
    }
}

void Renderer::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (!g_renderer || action != GLFW_PRESS) return;

    switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, true);
            break;
        case GLFW_KEY_1:
            g_renderer->waterSim->setMode(WaterMode::PHYSICS_SIMULATION);
            std::cout << "Mode: Physics Simulation" << std::endl;
            break;
        case GLFW_KEY_2:
            g_renderer->waterSim->setMode(WaterMode::PRESET_ANIMATION);
            std::cout << "Mode: Preset Animation (Gerstner Waves)" << std::endl;
            break;
        case GLFW_KEY_3:
            g_renderer->waterSim->toggleRainMode();
            std::cout << "Rain mode: " << (g_renderer->waterSim->isRainModeActive() ? "ON" : "OFF") << std::endl;
            break;
        case GLFW_KEY_R:
            g_renderer->waterSim->reset();
            std::cout << "Water reset" << std::endl;
            break;
        case GLFW_KEY_F:
            g_renderer->wireframeMode = !g_renderer->wireframeMode;
            std::cout << "Wireframe: " << (g_renderer->wireframeMode ? "ON" : "OFF") << std::endl;
            break;
        case GLFW_KEY_G:
            g_renderer->showGrid = !g_renderer->showGrid;
            break;
    }
}

void Renderer::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (!g_renderer) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // 获取鼠标位置
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        
        // 将屏幕坐标转换为归一化设备坐标 (NDC)
        float x = (2.0f * xpos) / g_renderer->screenWidth - 1.0f;
        float y = 1.0f - (2.0f * ypos) / g_renderer->screenHeight;
        
        // 创建射线
        glm::mat4 projection = glm::perspective(
            glm::radians(g_renderer->camera->Zoom),
            static_cast<float>(g_renderer->screenWidth) / static_cast<float>(g_renderer->screenHeight),
            0.1f, 100.0f
        );
        glm::mat4 view = g_renderer->camera->GetViewMatrix();
        
        glm::mat4 invVP = glm::inverse(projection * view);
        glm::vec4 rayClip(x, y, -1.0f, 1.0f);
        glm::vec4 rayEye = glm::inverse(projection) * rayClip;
        rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
        glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
        
        // 射线与水面平面 (y=0) 相交
        glm::vec3 rayOrigin = g_renderer->camera->Position;
        if (rayWorld.y != 0.0f) {
            float t = -rayOrigin.y / rayWorld.y;
            if (t > 0.0f) {
                glm::vec3 hitPoint = rayOrigin + rayWorld * t;
                g_renderer->waterSim->addDisturbance(hitPoint.x, hitPoint.z, 2.0f);
            }
        }
    }
}
