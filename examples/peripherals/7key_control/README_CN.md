# 7键控制示例

本示例演示如何在 TuyaOpen 平台上使用 7 个按键进行控制输入。

## 硬件连接

| 按键 | 功能 | GPIO 引脚 | 说明 |
|------|------|-----------|------|
| COM  | 公共端 | GND | 连接到 GND |
| UP   | 上键 | P27 | 连接到单片机 IO 引脚 |
| DOWN | 下键 | P31 | 连接到单片机 IO 引脚 |
| LEFT | 左键 | P36 | 连接到单片机 IO 引脚 |
| RIGHT| 右键 | P30 | 连接到单片机 IO 引脚 |
| MID  | 中间键 | P37 | 连接到单片机 IO 引脚 |
| SET  | 设置键 | P32 | 连接到单片机 IO 引脚 |
| RET  | 返回键 | P39 | 连接到单片机 IO 引脚 |

## 接线图

```
按键模块          单片机
┌─────────┐      ┌─────────┐
│   GND   │──────│  GND    │
│   UP    │──────│  P27    │
│   DOWN  │──────│  P31    │
│   LEFT  │──────│  P36    │
│   RIGHT │──────│  P30    │
│   MID   │──────│  P37    │
│   SET   │──────│  P32    │
│   RET   │──────│  P39    │
└─────────┘      └─────────┘
```

## 功能特性

- 检测所有 7 个按键的单击事件
- 检测所有 7 个按键的长按事件
- 可配置的消抖时间和长按持续时间
- 低电平有效按键逻辑（按下 = 低电平）

## 按键事件

- **单击**: 快速按下并释放按键
- **长按开始**: 按住按键超过 2 秒
- **长按保持**: 长按期间每 500ms 触发一次

## 编译和烧录

**重要**: 请确保已激活虚拟环境！如果遇到构建问题，请查看 [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md)

```bash
# 1. 激活虚拟环境（如果还没激活）
source ../../../export.sh

# 2. 配置项目（可选）
tos.py config

# 3. 编译
tos.py build

# 4. 烧录到设备
tos.py flash

# 5. 监控串口输出
tos.py monitor
```

## 预期输出

当你按下按键时，应该看到类似以下的输出：

```
========================================
7-Key Control Example
========================================
Project name:        7key_control
...
========================================
All buttons initialized successfully!
Button mapping:
  UP    -> P27
  DOWN  -> P31
  LEFT  -> P36
  RIGHT -> P30
  MID   -> P37
  SET   -> P32
  RET   -> P39
Press any button to test...
[UP] Single Click
[DOWN] Single Click
[LEFT] Long Press Start
[MID] Single Click
...
```

## 代码说明

### 按键配置

```c
TDL_BUTTON_CFG_T button_cfg = {
    .long_start_valid_time = 2000,      // 长按触发时间：2秒
    .long_keep_timer = 500,             // 长按保持重复间隔：500ms
    .button_debounce_time = 50,         // 消抖时间：50ms
    .button_repeat_valid_count = 2,     // 需要2次一致读取
    .button_repeat_valid_time = 500     // 重复检测时间：500ms
};
```

### GPIO 配置

所有按键 GPIO 配置为：
- 输入模式
- 内部上拉
- 低电平有效（按下时为低电平）

### 回调函数

`button_event_cb()` 函数处理所有按键事件：
- `TDL_BUTTON_PRESS_DOWN`: 单击事件
- `TDL_BUTTON_LONG_PRESS_START`: 长按开始
- `TDL_BUTTON_LONG_PRESS_HOLD`: 长按保持

## 应用场景

此示例可用于：
- 菜单导航系统
- 游戏控制器
- 设备设置界面
- 电子书阅读器翻页控制
- 任何需要多按键输入的应用

## 自定义修改

### 修改 GPIO 引脚

在 `example_7key.c` 中修改引脚定义：

```c
#define GPIO_PIN_UP     27  // 修改为你的引脚号
#define GPIO_PIN_DOWN   31
// ...
```

### 修改长按时间

修改 `button_cfg` 中的 `long_start_valid_time` 值（单位：毫秒）

### 添加更多事件

可以注册更多按键事件类型：
- `TDL_BUTTON_DOUBLE_CLICK`: 双击
- `TDL_BUTTON_TRIPLE_CLICK`: 三击
- 等等

## 故障排除

1. **按键无响应**
   - 检查 GPIO 引脚号是否正确
   - 确认按键模块正确连接到 GND 和对应 GPIO
   - 检查串口输出是否有初始化错误

2. **按键误触发**
   - 增加 `button_debounce_time` 值
   - 检查硬件连接是否稳定

3. **长按不工作**
   - 调整 `long_start_valid_time` 值
   - 确认按键按住时间足够长

## 许可证

Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
