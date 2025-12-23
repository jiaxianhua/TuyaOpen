# 快速参考卡片

## 显示所有中文字符（推荐）

```bash
# 1. 下载 HZK16 字库
wget https://github.com/aguegu/BitmapFont/raw/master/font/HZK16

# 2. 一键生成和编译
./build_with_full_font.sh  # Linux/Mac
# 或
build_with_full_font.bat   # Windows

# 3. 烧录
tos.py flash
```

**结果**：支持显示所有 GB2312 汉字（6,763 个），可以阅读任何网络小说！

---

## 三种字库方案对比

| 方案 | 字符数 | 大小 | 适用场景 |
|------|--------|------|----------|
| **内置字符** | ~10 | 1KB | 测试 |
| **专用字库** | 500-2000 | 17-68KB | 单本小说 |
| **完整字库** ⭐ | 6,763 | 230KB | 网络下载 |

---

## 常用命令

### 编译和烧录
```bash
tos.py build    # 编译
tos.py flash    # 烧录
tos.py monitor  # 查看日志
```

### 字库管理
```bash
# 生成完整字库
python tools/generate_full_hzk16.py HZK16 hzk16_full.c

# 生成专用字库（单本小说）
python tools/generate_hzk16_data.py novel.txt HZK16 hzk16.c

# 恢复默认字库
cp lib/Fonts/hzk16_default.c.bak lib/Fonts/hzk16.c
```

### 文本转换
```bash
# UTF-8 转 GBK
iconv -f UTF-8 -t GBK input.txt -o output.txt

# 查看编码
file -i novel.txt
```

---

## 配置修改

### WiFi 配置
编辑 `examples/EPD_4in26_network_novel.c`：
```c
#define WIFI_SSID "你的WiFi"
#define WIFI_PASSWORD "你的密码"
```

### 小说 URL
```c
#define NOVEL_URL "http://120.79.89.230/fanren.txt"
```

### 显示参数
```c
#define CHARS_PER_LINE 40  // 每行字符数
#define LINES_PER_PAGE 22  // 每页行数
```

### 按钮配置
编辑 `lib/Config/board_button_config.h`：
```c
#define BOARD_BUTTON_PIN TY_GPIOA_6
#define BOARD_BUTTON_ACTIVE_LV TUYA_GPIO_LEVEL_LOW
```

---

## 故障排除

### WiFi 连接失败
```bash
# 查看日志
tos.py monitor

# 检查：SSID、密码、信号强度
```

### 按钮无响应
```bash
# 检查 GPIO 配置
# 检查按钮硬件连接
# 查看日志中的按钮事件
```

### 中文显示为方框
```bash
# 使用完整字库
./build_with_full_font.sh

# 或检查文本编码
file -i novel.txt
```

### 编译时间过长
```bash
# 使用并行编译
tos.py build -j4
```

---

## 文件大小

| 项目 | 大小 | 说明 |
|------|------|------|
| 完整字库源码 | 230KB | hzk16_full.c |
| 编译后固件 | +230KB | 嵌入到 Flash |
| 显示缓冲区 | 48KB | RAM |
| 小说内容 | 可变 | RAM |

---

## 性能数据

| 操作 | 时间 | 说明 |
|------|------|------|
| WiFi 连接 | 5-10s | 取决于网络 |
| 下载小说 | 可变 | 取决于大小和网速 |
| 计算分页 | <1s | 100KB 文本 |
| 字符查找 | ~4μs | 二分查找 |
| 页面刷新 | ~2s | 墨水屏刷新 |

---

## 文档索引

| 文档 | 说明 |
|------|------|
| **QUICKSTART.md** | 5分钟快速开始 |
| **README_CN.md** | 完整使用说明 |
| **FULL_FONT_GUIDE.md** | 完整字库指南 ⭐ |
| **HZK16_GUIDE.md** | HZK16 技术细节 |
| **GBK_SUPPORT.md** | GBK 编码说明 |
| **BUTTON_SETUP.md** | 按钮配置 |
| **PROJECT_SUMMARY.md** | 项目总结 |

---

## 按钮操作

- **短按**：下一页
- **长按（3秒）**：上一页
- **首次按下**：开始下载小说

---

## 支持的编码

| 编码 | 支持 | 说明 |
|------|------|------|
| **GBK** | ✅ | 推荐使用 |
| **GB2312** | ✅ | 完整支持 |
| UTF-8 | ❌ | 需转换为 GBK |
| Big5 | ❌ | 繁体字不支持 |

---

## 下载链接

- **HZK16 字库**：https://github.com/aguegu/BitmapFont/raw/master/font/HZK16
- **TuyaOpen SDK**：https://github.com/tuya/tuyaopen
- **项目文档**：README_CN.md

---

## 技术支持

遇到问题？
1. 查看日志：`tos.py monitor`
2. 检查配置：WiFi、URL、按钮
3. 参考文档：README_CN.md
4. 查看示例：examples/

---

**提示**：使用完整字库后，你可以阅读任何从网络下载的中文小说！🎉
