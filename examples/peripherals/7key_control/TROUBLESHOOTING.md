# 故障排除指南 / Troubleshooting Guide

## 问题 1：所有按键同时触发

### 症状
- 启动后所有按键立即触发
- 没有按下按键也会持续触发长按事件

### 可能原因
1. **硬件接线问题**
   - COM 端没有正确连接到 GND
   - 按键模块的 VCC 没有连接（如果需要）
   - 线路接触不良

2. **GPIO 配置问题**
   - GPIO 引脚处于浮空状态
   - 需要配置内部上拉电阻

3. **按键模块类型不匹配**
   - 有些按键模块是高电平有效，需要修改 `BUTTON_ACTIVE_LEVEL`

### 解决方案

#### 方案 1：检查硬件连接

确保按键模块正确连接：

```
按键模块 COM 端 ──> MCU GND
按键模块 UP 端  ──> MCU P27
按键模块 DOWN 端 ──> MCU P31
... (其他按键同理)
```

**重要**: COM 端必须连接到 GND！

#### 方案 2：修改按键有效电平

如果你的按键模块是高电平有效（按下时输出高电平），修改代码：

```c
// 在 example_7key.c 中修改这一行
#define BUTTON_ACTIVE_LEVEL  TUYA_GPIO_LEVEL_HIGH  // 改为高电平有效
```

#### 方案 3：使用轮询模式

代码已经修改为使用轮询模式（BUTTON_POLL_MODE），这样更稳定。如果还有问题，可以尝试切换回中断模式：

```c
// 在 register_button_hardware() 函数中
.mode  = BUTTON_IRQ_MODE,  // 使用中断模式
.pin_type.irq_edge = TUYA_GPIO_IRQ_FALL,  // 下降沿触发
```

#### 方案 4：测试单个按键

临时注释掉其他按键，只测试一个：

```c
static BUTTON_INFO_T button_list[] = {
    {BTN_NAME_UP,    GPIO_PIN_UP,    NULL},
    // {BTN_NAME_DOWN,  GPIO_PIN_DOWN,  NULL},  // 注释掉
    // {BTN_NAME_LEFT,  GPIO_PIN_LEFT,  NULL},  // 注释掉
    // ... 其他按键也注释掉
};
```

## 问题 2：按键无响应

### 症状
- 按下按键没有任何输出
- 串口显示初始化成功，但按键不工作

### 解决方案

1. **检查 GPIO 引脚号**
   - 确认引脚号与你的硬件匹配
   - 不同的板子引脚定义可能不同

2. **检查按键有效电平**
   - 尝试切换 `BUTTON_ACTIVE_LEVEL` 的值

3. **增加调试输出**
   - 在回调函数中添加更多日志

## 问题 3：按键抖动严重

### 症状
- 单击一次触发多次事件
- 按键响应不稳定

### 解决方案

增加消抖时间：

```c
TDL_BUTTON_CFG_T button_cfg = {
    .button_debounce_time = 100,  // 从 50ms 增加到 100ms
    // ... 其他配置
};
```

## 问题 4：长按时间不合适

### 解决方案

调整长按配置：

```c
TDL_BUTTON_CFG_T button_cfg = {
    .long_start_valid_time = 1000,  // 长按触发时间（毫秒）
    .long_keep_timer = 300,         // 长按重复间隔（毫秒）
    // ... 其他配置
};
```

## 硬件测试方法

### 使用万用表测试

1. **测试 COM 端**
   - 万用表设置为电阻档
   - 测量 COM 端到 GND 的电阻，应该接近 0Ω

2. **测试按键**
   - 万用表设置为通断档
   - 按下按键时，对应引脚应该与 COM 端导通

3. **测试 GPIO 电平**
   - 万用表设置为电压档
   - 未按下时，GPIO 应该是高电平（约 3.3V）
   - 按下时，GPIO 应该是低电平（约 0V）

## 调试技巧

### 1. 添加 GPIO 状态读取

在代码中添加 GPIO 状态读取来调试：

```c
// 在 user_main() 中添加
for (int i = 0; i < BUTTON_COUNT; i++) {
    TUYA_GPIO_LEVEL_E level;
    tkl_gpio_read(button