# Changelog - SD Card Feature

## Version 1.1.0 (2025-01-24)

### 新增功能 / New Features

#### SD 卡文件浏览器
- ✅ 完整的文件浏览器界面
- ✅ 支持 TXT、BMP、PNG、JPG 文件类型识别
- ✅ 可视化文件列表，高亮当前选中项
- ✅ 支持最多 50 个文件

#### 文本文件阅读
- ✅ 读取 SD 卡上的 TXT 文件
- ✅ 自动分页显示
- ✅ 支持 GBK 和 UTF-8 编码
- ✅ 中英文混合显示（HZK24 字体）

#### 图片查看
- ✅ 显示 BMP 单色位图
- ✅ 使用 TuyaOpen 文件系统 API（兼容性更好）
- ✅ 手动解析 BMP 格式
- ✅ 支持任意尺寸图片

#### 按钮控制优化
- ✅ 长按时间从 3 秒改为 1.5 秒（更易触发）
- ✅ 三种模式：文件浏览、文本阅读、图片查看
- ✅ 智能模式切换

#### 示例文件
- ✅ 自动创建示例文本文件
- ✅ 自动创建示例 BMP 图片
- ✅ 首次运行即可体验

### 技术改进 / Technical Improvements

#### 文件系统兼容性
- 🔧 使用 TuyaOpen 文件系统 API（`tkl_fopen` 等）
- 🔧 不依赖标准 C 库的 `fopen`
- 🔧 更好的跨平台兼容性

#### BMP 图片解析
- 🔧 手动解析 BMP 文件头
- 🔧 支持 1-bit 单色 BMP
- 🔧 正确处理调色板
- 🔧 处理 BMP 倒置存储（bottom-up）

#### 内存管理
- 🔧 动态分配缓冲区
- 🔧 及时释放内存
- 🔧 错误处理完善

### 文件结构 / File Structure

```
新增文件：
├── include/sd_file_manager.h          # SD 卡管理接口
├── src/sd_file_manager.c              # SD 卡管理实现
├── tools/convert_image_to_bmp.py      # 图片转换工具
├── SD_CARD_GUIDE.md                   # 完整使用指南
├── SD_CARD_QUICKSTART.md              # 快速开始
├── SD_CARD_FEATURE_SUMMARY.md         # 功能总结
└── CHANGELOG.md                       # 本文件

修改文件：
├── examples/EPD_4in26_network_novel.c # 主程序（添加 SD 卡支持）
├── CMakeLists.txt                     # 构建配置
└── README.md                          # 项目文档
```

### API 变更 / API Changes

#### 新增 API

```c
// SD 卡初始化
int sd_card_init(void);

// 创建示例文件
int sd_create_sample_files(void);

// 扫描文件
int sd_scan_files(file_browser_t *browser);

// 读取文本文件
int sd_read_text_file(const char *filepath, char **buffer, int *size);

// 显示 BMP 图片（使用 TuyaOpen API）
int sd_display_bmp_image(const char *filepath);

// 获取文件类型
file_type_e sd_get_file_type(const char *filename);
```

#### 新增数据结构

```c
// 文件类型
typedef enum {
    FILE_TYPE_UNKNOWN = 0,
    FILE_TYPE_TXT,
    FILE_TYPE_BMP,
    FILE_TYPE_PNG,
    FILE_TYPE_JPG,
    FILE_TYPE_DIR
} file_type_e;

// 文件信息
typedef struct {
    char name[MAX_FILENAME_LEN];
    file_type_e type;
    int size;
} file_info_t;

// 文件浏览器
typedef struct {
    file_info_t files[MAX_FILES];
    int file_count;
    int current_index;
    int is_file_open;
    char current_path[256];
} file_browser_t;

// 应用模式
typedef enum {
    MODE_FILE_BROWSER = 0,
    MODE_TEXT_READER,
    MODE_IMAGE_VIEWER
} app_mode_e;
```

### 配置变更 / Configuration Changes

