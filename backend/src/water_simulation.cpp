#include "water_simulation.h"
#include <cmath>
#include <algorithm>
#include <iostream>

// 日志宏
#define LOG_INFO(msg) std::cout << "[INFO] WaterSim: " << msg << std::endl
#define LOG_WARN(msg) std::cerr << "[WARN] WaterSim: " << msg << std::endl
#define LOG_ERROR(msg) std::cerr << "[ERROR] WaterSim: " << msg << std::endl

WaterSimulation::WaterSimulation(int gridSize, float size)
    : gridSize(std::clamp(gridSize, WaterParams::MIN_GRID_SIZE, WaterParams::MAX_GRID_SIZE)), 
      size(std::clamp(size, WaterParams::MIN_SIZE, WaterParams::MAX_SIZE)), 
      currentMode(WaterMode::PHYSICS_SIMULATION),
      waveSpeed(1.0f), damping(0.98f), waveAmplitude(0.15f), waveFrequency(2.0f),
      totalTime(0.0f),
      rainModeActive(false), rainFrequency(8.0f), rainAccumulator(0.0f),
      rng(std::random_device{}()),
      posDist(-size / 2.0f + 0.5f, size / 2.0f - 0.5f),
      strengthDist(0.5f, 2.0f) {
    
    // 参数校验日志
    if (gridSize != this->gridSize) {
        LOG_WARN("Grid size clamped from " << gridSize << " to " << this->gridSize);
    }
    if (size != this->size) {
        LOG_WARN("Size clamped from " << size << " to " << this->size);
    }
    
    LOG_INFO("Initializing water simulation:");
    LOG_INFO("  Grid size: " << this->gridSize << "x" << this->gridSize);
    LOG_INFO("  Physical size: " << this->size << " units");
    LOG_INFO("  Vertex count: " << (this->gridSize * this->gridSize));
    
    gridSpacing = this->size / static_cast<float>(this->gridSize - 1);
    
    int vertexCount = this->gridSize * this->gridSize;
    positions.resize(vertexCount);
    normals.resize(vertexCount, glm::vec3(0.0f, 1.0f, 0.0f));
    texCoords.resize(vertexCount);
    heights.resize(vertexCount, 0.0f);
    velocities.resize(vertexCount, 0.0f);
    
    initMesh();
    LOG_INFO("Water simulation initialized successfully");
}

