# Multi-stage build for Water Simulation
# Supports both ARM64 and AMD64 architectures

# Stage 1: Build stage
FROM --platform=$BUILDPLATFORM ubuntu:22.04 AS builder

ARG TARGETPLATFORM
ARG BUILDPLATFORM
ARG DEBIAN_FRONTEND=noninteractive

# 使用阿里云镜像源加速
RUN sed -i 's/ports.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list && \
    sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list

# Install build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    wget \
    python3 \
    python3-pip \
    libglfw3-dev \
    libglm-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libxxf86vm-dev \
    xorg-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Generate GLAD files
RUN pip3 install glad && \
    mkdir -p third_party/glad && \
    python3 -m glad --generator=c --out-path=third_party/glad --api="gl=3.3" --profile=core

# Copy source files
COPY CMakeLists.txt .
COPY include/ include/
COPY src/ src/
COPY shaders/ shaders/

# Build the application
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# Stage 2: Runtime stage
FROM ubuntu:22.04 AS runtime

ARG DEBIAN_FRONTEND=noninteractive

# 使用阿里云镜像源加速
RUN sed -i 's/ports.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list && \
    sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list

# Install runtime dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    libglfw3 \
    libgl1-mesa-glx \
    libgl1-mesa-dri \
    mesa-utils \
    xvfb \
    x11vnc \
    fluxbox \
    novnc \
    websockify \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy built application and shaders
COPY --from=builder /app/build/WaterSimulation /app/
COPY --from=builder /app/shaders /app/shaders/

# Create startup script
RUN echo '#!/bin/bash\n\
set -e\n\
\n\
# 清理旧的X锁文件\n\
rm -f /tmp/.X99-lock /tmp/.X11-unix/X99\n\
\n\
# Start Xvfb (virtual framebuffer)\n\
Xvfb :99 -screen 0 1280x720x24 &\n\
sleep 1\n\
\n\
export DISPLAY=:99\n\
\n\
# Start window manager\n\
fluxbox &\n\
sleep 1\n\
\n\
# Start VNC server\n\
x11vnc -display :99 -forever -shared -rfbport 5900 &\n\
\n\
# Start noVNC web server\n\
websockify --web=/usr/share/novnc/ 6080 localhost:5900 &\n\
\n\
# 创建index.html重定向\n\
echo "<html><head><meta http-equiv=refresh content=0;url=/vnc.html></head></html>" > /usr/share/novnc/index.html\n\
\n\
echo "==========================================="\n\
echo "Water Simulation is running!"\n\
echo "Access via browser: http://localhost:8081"\n\
echo "==========================================="\n\
\n\
# Run the application\n\
cd /app && ./WaterSimulation\n\
' > /app/start.sh && chmod +x /app/start.sh

EXPOSE 6080 5900

CMD ["/app/start.sh"]