#### 按钮长按时间
```c
// 旧值：3000ms (3秒)
// 新值：1500ms (1.5秒)
#define BUTTON_LONG_PRESS_TIME 1500
```

#### SD 卡挂载路径
```c
#define SDCARD_MOUNT_PATH "/sdcard"
```

#### 文件限制
```c
#define MAX_FILES 50              // 最多 50 个文件
#define MAX_FILENAME_LEN 128      // 文件名最大长度
```

### 使用方法 / Usage

#### 1. 准备 SD 卡
```bash
# 格式化为 FAT32
# 复制 TXT 文件到根目录
# 使用工具转换图片
python tools/convert_image_to_bmp.py photo.jpg photo.bmp
```

#### 2. 编译和烧录
```bash
source export.sh
tos.py build
tos.py flash
```

#### 3. 使用设备
- 开机自动扫描 SD 卡
- 短按切换文件
- 长按（1.5秒）打开文件
- 阅读/查看内容
- 长按返回浏览器

### 已知问题 / Known Issues

1. **PNG/JPG 支持**
   - 状态：计划中
   - 临时方案：使用转换工具转为 BMP

2. **子目录浏览**
   - 状态：未实现
   - 当前：仅支持根目录

3. **文件排序**
   - 状态：未实现
   - 当前：按扫描顺序显示

### 性能优化 / Performance

- 文本文件：< 1MB（推荐 < 500KB）
- 图片文件：800x480 或更小
- 文件数量：< 50 个
- 显示刷新：~2 秒

### 兼容性 / Compatibility

#### 支持的平台
- ✅ T5AI
- ✅ ESP32 系列
- ✅ 其他支持 SD 卡的平台

#### 支持的文件系统
- ✅ FAT32（推荐）
- ✅ FAT16

#### 支持的编码
- ✅ GBK（中文）
- ✅ UTF-8
- ✅ ASCII

### 故障排除 / Troubleshooting

#### 编译错误
```bash
# 确保包含了必要的头文件
#include "GUI_Paint.h"  // BLACK, WHITE, Paint_SetPixel

# 清理并重新编译
tos.py clean
tos.py build
```

#### SD 卡未检测
- 检查硬件连接
- 确认 SD 卡已格式化（FAT32）
- 查看串口日志

#### 图片无法显示
- 确保是 1-bit 单色 BMP
- 使用提供的转换工具
- 检查文件路径

#### 长按难以触发
- 已优化为 1.5 秒
- 确保按钮硬件正常
- 查看按钮事件日志

### 未来计划 / Future Plans

- [ ] PNG/JPG 直接支持（带抖动算法）
- [ ] 子目录导航
- [ ] 文件排序功能
- [ ] 书签支持
- [ ] 图片缩放和平移
- [ ] 文件搜索
- [ ] 最近打开列表

### 贡献者 / Contributors

- 初始实现：2025-01-24
- SD 卡功能：完整实现
- 文档：中英文双语

### 参考文档 / References

- [SD_CARD_QUICKSTART.md](./SD_CARD_QUICKSTART.md) - 快速开始
- [SD_CARD_GUIDE.md](./SD_CARD_GUIDE.md) - 完整指南
- [SD_CARD_FEATURE_SUMMARY.md](./SD_CARD_FEATURE_SUMMARY.md) - 功能总结
- [README.md](./README.md) - 项目总览

---

## 版本历史 / Version History

### v1.1.0 (2025-01-24)
- ✅ 添加 SD 卡文件浏览器
- ✅ 添加 TXT 文件阅读
- ✅ 添加 BMP 图片显示
- ✅ 优化按钮控制
- ✅ 添加示例文件生成
- ✅ 添加图片转换工具
- ✅ 完善文档

### v1.0.0 (之前)
- ✅ 网络小说下载
- ✅ 电子墨水屏显示
- ✅ 按钮翻页
- ✅ GBK 中文支持
- ✅ 内置小说