WaterSimulation::~WaterSimulation() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void WaterSimulation::initMesh() {
    float halfSize = size / 2.0f;
    
    // 生成顶点
    for (int z = 0; z < gridSize; ++z) {
        for (int x = 0; x < gridSize; ++x) {
            int idx = z * gridSize + x;
            float xPos = -halfSize + x * gridSpacing;
            float zPos = -halfSize + z * gridSpacing;
            
            positions[idx] = glm::vec3(xPos, 0.0f, zPos);
            texCoords[idx] = glm::vec2(
                static_cast<float>(x) / (gridSize - 1),
                static_cast<float>(z) / (gridSize - 1)
            );
        }
    }
    
    // 生成索引（三角形）
    for (int z = 0; z < gridSize - 1; ++z) {
        for (int x = 0; x < gridSize - 1; ++x) {
            int topLeft = z * gridSize + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * gridSize + x;
            int bottomRight = bottomLeft + 1;
            
            // 第一个三角形
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            // 第二个三角形
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    
    // 创建OpenGL缓冲区
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    // 顶点数据：位置(3) + 法线(3) + 纹理坐标(2) + 速度(1) = 9 floats per vertex
    std::vector<float> vertexData;
    vertexData.reserve(positions.size() * 9);
    
    for (size_t i = 0; i < positions.size(); ++i) {
        vertexData.push_back(positions[i].x);
        vertexData.push_back(positions[i].y);
        vertexData.push_back(positions[i].z);
        vertexData.push_back(normals[i].x);
        vertexData.push_back(normals[i].y);
        vertexData.push_back(normals[i].z);
        vertexData.push_back(texCoords[i].x);
        vertexData.push_back(texCoords[i].y);
        vertexData.push_back(velocities[i]);
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), 
                 vertexData.data(), GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);
    
    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 法线属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), 
                         (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // 纹理坐标属性
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
                         (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // 速度属性
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
                         (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    
    glBindVertexArray(0);
}

void WaterSimulation::update(float deltaTime) {
    totalTime += deltaTime;

    updateRainDrops(deltaTime);
    
    if (currentMode == WaterMode::PHYSICS_SIMULATION) {
        updatePhysicsSimulation(deltaTime);
    } else {
        updatePresetAnimation(deltaTime);
    }
    
    updateNormals();
    updateBuffers();
}

void WaterSimulation::updatePhysicsSimulation(float deltaTime) {
    // 波动方程模拟
    float c2 = waveSpeed * waveSpeed;
    float dt2 = deltaTime * deltaTime;
    float dx2 = gridSpacing * gridSpacing;
    
    std::vector<float> newHeights = heights;
    
    for (int z = 1; z < gridSize - 1; ++z) {
        for (int x = 1; x < gridSize - 1; ++x) {
            int idx = z * gridSize + x;
            int left = idx - 1;
            int right = idx + 1;
            int up = idx - gridSize;
            int down = idx + gridSize;
            
            // 拉普拉斯算子
            float laplacian = (heights[left] + heights[right] + 
                              heights[up] + heights[down] - 4.0f * heights[idx]) / dx2;
            
            // 更新速度和高度
            velocities[idx] += c2 * laplacian * deltaTime;
            velocities[idx] *= damping;
            
            // 限制速度防止爆炸
            velocities[idx] = std::clamp(velocities[idx], -2.0f, 2.0f);
            
            newHeights[idx] = heights[idx] + velocities[idx] * deltaTime;
            
            // 限制高度范围
            newHeights[idx] = std::clamp(newHeights[idx], -0.5f, 0.5f);
        }
    }
    
    heights = newHeights;
    
    // 更新位置
    for (int i = 0; i < gridSize * gridSize; ++i) {
        positions[i].y = heights[i];
    }
}

void WaterSimulation::updatePresetAnimation(float deltaTime) {
    // Gerstner波动画
    float halfSize = size / 2.0f;
    
    // 多个波的参数
    struct Wave {
        glm::vec2 direction;
        float amplitude;
        float frequency;
        float speed;
        float steepness;
    };
    
    std::vector<Wave> waves = {
        {{1.0f, 0.0f}, waveAmplitude, waveFrequency, 1.5f, 0.5f},
        {{0.0f, 1.0f}, waveAmplitude * 0.5f, waveFrequency * 1.5f, 1.2f, 0.3f},
        {{0.7f, 0.7f}, waveAmplitude * 0.3f, waveFrequency * 2.0f, 1.8f, 0.4f},
        {{-0.5f, 0.8f}, waveAmplitude * 0.4f, waveFrequency * 0.8f, 1.0f, 0.35f}
    };
    
    for (int z = 0; z < gridSize; ++z) {
        for (int x = 0; x < gridSize; ++x) {
            int idx = z * gridSize + x;
            
            float baseX = -halfSize + x * gridSpacing;
            float baseZ = -halfSize + z * gridSpacing;
            
            float totalHeight = 0.0f;
            float totalX = baseX;
            float totalZ = baseZ;
            
            for (const auto& wave : waves) {
                float k = 2.0f * 3.14159f * wave.frequency;
                float phase = wave.speed * totalTime;
                float dot = wave.direction.x * baseX + wave.direction.y * baseZ;
                float theta = k * dot + phase;
                
                // Gerstner波公式
                totalX += wave.steepness * wave.amplitude * wave.direction.x * cos(theta);
                totalZ += wave.steepness * wave.amplitude * wave.direction.y * cos(theta);
                totalHeight += wave.amplitude * sin(theta);
            }
            
            positions[idx] = glm::vec3(totalX, totalHeight, totalZ);
            heights[idx] = totalHeight;
        }
    }
}

void WaterSimulation::updateNormals() {
    // 重置法线
    for (auto& n : normals) {
        n = glm::vec3(0.0f);
    }
    
    // 计算面法线并累加到顶点
    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];
        
        glm::vec3 v0 = positions[i0];
        glm::vec3 v1 = positions[i1];
        glm::vec3 v2 = positions[i2];
        
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));
        
        normals[i0] += faceNormal;
        normals[i1] += faceNormal;
        normals[i2] += faceNormal;
    }
    
    // 归一化
    for (auto& n : normals) {
        n = glm::normalize(n);
    }
}

