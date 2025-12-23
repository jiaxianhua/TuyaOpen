# 按钮配置说明 / Button Setup Guide

## 中文说明

### 按钮 GPIO 配置

项目默认使用 **GPIO 17** 作为按钮引脚。如果你的板子按钮连接到其他 GPIO，需要修改配置。

### 修改步骤

1. 打开文件 `board_button_config.h`

2. 修改 `BOARD_BUTTON_PIN` 定义：

```c
// 将 TUYA_GPIO_NUM_17 改为你的按钮 GPIO
#define BOARD_BUTTON_PIN TUYA_GPIO_NUM_17  // 改成你的 GPIO 编号
```

### 常见 T5AI 按钮 GPIO

不同的 T5AI 板子按钮 GPIO 可能不同，常见的有：

- **GPIO 17** - 默认配置
- **GPIO 18** - 第二个按钮
- **GPIO 19** - 第三个按钮  
- **GPIO 26** - 第四个按钮

### 如何找到你的按钮 GPIO？

1. 查看板子原理图
2. 查看板级配置文件：`boards/T5AI/<你的板子>/`
3. 使用万用表测试按钮连接的引脚

### 按钮电平配置

默认配置为**低电平有效**（按下时为 LOW）：

```c
#define BOARD_BUTTON_ACTIVE_LV TUYA_GPIO_LEVEL_LOW
```

如果你的按钮是高电平有效（按下时为 HIGH），修改为：

```c
#define BOARD_BUTTON_ACTIVE_LV TUYA_GPIO_LEVEL_HIGH
```

### 按钮功能

- **短按**：下一页
- **长按（3秒）**：上一页

---

## English Guide

### Button GPIO Configuration

The project uses **GPIO 17** as the default button pin. If your board's button is connected to a different GPIO, you need to modify the configuration.

### Modification Steps

1. Open file `board_button_config.h`

2. Modify the `BOARD_BUTTON_PIN` definition:

```c
// Change TUYA_GPIO_NUM_17 to your button GPIO
#define BOARD_BUTTON_PIN TUYA_GPIO_NUM_17  // Change to your GPIO number
```

### Common T5AI Button GPIOs

Different T5AI boards may use different button GPIOs:

- **GPIO 17** - Default configuration
- **GPIO 18** - Second button
- **GPIO 19** - Third button
- **GPIO 26** - Fourth button

### How to Find Your Button GPIO?

1. Check your board schematic
2. Check board configuration files: `boards/T5AI/<your_board>/`
3. Use a multimeter to test which pin the button is connected to

### Button Active Level Configuration

Default configuration is **active low** (LOW when pressed):

```c
#define BOARD_BUTTON_ACTIVE_LV TUYA_GPIO_LEVEL_LOW
```

If your button is active high (HIGH when pressed), change to:

```c
#define BOARD_BUTTON_ACTIVE_LV TUYA_GPIO_LEVEL_HIGH
```

### Button Functions

- **Short press**: Next page
- **Long press (3 seconds)**: Previous page

---

## Troubleshooting / 故障排除

### Button not working / 按钮不工作

1. **Check GPIO number** / 检查 GPIO 编号
   - Verify the GPIO pin in `board_button_config.h`
   - 验证 `board_button_config.h` 中的 GPIO 引脚

2. **Check active level** / 检查有效电平
   - Test if button is active LOW or HIGH
   - 测试按钮是低电平还是高电平有效

3. **Check serial output** / 检查串口输出
   ```
   [01-01 00:00:08 ty N][EPD_4in26_network_novel.c:316] Initializing button...
   [01-01 00:00:08 ty D][EPD_4in26_network_novel.c:328] Button hardware registered on GPIO 17
   [01-01 00:00:08 ty N][EPD_4in26_network_novel.c:345] Button initialized successfully
   ```

4. **Test button press** / 测试按钮按下
   - Short press should show: `Button: novel_button, Event: 0`
   - Long press should show: `Button: novel_button, Event: 2`

### Common GPIO Pins by Board / 各板子常用 GPIO

| Board / 板子 | Button GPIO | Notes / 备注 |
|-------------|-------------|--------------|
| TUYA_T5AI_POCKET | GPIO 17-19, 26 | 4 buttons / 4个按钮 |
| TUYA_T5AI_EVB | Check schematic / 查看原理图 | - |
| Custom board / 自定义板 | User defined / 用户定义 | - |

---

## Example Configuration / 配置示例

### Example 1: Using GPIO 18 / 使用 GPIO 18

```c
#define BOARD_BUTTON_PIN TUYA_GPIO_NUM_18
#define BOARD_BUTTON_ACTIVE_LV TUYA_GPIO_LEVEL_LOW
```

### Example 2: Using GPIO 26 with active HIGH / 使用 GPIO 26 高电平有效

```c
#define BOARD_BUTTON_PIN TUYA_GPIO_NUM_26
#define BOARD_BUTTON_ACTIVE_LV TUYA_GPIO_LEVEL_HIGH
```

---

## After Configuration / 配置完成后

1. **Rebuild** / 重新编译:
   ```bash
   tos.py build
   ```

2. **Flash** / 烧录:
   ```bash
   tos.py flash
   ```

3. **Test** / 测试:
   - Short press button → Next page / 短按按钮 → 下一页
   - Long press button → Previous page / 长按按钮 → 上一页
