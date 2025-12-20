# 4.26 英寸电子墨水屏网络时钟示例

## 概述

这个示例演示如何使用 4.26 英寸电子墨水屏显示实时时钟，并通过网络自动同步时间。时钟每秒更新一次，时间显示采用大字体并居中显示，提供更好的可读性。

## 功能特性

- ✓ **网络时间同步**：启动时自动从 HTTP 服务器获取准确时间
- ✓ **WiFi 自动连接**：启动时自动连接配置的 WiFi 网络
- ✓ **大字体显示**：时间使用 Font24 大字体，居中显示
- ✓ **分层信息**：日期、时间、星期分层显示，层次清晰
- ✓ **同步状态**：显示时间是否已同步的状态指示
- ✓ **实时更新**：每秒自动更新一次
- ✓ **部分刷新**：使用部分刷新技术，减少闪烁
- ✓ **持续运行**：程序持续运行，不会退出
- ✓ **优雅降级**：网络不可用时使用本地时间

## 硬件要求

- Tuya T5AI 开发板（带 WiFi 功能）
- 4.26 英寸电子墨水屏显示模块

## 配置说明

### 1. WiFi 配置

在使用前，**必须**修改 `examples/EPD_4in26_clock.c` 中的 WiFi 配置：

```c
#define DEFAULT_WIFI_SSID "your_wifi_ssid"      // 改为你的 WiFi 名称
#define DEFAULT_WIFI_PSWD "your_wifi_password"  // 改为你的 WiFi 密码
```

### 2. 时间服务器配置（可选）

默认使用 WorldTimeAPI 获取中国时区时间。如需更改，可修改：

```c
#define TIME_SERVER_URL  "worldtimeapi.org"
#define TIME_SERVER_PATH "/api/timezone/Asia/Shanghai"
```

支持的时区示例：
- 北京时间：`/api/timezone/Asia/Shanghai`
- 东京时间：`/api/timezone/Asia/Tokyo`
- 纽约时间：`/api/timezone/America/New_York`
- 伦敦时间：`/api/timezone/Europe/London`


## 显示布局

```
┌─────────────────────────────────┐
│ Network Clock                   │  ← 标题
│ Time Synced                     │  ← 同步状态
├─────────────────────────────────┤
│                                 │
│         2024-12-20              │  ← 日期（居中）
│                                 │
│          12:34:56               │  ← 时间（大字体，居中）
│                                 │
│          Thursday               │  ← 星期（居中）
│                                 │
│ *Synced                         │  ← 同步指示器
└─────────────────────────────────┘
```

### 显示特点

- **居中对齐**：所有信息都居中显示，视觉效果更好
- **大字体时间**：时间使用 Font24，清晰易读
- **分层显示**：日期、时间、星期分三层显示
- **状态指示**：顶部显示同步状态，底部显示同步标记

## 工作流程

### 1. 启动阶段（约 5-30 秒）
```
初始化日志 → 初始化网络服务 → 连接 WiFi → 同步时间 → 初始化显示屏
```

### 2. 时间同步
- 连接到配置的 WiFi 网络
- 发送 HTTP 请求到时间服务器
- 解析响应头中的 Date 字段
- 计算并保存时间偏移
- 显示"Time Synced"状态

### 3. 显示循环
- 获取当前时间（已同步则应用偏移）
- 格式化日期、时间、星期字符串
- 计算居中位置
- 绘制到屏幕缓冲区
- 部分刷新显示
- 等待 1 秒后重复

## 技术细节

### 网络时间同步

程序使用 HTTP 协议从时间服务器获取时间：

1. **发送请求**：GET 请求到 worldtimeapi.org
2. **解析响应**：从 HTTP 响应头提取 Date 字段
3. **时间格式**：`Thu, 20 Dec 2024 12:34:56 GMT`
4. **计算偏移**：`server_time - local_time = offset`
5. **应用偏移**：每次获取时间时加上偏移量

