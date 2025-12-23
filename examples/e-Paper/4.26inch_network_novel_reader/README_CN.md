# 4.26寸墨水屏网络小说阅读器

这是一个基于 TuyaOpen SDK 的墨水屏网络小说阅读器项目，支持从网络下载 GBK 编码的中文小说并通过按钮翻页。

## 功能特性

- ✅ WiFi 网络连接
- ✅ HTTP 下载小说内容（支持 GBK 编码）
- ✅ 墨水屏显示中文文本
- ✅ 按钮翻页控制
  - 短按：下一页
  - 长按（3秒）：上一页
- ✅ 智能分页（支持中英文混排）
- ✅ GBK 编码支持

## 硬件要求

- 4.26寸墨水屏模块（800x480）
- ESP32 或其他支持的开发板
- 1个按钮（连接到 GPIO）
- WiFi 网络环境

## 配置说明

### 1. WiFi 配置

编辑 `examples/EPD_4in26_network_novel.c` 文件，修改以下配置：

```c
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"
```

### 2. 小说 URL 配置

默认配置为：

```c
#define NOVEL_URL "http://120.79.89.230/fanren.txt"
```

你可以修改为任何支持 HTTP 的小说 URL。**注意：小说文件必须是 GBK 编码的文本文件。**

### 3. 按钮配置

按钮配置在 `lib/Config/board_button_config.h` 中：

```c
#define BOARD_BUTTON_PIN TY_GPIOA_6
#define BOARD_BUTTON_ACTIVE_LV TUYA_GPIO_LEVEL_LOW
#define BUTTON_NAME "user_button"
```

**只使用第一个按钮**，短按下一页，长按上一页。

## 编译和烧录

### 1. 激活环境

```bash
source export.sh  # Linux/Mac
```

### 2. 配置项目

```bash
cd examples/e-Paper/4.26inch_network_novel_reader
tos.py config
```

选择你的平台和开发板（推荐 T5AI）。

### 3. 编译

```bash
tos.py build
```

### 4. 烧录

```bash
tos.py flash
```

### 5. 查看日志

```bash
tos.py monitor
```

## 使用说明

1. 上电后，设备会自动连接 WiFi
2. 连接成功后，显示 "WiFi Connected! Press button to load novel"
3. **按下按钮**开始从网络下载小说
4. 下载完成后自动显示第一页
5. 使用按钮翻页：
   - **短按**：下一页
   - **长按（3秒）**：上一页

## GBK 编码说明

本项目支持 GBK 编码的中文小说：

- **GBK 字符检测**：自动识别 GBK 双字节字符（0x81-0xFE）
- **智能分页**：根据字符宽度（中文2单位，英文1单位）进行分页
- **混排支持**：支持中英文混合显示

### 如何准备 GBK 编码的小说文件

如果你的小说是 UTF-8 编码，需要转换为 GBK：

```bash
# Linux/Mac
iconv -f UTF-8 -t GBK input.txt -o output.txt

# Python
python -c "open('output.txt','wb').write(open('input.txt','r',encoding='utf-8').read().encode('gbk'))"
```

## 显示参数

- **屏幕分辨率**：800x480
- **每行字符数**：40（中文约20个字，英文40个字符）
- **每页行数**：22
- **字体**：Font16（16像素高度）
- **中文显示**：使用占位符 `[]` 表示（完整实现需要 GBK 字库）

## 内嵌小说功能（可选）

如果不想从网络下载，可以使用内嵌小说功能：

1. 将 GBK 编码的小说文本文件放在项目根目录
2. 运行转换脚本：

```bash
python tools/txt_to_c_array.py your_novel.txt
```

3. 这会生成 `examples/embedded_novel.c` 和 `examples/embedded_novel.h`
4. 重新编译项目

## 故障排除

### WiFi 连接失败

- 检查 SSID 和密码是否正确
- 确认 WiFi 信号强度
- 查看串口日志：`tos.py monitor`

### 按钮无响应

- 检查按钮 GPIO 配置是否正确
- 确认按钮硬件连接
- 检查按钮电平配置（上拉/下拉）
- 查看日志中的按钮事件

### 小说下载失败

- 确认网络连接正常
- 检查 URL 是否可访问（可用浏览器测试）
- 查看 HTTP 响应状态码
- 确认服务器支持 HTTP（不是 HTTPS）

### 中文显示为方框

这是正常的，当前版本使用占位符 `[]` 表示 GBK 字符。完整的中文显示需要：

1. 添加 GBK 字库文件
2. 实现 GBK 到字库的映射
3. 修改 `display_page()` 函数调用中文字体绘制

## 技术细节

### 网络模块

- 使用 TuyaOpen 的 `netmgr` 进行网络管理
- 使用 `http_client_interface` 进行 HTTP 请求
- 支持 lwIP 协议栈

### 按钮模块

- 使用 TDL (Tuya Device Layer) 按钮管理
- 支持短按、长按事件
- 长按时间：3000ms（可配置）

### 显示模块

- 使用 Waveshare 4.26寸墨水屏驱动
- 支持黑白显示
- 使用 GUI_Paint 库进行图形绘制

### GBK 编码处理

- **字符识别**：`is_gbk_lead_byte()` 检测 GBK 首字节
- **字节长度**：`get_char_byte_len()` 返回字符字节数
- **智能分页**：`calculate_page_offsets()` 根据字符宽度计算页偏移

## 代码结构

```
4.26inch_network_novel_reader/
├── CMakeLists.txt              # 编译配置
├── app_default.config          # 板级和功能配置
├── README.md                   # 英文说明
├── README_CN.md                # 中文说明(本文件)
├── BUTTON_SETUP.md             # 按钮配置说明
├── examples/
│   ├── main.c                  # 入口点
│   ├── EPD_Test.h              # 头文件
│   ├── EPD_4in26_network_novel.c  # 主要实现
│   ├── embedded_novel.c        # 内嵌小说数据
│   └── embedded_novel.h        # 内嵌小说头文件
├── lib/                        # 墨水屏驱动库
│   ├── Config/                 # 硬件配置
│   ├── e-Paper/                # 墨水屏驱动
│   ├── Fonts/                  # 字体数据
│   └── GUI/                    # 图形函数
└── tools/
    └── txt_to_c_array.py       # 文本转C数组工具
```

## 下一步改进

- [ ] 添加完整的 GBK 字库支持
- [ ] 支持 UTF-8 编码
- [ ] 添加书签功能
- [ ] 支持多本小说切换
- [ ] 添加 SD 卡存储支持
- [ ] 支持 HTTPS 下载

## 许可证

Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.

本项目基于 TuyaOpen SDK，遵循相应的开源许可证。

## 技术支持

如有问题:
- 查看TuyaOpen文档
- 参考 `examples/e-Paper/` 中的示例代码
- 查阅 `boards/T5AI/` 中的板级文档
