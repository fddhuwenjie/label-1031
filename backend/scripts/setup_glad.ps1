# GLAD 自动生成脚本 (Windows PowerShell)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir
$GladDir = Join-Path $ProjectDir "third_party\glad"

Write-Host "[INFO] Setting up GLAD for OpenGL 3.3 Core Profile..."

# 检查 Python3
try {
    python --version | Out-Null
} catch {
    Write-Host "[ERROR] Python is required but not installed."
    exit 1
}

# 检查/安装 glad
$gladInstalled = python -c "import glad" 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "[INFO] Installing glad via pip..."
    pip install glad
}

# 创建目录
New-Item -ItemType Directory -Force -Path $GladDir | Out-Null

# 生成 GLAD
Write-Host "[INFO] Generating GLAD files..."
python -m glad --generator=c --out-path="$GladDir" --api="gl=3.3" --profile=core

# 验证
$gladC = Join-Path $GladDir "src\glad.c"
$gladH = Join-Path $GladDir "include\glad\glad.h"

if ((Test-Path $gladC) -and (Test-Path $gladH)) {
    Write-Host "[SUCCESS] GLAD generated successfully at $GladDir"
} else {
    Write-Host "[ERROR] GLAD generation failed"
    exit 1
}
