# Water Simulation - OpenGL

## How to Run

### 使用 Docker（推荐）

```bash
# 构建并启动
docker-compose up --build -d

# 查看日志
docker-compose logs -f

# 停止服务
docker-compose down
```

启动后，打开浏览器访问：**http://localhost:8081/vnc.html**

### 本地编译运行

```bash
cd backend

# 安装依赖（Ubuntu/Debian）
sudo apt-get install libglfw3-dev libglm-dev libgl1-mesa-dev

# 自动生成 GLAD（推荐）
./scripts/setup_glad.sh

# 或手动生成
# pip3 install glad
# python3 -m glad --generator=c --out-path=third_party/glad --api="gl=3.3" --profile=core

# 编译
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 运行
./WaterSimulation
```

**Windows 用户**：
```powershell
cd backend
.\scripts\setup_glad.ps1
mkdir build; cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
.\Release\WaterSimulation.exe
```

## Services

| 服务 | 端口 | 描述 |
|------|------|------|
| noVNC | 8081 | Web 界面（浏览器访问） |
| VNC | 5901 | VNC 客户端访问 |

## 测试账号

本项目为纯图形渲染应用，无需登录账号。

## 题目内容

使用 C++ 和 OpenGL 库（glfw, glad, glm）实现水效果，包含：
- 物理模拟（波动方程）
- 预设动画（Gerstner 波）

（编译版）

---

## 项目介绍

基于 OpenGL 的实时水面模拟程序，实现了物理模拟和预设动画两种水效果模式。

### 控制方式

| 按键 | 功能 |
|------|------|
| W/A/S/D | 移动相机 |
| 鼠标右键+移动 | 旋转视角 |
| 滚轮 | 缩放 |
| Space/Q | 上升/下降 |
| 1 | 切换到物理模拟模式 |
| 2 | 切换到预设动画模式 |
| 左键点击 | 添加水面扰动（物理模式） |
| R | 重置水面 |
| F | 切换线框模式 |
| ESC | 退出 |

---

## 项目架构

```
backend/
├── include/                 # 头文件
│   ├── camera.h            # 相机控制类
│   ├── renderer.h          # 渲染器主类
│   ├── shader.h            # 着色器管理
│   └── water_simulation.h  # 水面模拟核心
├── src/                    # 源文件
│   ├── main.cpp           # 程序入口
│   ├── camera.cpp         # 相机实现（FPS风格）
│   ├── renderer.cpp       # 渲染循环、输入处理
│   ├── shader.cpp         # 着色器编译、链接
│   └── water_simulation.cpp # 水面物理/动画
├── shaders/               # GLSL 着色器
│   ├── water.vert/frag   # 水面渲染
│   └── grid.vert/frag    # 参考网格
├── CMakeLists.txt        # 构建配置
└── Dockerfile            # 容器化配置
```

### 核心类职责

- **Renderer**: 管理 OpenGL 上下文、渲染循环、输入回调
- **WaterSimulation**: 水面网格生成、物理模拟、动画更新
- **Camera**: 视角控制、投影矩阵计算
- **Shader**: 着色器加载、编译、uniform 设置

---

## 核心算法

### 1. 物理模拟模式 - 波动方程

基于二维波动方程的数值求解：

$$\frac{\partial^2 h}{\partial t^2} = c^2 \nabla^2 h$$

其中 $h$ 是水面高度，$c$ 是波速。

**离散化实现**（有限差分法）：

```cpp
// 拉普拉斯算子（中心差分）
float laplacian = (h[left] + h[right] + h[up] + h[down] - 4*h[center]) / dx²

// 速度更新
velocity[i] += c² * laplacian * dt
velocity[i] *= damping  // 阻尼衰减

// 高度更新
height[i] += velocity[i] * dt
```

**参数说明**：
- `waveSpeed` (0.1-10.0): 波传播速度
- `damping` (0.9-1.0): 能量衰减系数，越接近1波持续越久

### 2. 预设动画模式 - Gerstner 波

Gerstner 波是一种精确的水波解，能产生真实的海浪效果：

$$\mathbf{P} = \begin{pmatrix} x - \sum Q_i A_i D_{ix} \cos(\omega_i \mathbf{D}_i \cdot \mathbf{P}_0 + \phi_i t) \\ \sum A_i \sin(\omega_i \mathbf{D}_i \cdot \mathbf{P}_0 + \phi_i t) \\ z - \sum Q_i A_i D_{iz} \cos(\omega_i \mathbf{D}_i \cdot \mathbf{P}_0 + \phi_i t) \end{pmatrix}$$

**实现特点**：
- 多波叠加（4个不同方向、频率的波）
- 水平位移模拟（波峰尖锐、波谷平缓）
- `steepness` 参数控制波形陡峭程度

```cpp
struct Wave {
    vec2 direction;    // 传播方向
    float amplitude;   // 振幅
    float frequency;   // 频率 (ω = 2π/λ)
    float speed;       // 相速度
    float steepness;   // 陡峭度 Q ∈ [0,1]
};
```

### 3. 渲染技术

**Blinn-Phong 光照**：
```glsl
vec3 halfDir = normalize(lightDir + viewDir);
float spec = pow(max(dot(normal, halfDir), 0.0), shininess);
```

**菲涅尔效果**：
```glsl
float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 3.0);
vec3 color = mix(waterColorDeep, waterColorShallow, fresnel);
```

**动态法线计算**：
- 每帧根据相邻顶点位置重新计算面法线
- 顶点法线通过相邻面法线平均得到

---

## 参数配置

### 物理模拟参数

| 参数 | 范围 | 默认值 | 说明 |
|------|------|--------|------|
| gridSize | 10-500 | 100 | 网格分辨率 |
| size | 1.0-100.0 | 10.0 | 物理尺寸（单位） |
| waveSpeed | 0.1-10.0 | 1.0 | 波传播速度 |
| damping | 0.9-1.0 | 0.98 | 阻尼系数 |

### 预设动画参数

| 参数 | 范围 | 默认值 | 说明 |
|------|------|--------|------|
| waveAmplitude | 0.01-2.0 | 0.15 | 波浪振幅 |
| waveFrequency | 0.1-10.0 | 2.0 | 波浪频率 |

---

## 技术栈

- **语言**: C++17
- **图形库**: OpenGL 3.3 Core Profile
- **窗口管理**: GLFW3
- **数学库**: GLM
- **OpenGL 加载**: GLAD
- **容器化**: Docker (支持 ARM64/AMD64)

## 跨平台支持

| 平台 | 状态 | 备注 |
|------|------|------|
| Linux (x64/ARM64) | ✅ | 主要开发平台 |
| macOS | ✅ | 需要 OpenGL 3.3+ |
| Windows | ✅ | 需要 MSVC 或 MinGW |
| Docker | ✅ | 通过 VNC 访问 |

## License

MIT
