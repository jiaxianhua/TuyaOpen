# 4.26 inch e-Paper Clock Example with Network Time Sync

## 概述

这个示例演示如何使用 4.26 英寸 e-Paper 显示屏显示实时时钟，并通过网络自动同步时间。时钟每秒更新一次，显示当前的日期、时间和星期。

## 功能特性

- ✓ 启动时自动连接 WiFi
- ✓ 从 HTTP 服务器同步网络时间
- ✓ 显示当前日期（年-月-日格式）
- ✓ 显示当前时间（时:分:秒格式）- 大字体居中显示
- ✓ 显示星期几（英文）
- ✓ 每秒自动更新一次
- ✓ 使用部分刷新技术，减少闪烁
- ✓ 显示时间同步状态
- ✓ 程序持续运行，不会退出

## 硬件要求

- Tuya T5AI 开发板（带 WiFi 功能）
- 4.26 英寸 e-Paper 显示屏模块

## 配置

### WiFi 设置

在使用前，需要修改 `examples/EPD_4in26_clock.c` 中的 WiFi 配置：

```c
#define DEFAULT_WIFI_SSID "your_wifi_ssid"      // 修改为你的 WiFi 名称
#define DEFAULT_WIFI_PSWD "your_wifi_password"  // 修改为你的 WiFi 密码
```

### 时间服务器

默认使用 `worldtimeapi.org` 的 API 获取时间。如需更改，可修改：

```c
#define TIME_SERVER_URL  "worldtimeapi.org"
#define TIME_SERVER_PATH "/api/timezone/Asia/Shanghai"
```

## 构建和运行

### 1. 激活开发环境

```bash
source export.sh
```

### 2. 配置项目

```bash
tos.py config
```

选择：
- Platform: T5AI
- Board: TUYA_T5AI_BOARD (或您使用的具体板型)
- Example: examples/e-Paper/4.26inch_e-Paper_clock

### 3. 编译

```bash
tos.py build
```

### 4. 烧录到设备

```bash
tos.py flash
```

### 5. 查看输出

```bash
tos.py monitor
```

## 代码说明

### 主要文件

- `examples/main.c` - 程序入口点
- `examples/EPD_4in26_clock.c` - 时钟显示逻辑
- `examples/EPD_Test.h` - 头文件
- `lib/` - e-Paper 驱动库（从原示例复制）

### 工作原理

1. 初始化 e-Paper 显示屏
2. 全屏刷新显示标题和分隔线
3. 进入无限循环：
   - 获取系统当前时间
   - 格式化日期、时间和星期字符串
   - 使用部分刷新更新显示区域
   - 等待 1 秒
   - 重复

### 时间源

程序使用系统时间（`time()` 和 `localtime()`）。确保您的设备时间已正确设置。

## 自定义

您可以修改以下内容来自定义显示：

- 字体大小：修改 `Paint_DrawString_EN()` 中的 `&Font24` 参数
- 显示位置：修改 `EPD_4in26_Display_Part()` 的坐标参数
- 更新频率：修改 `DEV_Delay_ms(1000)` 的延迟时间
- 日期时间格式：修改 `strftime()` 的格式字符串

## 注意事项

- e-Paper 显示屏刷新速度较慢，频繁刷新可能影响显示寿命
- 部分刷新比全屏刷新更快，但长时间使用后可能需要全屏刷新来清除残影
- 程序设计为持续运行，不会自动退出

## 许可证

基于原 Waveshare e-Paper 示例代码修改。
