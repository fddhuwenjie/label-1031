#ifndef WATER_SIMULATION_H
#define WATER_SIMULATION_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <random>

// 水面模拟模式
enum class WaterMode {
    PHYSICS_SIMULATION,  // 物理模拟（波动方程）
    PRESET_ANIMATION     // 预设动画（Gerstner波）
};

// 参数约束常量
namespace WaterParams {
    constexpr int MIN_GRID_SIZE = 10;
    constexpr int MAX_GRID_SIZE = 500;
    constexpr float MIN_SIZE = 1.0f;
    constexpr float MAX_SIZE = 100.0f;
    constexpr float MIN_WAVE_SPEED = 0.1f;
    constexpr float MAX_WAVE_SPEED = 10.0f;
    constexpr float MIN_DAMPING = 0.9f;
    constexpr float MAX_DAMPING = 1.0f;
    constexpr float MIN_AMPLITUDE = 0.01f;
    constexpr float MAX_AMPLITUDE = 2.0f;
    constexpr float MIN_FREQUENCY = 0.1f;
    constexpr float MAX_FREQUENCY = 10.0f;
    constexpr float MIN_RAIN_INTENSITY = 0.5f;
    constexpr float MAX_RAIN_INTENSITY = 2.0f;
    constexpr float DEFAULT_RAIN_FREQUENCY = 8.0f;
}

class WaterSimulation {
public:
    WaterSimulation(int gridSize = 100, float size = 10.0f);
    ~WaterSimulation();

    void update(float deltaTime);
    void render();
    void reset();

    // 交互
    void addDisturbance(float x, float z, float strength);
    void setMode(WaterMode mode);
    WaterMode getMode() const { return currentMode; }

    // 参数调整（带范围校验）
    void setWaveSpeed(float speed) { 
        waveSpeed = std::clamp(speed, WaterParams::MIN_WAVE_SPEED, WaterParams::MAX_WAVE_SPEED); 
    }
    void setDamping(float d) { 
        damping = std::clamp(d, WaterParams::MIN_DAMPING, WaterParams::MAX_DAMPING); 
    }
    void setWaveAmplitude(float amp) { 
        waveAmplitude = std::clamp(amp, WaterParams::MIN_AMPLITUDE, WaterParams::MAX_AMPLITUDE); 
    }
    void setWaveFrequency(float freq) { 
        waveFrequency = std::clamp(freq, WaterParams::MIN_FREQUENCY, WaterParams::MAX_FREQUENCY); 
    }

    // Getters
    float getWaveSpeed() const { return waveSpeed; }
    float getDamping() const { return damping; }
    float getWaveAmplitude() const { return waveAmplitude; }
    float getWaveFrequency() const { return waveFrequency; }
    int getGridSize() const { return gridSize; }
    float getSize() const { return size; }

    unsigned int getVAO() const { return VAO; }
    int getIndexCount() const { return static_cast<int>(indices.size()); }

    // 雨滴模式控制
    void toggleRainMode();                    // 切换雨滴模式开关
    void setRainMode(bool enabled);           // 设置雨滴模式状态
    bool isRainModeEnabled() const { return rainModeEnabled; }  // 获取雨滴模式状态
    void setRainFrequency(float freq);        // 设置雨滴生成频率（每秒滴数）
    float getRainFrequency() const { return rainFrequency; }    // 获取雨滴生成频率

private:
    void initMesh();
    void updatePhysicsSimulation(float deltaTime);
    void updatePresetAnimation(float deltaTime);
    void updateNormals();
    void updateBuffers();
    void updateRainDrops(float deltaTime);  // 更新雨滴生成逻辑

    int gridSize;
    float gridSpacing;
    float size;

    // 顶点数据
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<unsigned int> indices;

    // 物理模拟数据
    std::vector<float> heights;
    std::vector<float> velocities;
    float waveSpeed;
    float damping;

    // 预设动画参数
    float waveAmplitude;
    float waveFrequency;
    float totalTime;

    // OpenGL对象
    unsigned int VAO, VBO, EBO;

    WaterMode currentMode;

    // 雨滴模式相关成员
    bool rainModeEnabled;                     // 雨滴模式开关状态
    float rainFrequency;                      // 雨滴生成频率（每秒滴数）
    float rainAccumulator;                    // 雨滴生成时间累加器
    std::mt19937 randomEngine;                // 随机数生成器
    std::uniform_real_distribution<float> posDist;  // 位置随机分布
    std::uniform_real_distribution<float> strengthDist;  // 强度随机分布
};

#endif
