# 快速开始指南

## 5 分钟上手 7 键控制

### 1. 硬件连接（2 分钟）

将按键模块连接到开发板：

```
按键模块          开发板
  COM    ────>    GND
  UP     ────>    P27
  DOWN   ────>    P31
  LEFT   ────>    P36
  RIGHT  ────>    P30
  MID    ────>    P37
  SET    ────>    P32
  RET    ────>    P39
```

**重要**: COM 端必须连接到 GND！

### 2. 编译烧录（2 分钟）

```bash
# 激活虚拟环境
source export.sh

# 进入项目目录
cd examples/peripherals/7key_control

# 编译
tos.py build

# 烧录
tos.py flash

# 监控串口
tos.py monitor
```

### 3. 测试按键（1 分钟）

按下任意按键，串口应该输出：

```
[UP] Single Click
[DOWN] Single Click
[LEFT] Single Click
...
```

按住按键 2 秒以上，会触发长按：

```
[MID] Long Press Start
[MID] Long Press Hold
[MID] Long Press Hold
...
```

## 完成！

现在你可以：
- 修改 GPIO 引脚定义适配你的硬件
- 调整消抖时间和长按时间
- 添加自己的按键处理逻辑

## 遇到问题？

查看 [TROUBLESHOOTING.md](TROUBLESHOOTING.md) 获取帮助。