void WaterSimulation::updateBuffers() {
    std::vector<float> vertexData;
    vertexData.reserve(positions.size() * 9);
    
    for (size_t i = 0; i < positions.size(); ++i) {
        vertexData.push_back(positions[i].x);
        vertexData.push_back(positions[i].y);
        vertexData.push_back(positions[i].z);
        vertexData.push_back(normals[i].x);
        vertexData.push_back(normals[i].y);
        vertexData.push_back(normals[i].z);
        vertexData.push_back(texCoords[i].x);
        vertexData.push_back(texCoords[i].y);
        vertexData.push_back(velocities[i]);
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexData.size() * sizeof(float), vertexData.data());
}

void WaterSimulation::render() {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void WaterSimulation::reset() {
    std::fill(heights.begin(), heights.end(), 0.0f);
    std::fill(velocities.begin(), velocities.end(), 0.0f);
    totalTime = 0.0f;
    
    float halfSize = size / 2.0f;
    for (int z = 0; z < gridSize; ++z) {
        for (int x = 0; x < gridSize; ++x) {
            int idx = z * gridSize + x;
            positions[idx] = glm::vec3(
                -halfSize + x * gridSpacing,
                0.0f,
                -halfSize + z * gridSpacing
            );
        }
    }
    
    updateNormals();
    updateBuffers();
}

void WaterSimulation::addDisturbance(float x, float z, float strength) {
    if (currentMode != WaterMode::PHYSICS_SIMULATION) return;
    
    float halfSize = size / 2.0f;
    
    // 找到最近的网格点
    int gridX = static_cast<int>((x + halfSize) / gridSpacing);
    int gridZ = static_cast<int>((z + halfSize) / gridSpacing);
    
    // 柔和的圆形波纹
    int radius = 4;
    for (int dz = -radius; dz <= radius; ++dz) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int gx = gridX + dx;
            int gz = gridZ + dz;
            
            if (gx >= 1 && gx < gridSize - 1 && gz >= 1 && gz < gridSize - 1) {
                int idx = gz * gridSize + gx;
                float dist = std::sqrt(static_cast<float>(dx * dx + dz * dz));
                if (dist <= radius) {
                    float falloff = 1.0f - dist / radius;
                    heights[idx] -= strength * falloff * 0.08f;  // 减小强度
                }
            }
        }
    }
}

void WaterSimulation::setMode(WaterMode mode) {
    if (currentMode != mode) {
        currentMode = mode;
        LOG_INFO("Mode changed to: " << (mode == WaterMode::PHYSICS_SIMULATION ? "Physics Simulation" : "Preset Animation"));
        if (mode == WaterMode::PRESET_ANIMATION) {
            rainModeActive = false;  // 切换到预设动画时自动关闭雨滴模式
        }
        reset();
    }
}

void WaterSimulation::updateRainDrops(float deltaTime) {
    if (!rainModeActive || currentMode != WaterMode::PHYSICS_SIMULATION) return;

    rainAccumulator += deltaTime;
    float dropInterval = 1.0f / rainFrequency;

    while (rainAccumulator >= dropInterval) {
        rainAccumulator -= dropInterval;
        float x = posDist(rng);
        float z = posDist(rng);
        float strength = strengthDist(rng);
        addDisturbance(x, z, strength);
    }
}

void WaterSimulation::toggleRainMode() {
    if (currentMode != WaterMode::PHYSICS_SIMULATION) {
        rainModeActive = false;
        return;
    }
    rainModeActive = !rainModeActive;
    LOG_INFO("Rain mode: " << (rainModeActive ? "ON" : "OFF"));
}