### 显示优化

- **字体选择**：
  - 标题：Font20
  - 日期：Font20
  - 时间：Font24（最大）
  - 星期：Font16
  - 状态：Font12

- **居中算法**：
  ```c
  int x = (screen_width - text_width) / 2;
  ```

- **刷新策略**：
  - 首次：全屏刷新（显示标题）
  - 后续：部分刷新（仅更新时间区域）

### 内存管理

- 初始缓冲区：全屏大小
- 时间显示缓冲区：200 像素高度
- 动态分配和释放

## 故障排除

### WiFi 连接失败
**症状**：显示"Using Local Time"
**原因**：
- WiFi 凭据错误
- WiFi 信号弱
- 路由器问题

**解决方案**：
1. 检查 WiFi SSID 和密码是否正确
2. 确保设备在 WiFi 信号范围内
3. 检查路由器是否正常工作
4. 查看串口输出的错误信息

### 时间同步失败
**症状**：显示"Using Local Time"但 WiFi 已连接
**原因**：
- 时间服务器不可达
- 网络防火墙阻止
- DNS 解析失败

**解决方案**：
1. 检查网络是否可以访问外网
2. 尝试更换时间服务器
3. 检查防火墙设置
4. 查看串口输出的详细错误

### 显示异常
**症状**：屏幕显示不正常或有残影
**原因**：
- 长时间使用部分刷新
- 电源不稳定
- 硬件连接问题

**解决方案**：
1. 定期进行全屏刷新
2. 检查电源供电
3. 检查硬件连接
4. 重启设备

### 时间不准确
**症状**：显示的时间与实际时间不符
**原因**：
- 时区设置错误
- 时间服务器返回错误
- 本地时间未设置

**解决方案**：
1. 检查时区配置（TIME_SERVER_PATH）
2. 更换时间服务器
3. 查看串口输出的时间信息

## 扩展建议

可以在此基础上添加以下功能：

### 1. 传感器数据显示
```c
// 添加温度显示
char temp_str[32];
snprintf(temp_str, sizeof(temp_str), "Temp: %.1f°C", temperature);
Paint_DrawString_EN(x, y, temp_str, &Font16, BLACK, WHITE);
```

### 2. 定期重新同步
```c
// 每小时重新同步一次
static int sync_counter = 0;
if (++sync_counter >= 3600) {  // 3600 秒 = 1 小时
    sync_time_from_http();
    sync_counter = 0;
}
```

### 3. 多时区显示
```c
// 显示多个时区的时间
time_t utc_time = get_synced_time();
struct tm *beijing_time = localtime(&utc_time);
struct tm *ny_time = gmtime(&utc_time);  // 需要调整时区
```

### 4. 天气信息
```c
// 集成天气 API
char weather_str[64];
snprintf(weather_str, sizeof(weather_str), "Weather: %s, %d°C", 
         weather_desc, temperature);
```

### 5. 农历日期
```c
// 添加农历转换函数
char lunar_str[32];
get_lunar_date(timeinfo, lunar_str, sizeof(lunar_str));
Paint_DrawString_CN(x, y, lunar_str, &Font12CN, BLACK, WHITE);
```

## 性能优化

### 减少刷新闪烁
- 使用部分刷新而非全屏刷新
- 只更新变化的区域
- 合理设置刷新间隔

### 降低功耗
- 使用深度睡眠模式（需要修改代码）
- 减少刷新频率
- 关闭不必要的外设

### 提高响应速度
- 异步网络请求
- 缓存时间偏移
- 优化绘图算法

## 参考资料

