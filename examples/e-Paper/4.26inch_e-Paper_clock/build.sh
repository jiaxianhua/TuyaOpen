#!/bin/bash
# 构建 4.26inch_e-Paper_clock 示例的脚本

cd "$(dirname "$0")"
PROJECT_DIR=$(pwd)
TUYAOPEN_ROOT=$(cd ../../.. && pwd)

echo "项目目录: $PROJECT_DIR"
echo "TuyaOpen 根目录: $TUYAOPEN_ROOT"

cd "$TUYAOPEN_ROOT"

# 激活虚拟环境
if [ -f ".venv/bin/activate" ]; then
    source .venv/bin/activate
fi

# 进入项目目录并构建
cd "$PROJECT_DIR"
python3 "$TUYAOPEN_ROOT/tos.py" build
