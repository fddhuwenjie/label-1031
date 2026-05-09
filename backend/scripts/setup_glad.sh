#!/bin/bash
# GLAD 自动生成脚本
# 支持 Linux/macOS

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
GLAD_DIR="$PROJECT_DIR/third_party/glad"

echo "[INFO] Setting up GLAD for OpenGL 3.3 Core Profile..."

# 检查 Python3
if ! command -v python3 &> /dev/null; then
    echo "[ERROR] Python3 is required but not installed."
    exit 1
fi

# 检查/安装 glad
if ! python3 -c "import glad" &> /dev/null; then
    echo "[INFO] Installing glad via pip..."
    pip3 install glad --user
fi

# 创建目录
mkdir -p "$GLAD_DIR"

# 生成 GLAD
echo "[INFO] Generating GLAD files..."
python3 -m glad --generator=c --out-path="$GLAD_DIR" --api="gl=3.3" --profile=core

# 验证
if [ -f "$GLAD_DIR/src/glad.c" ] && [ -f "$GLAD_DIR/include/glad/glad.h" ]; then
    echo "[SUCCESS] GLAD generated successfully at $GLAD_DIR"
else
    echo "[ERROR] GLAD generation failed"
    exit 1
fi