- [TuyaOpen 文档](https://tuyaopen.ai/docs)
- [WorldTimeAPI 文档](http://worldtimeapi.org/)
- [e-Paper 显示屏规格](https://www.waveshare.com/)
- [HTTP 协议 RFC](https://tools.ietf.org/html/rfc2616)

## 许可证

基于 Waveshare e-Paper 示例代码修改。

## 更新日志

详见 [CHANGELOG.md](./CHANGELOG.md)


### 1. 激活开发环境

```bash
source export.sh
```

### 2. 配置项目

```bash
tos.py config
```

在配置菜单中选择：
- Platform: T5AI
- Board: TUYA_T5AI_BOARD（或您使用的具体板型）
- Example: examples/e-Paper/4.26inch_e-Paper_clock

### 3. 编译项目

```bash
tos.py build
```

### 4. 烧录到设备

```bash
tos.py flash
```

### 5. 查看串口输出

```bash
tos.py monitor
```

## 代码结构

### 主要文件

- `examples/main.c` - 程序入口点，初始化日志系统并启动时钟任务
- `examples/EPD_4in26_clock.c` - 时钟显示核心逻辑
- `examples/EPD_Test.h` - 头文件声明
- `lib/` - 电子墨水屏驱动库（从原示例复制）
  - `lib/e-Paper/` - EPD 驱动
  - `lib/GUI/` - 图形绘制函数
  - `lib/Fonts/` - 字体库
  - `lib/Config/` - 硬件配置

### 工作流程

1. **初始化阶段**
   - 初始化电子墨水屏硬件
   - 清空屏幕
   - 全屏刷新显示标题和分隔线

2. **循环显示阶段**
   - 获取系统当前时间
   - 格式化日期、时间和星期字符串
   - 清空部分刷新缓冲区
   - 绘制日期、时间和星期信息
   - 使用部分刷新更新显示区域
   - 等待 1 秒
   - 重复上述步骤

### 时间源说明

程序使用标准 C 库的时间函数（`time()` 和 `localtime()`）获取系统时间。请确保您的设备系统时间已正确设置。

## 自定义选项

您可以根据需要修改以下参数：

### 字体大小
```c
// 在 EPD_4in26_clock.c 中修改
Paint_DrawString_EN(20, 10, date_buffer, &Font24, BLACK, WHITE);  // 改为 &Font20 或 &Font16
```

### 显示位置
```c
// 修改部分刷新的位置和大小
EPD_4in26_Display_Part(BlackImage, 50, 60, 300, 150);
//                                  ^   ^   ^    ^
//                                  X   Y   宽   高
```

### 更新频率
```c
// 修改延迟时间（毫秒）
DEV_Delay_ms(1000);  // 改为 500 表示 0.5 秒更新一次
```

### 日期时间格式
```c
// 修改 strftime 格式字符串
strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", timeinfo);     // 日期格式
strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", timeinfo);     // 时间格式
// 可以改为："%Y/%m/%d" 或 "%d-%m-%Y" 等
```

## 注意事项

⚠️ **重要提示**

1. **刷新频率**：电子墨水屏刷新速度较慢，频繁刷新可能影响显示屏寿命
2. **残影问题**：长时间使用部分刷新后可能出现残影，建议定期进行全屏刷新
3. **持续运行**：程序设计为持续运行，不会自动退出
4. **时间同步**：如需准确时间，请确保设备已通过 NTP 或其他方式同步系统时间

## 扩展建议

可以在此基础上添加以下功能：

- 添加温度、湿度等传感器数据显示
- 连接 Wi-Fi 并通过 NTP 自动同步时间
- 添加闹钟功能
- 显示农历日期
- 添加天气信息显示
- 定期全屏刷新以清除残影

## 故障排除

### 屏幕无显示
- 检查硬件连接是否正确
- 确认电源供电正常
- 查看串口输出是否有错误信息

### 时间不准确
- 检查系统时间设置
- 考虑添加 NTP 时间同步功能

### 显示有残影
- 增加全屏刷新频率
- 在循环中定期调用 `EPD_4in26_Clear()`

## 许可证

基于 Waveshare e-Paper 示例代码修改。
