# SD Card Feature Summary

## 功能概述 / Feature Overview

为 4.26 英寸电子墨水屏网络小说阅读器添加了完整的 SD 卡文件浏览和显示功能。

Added complete SD card file browsing and display functionality to the 4.26-inch E-Paper network novel reader.

## 新增功能 / New Features

### 1. 文件浏览器 / File Browser
- ✅ 扫描 SD 卡根目录的所有支持文件
- ✅ 可视化文件列表界面
- ✅ 显示文件类型标签 ([TXT], [BMP], [PNG], [JPG])
- ✅ 高亮显示当前选中文件
- ✅ 支持最多 50 个文件

### 2. 文本文件阅读 / Text File Reading
- ✅ 读取 TXT 文件内容
- ✅ 自动分页显示
- ✅ 支持 GBK 和 UTF-8 编码
- ✅ 中英文混合显示（使用 HZK24 字体）
- ✅ 按钮翻页控制

### 3. 图片查看 / Image Viewing
- ✅ 显示 BMP 单色位图
- ✅ 支持任意尺寸（推荐 800x480）
- ✅ 自动转换为黑白显示
- ⏳ PNG/JPG 支持（即将推出）

### 4. 按钮控制 / Button Controls
- ✅ 短按：浏览模式下切换文件，阅读模式下翻页
- ✅ 长按（3秒）：打开/关闭文件
- ✅ 三种模式：文件浏览、文本阅读、图片查看

### 5. 默认示例文件 / Default Sample Files
- ✅ 自动创建示例文本文件（中英文）
- ✅ 自动创建示例图片（笑脸图标）
- ✅ 首次运行即可体验完整功能

## 文件结构 / File Structure

```
examples/e-Paper/4.26inch_network_novel_reader/
├── include/
│   └── sd_file_manager.h          # SD 卡管理头文件
├── src/
│   └── sd_file_manager.c          # SD 卡管理实现
├── examples/
│   └── EPD_4in26_network_novel.c  # 主程序（已更新）
├── tools/
│   └── convert_image_to_bmp.py    # 图片转换工具
├── SD_CARD_GUIDE.md               # 完整使用指南
├── SD_CARD_QUICKSTART.md          # 快速开始指南
└── SD_CARD_FEATURE_SUMMARY.md     # 本文件
```

## 代码修改 / Code Changes

### 新增文件 / New Files
1. **include/sd_file_manager.h** - SD 卡文件管理接口
2. **src/sd_file_manager.c** - SD 卡文件管理实现
3. **tools/convert_image_to_bmp.py** - 图片转换工具

### 修改文件 / Modified Files
1. **examples/EPD_4in26_network_novel.c**
   - 添加 SD 卡初始化
   - 添加文件浏览器界面
   - 添加多模式切换逻辑
   - 更新按钮控制逻辑

2. **CMakeLists.txt**
   - 添加 src/ 目录源文件
   - 添加 include/ 目录头文件

## API 接口 / API Interface

### SD 卡初始化
```c
int sd_card_init(void);
```

### 创建示例文件
```c
int sd_create_sample_files(void);
```

### 扫描文件
```c
int sd_scan_files(file_browser_t *browser);
```

### 读取文本文件
```c
int sd_read_text_file(const char *filepath, char **buffer, int *size);
```

### 显示 BMP 图片
```c
int sd_display_bmp_image(const char *filepath);
```

### 获取文件类型
```c
file_type_e sd_get_file_type(const char *filename);
```

## 数据结构 / Data Structures

### 文件类型枚举
```c
typedef enum {
    FILE_TYPE_UNKNOWN = 0,
    FILE_TYPE_TXT,
    FILE_TYPE_BMP,
    FILE_TYPE_PNG,
    FILE_TYPE_JPG,
    FILE_TYPE_DIR
} file_type_e;
```

### 文件信息
```c
typedef struct {
    char name[MAX_FILENAME_LEN];
    file_type_e type;
    int size;
} file_info_t;
```

### 文件浏览器
```c
typedef struct {
    file_info_t files[MAX_FILES];
    int file_count;
    int current_index;
    int is_file_open;
    char current_path[256];
} file_browser_t;
```

### 应用模式
```c
typedef enum {
    MODE_FILE_BROWSER = 0,  // 文件浏览模式
    MODE_TEXT_READER,       // 文本阅读模式
    MODE_IMAGE_VIEWER       // 图片查看模式
} app_mode_e;
```

## 使用流程 / Usage Flow

