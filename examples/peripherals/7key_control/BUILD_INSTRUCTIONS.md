# 构建说明 / Build Instructions

## 问题 / Issue

如果你遇到 `python: command not found` 错误，这是因为系统中只有 `python3` 命令。

## 解决方案 / Solution

### 方法 1：在虚拟环境中构建（推荐）

```bash
# 1. 回到项目根目录
cd ~/Code/TuyaOpen_jiaxianhua

# 2. 激活虚拟环境（如果还没激活）
source export.sh

# 3. 进入项目目录
cd examples/peripherals/7key_control

# 4. 构建项目
tos.py build
```

### 方法 2：创建 python 符号链接

如果你有 sudo 权限，可以创建系统级符号链接：

```bash
sudo ln -s /usr/bin/python3 /usr/bin/python
```

然后重新运行构建：

```bash
cd examples/peripherals/7key_control
tos.py build
```

### 方法 3：使用构建脚本

```bash
cd examples/peripherals/7key_control
./build.sh
```

## 验证构建成功

构建成功后，你应该看到：

```
[INFO]: Build success.
```

输出文件位于：`.build/` 目录

## 烧录到设备

```bash
tos.py flash
```

## 监控串口输出

```bash
tos.py monitor
```

## 常见问题

### Q: 提示 "tos.py: command not found"
A: 确保你已经激活了虚拟环境（运行 `source export.sh`）

### Q: 提示 "python: not found"  
A: 使用上述方法 1 或方法 2 解决

### Q: 编译错误
A: 检查 GPIO 引脚定义是否与你的硬件匹配
