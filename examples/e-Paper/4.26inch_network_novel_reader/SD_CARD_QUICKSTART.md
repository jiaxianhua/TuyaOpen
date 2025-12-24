# SD Card Quick Start Guide

## 快速开始

### 1. 硬件准备

连接 SD 卡模块到开发板（具体引脚参考板级配置）

### 2. 准备文件

#### 文本文件 (TXT)
在 SD 卡根目录放置 `.txt` 文件：
```
/sdcard/
├── novel.txt
├── readme.txt
└── ...
```

#### 图片文件 (BMP)
使用工具转换图片为单色 BMP：
```bash
cd tools
python convert_image_to_bmp.py photo.jpg photo.bmp
```

将生成的 `.bmp` 文件复制到 SD 卡根目录。

### 3. 使用方法

1. **开机** - 系统自动扫描 SD 卡文件
2. **浏览** - 短按按钮切换文件
3. **打开** - 长按按钮（3秒）打开选中的文件
4. **阅读/查看** - 短按翻页（文本），长按返回（图片/文本）

### 4. 按钮控制

| 模式 | 短按 | 长按（3秒） |
|------|------|-------------|
| 文件浏览 | 下一个文件 | 打开文件 |
| 文本阅读 | 下一页 | 返回浏览器 |
| 图片查看 | - | 返回浏览器 |

### 5. 支持的文件格式

- ✅ **TXT** - 文本文件（支持中英文混合，GBK/UTF-8编码）
- ✅ **BMP** - 位图图片（推荐单色 1-bit）
- ⏳ **PNG** - PNG图片（即将支持）
- ⏳ **JPG** - JPEG图片（即将支持）

### 6. 示例文件

首次运行时，系统会自动创建示例文件：
- `sample1.txt` - 中英文混合文本示例
- `readme.txt` - 英文说明文档
- `smiley.bmp` - 16x16 笑脸图标

### 7. 故障排除

**SD 卡未检测到**
- 检查硬件连接
- 确认 SD 卡已格式化（推荐 FAT32）
- 查看串口日志

**文件不显示**
- 确认文件扩展名正确（.txt, .bmp）
- 文件名不要以 `.` 开头
- 文件需在根目录
- 最多支持 50 个文件

**图片显示异常**
- 使用单色 BMP 格式
- 推荐尺寸：800x480 或更小
- 使用提供的转换工具

### 8. 图片转换工具

```bash
# 安装依赖
pip install Pillow

# 转换单个图片
python tools/convert_image_to_bmp.py input.jpg output.bmp

# 自定义尺寸
python tools/convert_image_to_bmp.py input.jpg output.bmp --width 800 --height 480

# 调整阈值（控制黑白对比度）
python tools/convert_image_to_bmp.py input.jpg output.bmp --threshold 150

# 批量转换
python tools/convert_image_to_bmp.py input_dir/ output_dir/ --batch
```

### 9. 性能建议

- 文本文件：建议 < 500KB
- 图片尺寸：800x480 或更小
- 文件数量：建议 < 50 个
- 格式：单色 BMP 显示最快

### 10. 更多信息

详细文档请参考：
- [SD_CARD_GUIDE.md](./SD_CARD_GUIDE.md) - 完整功能说明
- [README.md](./README.md) - 项目总览
- [GBK_SUPPORT.md](./GBK_SUPPORT.md) - 中文字体支持

## Quick Start (English)

### 1. Hardware Setup
Connect SD card module to your board (check board configuration for pins)

### 2. Prepare Files

**Text Files (TXT)**
Place `.txt` files in SD card root:
```
/sdcard/
├── novel.txt
├── readme.txt
└── ...
```

**Image Files (BMP)**
Convert images to monochrome BMP:
```bash
cd tools
python convert_image_to_bmp.py photo.jpg photo.bmp
```

Copy generated `.bmp` files to SD card root.

### 3. Usage

1. **Power On** - System scans SD card automatically
2. **Browse** - Short press to switch files
3. **Open** - Long press (3s) to open selected file
4. **Read/View** - Short press for next page (text), long press to return (image/text)

### 4. Button Controls

| Mode | Short Press | Long Press (3s) |
|------|-------------|-----------------|
| File Browser | Next file | Open file |
| Text Reader | Next page | Return to browser |
| Image Viewer | - | Return to browser |

### 5. Supported Formats

- ✅ **TXT** - Text files (Chinese/English, GBK/UTF-8)
- ✅ **BMP** - Bitmap images (1-bit monochrome recommended)
- ⏳ **PNG** - PNG images (coming soon)
- ⏳ **JPG** - JPEG images (coming soon)

For more details, see [SD_CARD_GUIDE.md](./SD_CARD_GUIDE.md)