```
开机 Power On
    ↓
初始化 SD 卡 Initialize SD Card
    ↓
扫描文件 Scan Files
    ↓
显示文件浏览器 Display File Browser
    ↓
[短按] 切换文件 [Short Press] Switch Files
    ↓
[长按] 打开文件 [Long Press] Open File
    ↓
┌─────────────┬─────────────┐
│  文本阅读   │  图片查看   │
│ Text Reader │Image Viewer │
│             │             │
│ [短按] 翻页 │ [长按] 返回 │
│ [长按] 返回 │             │
└─────────────┴─────────────┘
    ↓
返回文件浏览器 Return to Browser
```

## 配置选项 / Configuration Options

### SD 卡挂载路径
```c
#define SDCARD_MOUNT_PATH "/sdcard"
```

### 最大文件数
```c
#define MAX_FILES 50
```

### 最大文件名长度
```c
#define MAX_FILENAME_LEN 128
```

## 工具使用 / Tool Usage

### 图片转换工具
```bash
# 安装依赖
pip install Pillow

# 基本用法
python tools/convert_image_to_bmp.py input.jpg output.bmp

# 指定尺寸
python tools/convert_image_to_bmp.py input.jpg output.bmp --width 800 --height 480

# 调整阈值（0-255，越低越黑）
python tools/convert_image_to_bmp.py input.jpg output.bmp --threshold 150

# 批量转换
python tools/convert_image_to_bmp.py input_dir/ output_dir/ --batch
```

## 性能优化 / Performance Optimization

1. **文件大小限制**
   - 文本文件：< 1MB（推荐 < 500KB）
   - 图片文件：800x480 或更小

2. **文件数量限制**
   - 最多 50 个文件
   - 超过限制的文件将被忽略

3. **显示优化**
   - 使用单色 BMP 格式最快
   - 文本使用 HZK24 字体（24x24 像素）
   - 每页显示 32 行文本

## 兼容性 / Compatibility

### 支持的平台
- ✅ T5AI
- ✅ ESP32 系列
- ✅ 其他支持 SD 卡的平台

### 支持的文件系统
- ✅ FAT32（推荐）
- ✅ FAT16
- ⚠️ exFAT（取决于平台支持）

### 支持的编码
- ✅ GBK（中文）
- ✅ UTF-8
- ✅ ASCII

## 已知限制 / Known Limitations

1. **文件位置**
   - 仅支持根目录文件
   - 不支持子目录浏览

2. **图片格式**
   - 当前仅支持 BMP
   - PNG/JPG 需要转换

3. **文件数量**
   - 最多 50 个文件
   - 超过部分被忽略

4. **文本文件**
   - 最大 1MB
   - 超大文件可能导致内存不足

## 未来改进 / Future Improvements

- [ ] PNG/JPG 直接支持（带抖动算法）
- [ ] 子目录导航
- [ ] 文件排序（按名称、类型、大小）
- [ ] 书签功能
- [ ] 图片缩放和平移
- [ ] 文件信息显示（大小、日期）
- [ ] 搜索功能
- [ ] 最近打开列表

## 测试建议 / Testing Recommendations

### 1. 基本功能测试
- [ ] SD 卡挂载成功
- [ ] 文件扫描正常
- [ ] 文件浏览器显示正确
- [ ] 按钮控制响应

### 2. 文本文件测试
- [ ] 纯英文文本
- [ ] 纯中文文本
- [ ] 中英文混合
- [ ] 特殊字符
- [ ] 大文件（> 100KB）

### 3. 图片文件测试
- [ ] 小图片（< 100x100）
- [ ] 中等图片（400x300）
- [ ] 全屏图片（800x480）
- [ ] 不同格式 BMP

### 4. 边界测试
- [ ] 空 SD 卡
- [ ] 单个文件
- [ ] 50 个文件
- [ ] 超过 50 个文件
- [ ] 无效文件名

## 参考文档 / References

1. **SD_CARD_QUICKSTART.md** - 快速开始指南
2. **SD_CARD_GUIDE.md** - 完整功能文档
3. **README.md** - 项目总览
4. **GBK_SUPPORT.md** - 中文字体支持
5. **BUTTON_SETUP.md** - 按钮配置

## 技术支持 / Technical Support

如遇问题，请检查：
1. 串口日志输出
2. SD 卡格式和连接
3. 文件格式和编码
4. 内存使用情况

For issues, please check:
1. Serial log output
2. SD card format and connection
3. File format and encoding
4. Memory usage

## 版本历史 / Version History

### v1.0.0 (2025-01-24)
- ✅ 初始版本
- ✅ 文件浏览器
- ✅ TXT 文件阅读
- ✅ BMP 图片显示
- ✅ 按钮控制
- ✅ 示例文件生成
- ✅ 图片转换工具

---

**注意 / Note**: 此功能基于 TuyaOpen SDK 的文件系统抽象层实现，确保您的平台支持 SD 卡功能。

This feature is implemented based on TuyaOpen SDK's filesystem abstraction layer. Ensure your platform supports SD card functionality.
