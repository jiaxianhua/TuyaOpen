# 壁纸功能说明

## 功能概述

电子墨水屏阅读器现在支持自动下载和显示壁纸功能。启动时会显示壁纸，然后进入文件浏览器。

## 工作流程

### 首次启动（无壁纸）

```
1. 检测SD卡 ✓
2. 扫描文件 ✓
3. 检查壁纸 → 不存在或检查失败
4. 显示"Syncing time..." ✓
5. 连接WiFi并同步时间 ✓
6. 显示"Downloading wallpaper..." ✓
7. 从服务器下载壁纸 ✓
8. 保存到 /sdcard/wallpaper.bmp ✓
9. 显示壁纸 3秒 ✓
10. 进入文件浏览器 ✓
```

### 后续启动（有壁纸）

```
1. 检测SD卡 ✓
2. 扫描文件 ✓
3. 检查壁纸 → 存在 ✓
4. 显示壁纸 3秒 ✓
5. 显示"Syncing time..." ✓
6. 连接WiFi并同步时间 ✓
7. 进入文件浏览器 ✓
```

**注意**: 如果壁纸文件检查失败（例如文件系统错误），系统会自动尝试下载壁纸，确保功能正常工作。

## 配置

在 `EPD_4in26_network_novel.c` 文件顶部：

```c
// Wallpaper settings
#define WALLPAPER_URL "http://120.79.89.230:8001/files/wallpaper.bmp"
#define WALLPAPER_HOST "120.79.89.230"
#define WALLPAPER_PORT 8001
#define WALLPAPER_PATH "/files/wallpaper.bmp"
#define WALLPAPER_FILE "/sdcard/wallpaper.bmp"
#define WALLPAPER_TIMEOUT 30000  // 30 seconds for image download
```

## 修改配置

### 更换壁纸服务器

```c
#define WALLPAPER_HOST "your-server.com"
#define WALLPAPER_PORT 80
#define WALLPAPER_PATH "/images/wallpaper.bmp"
```

### 更改显示时间

在主函数中修改：

```c
tal_system_sleep(3000);  // 改为 5000 = 5秒
```

### 更改保存位置

```c
#define WALLPAPER_FILE "/sdcard/my_wallpaper.bmp"
```

## 壁纸要求

### 格式要求

- **格式**: BMP (Bitmap)
- **支持位深度**: 
  - 1-bit 单色
  - 24-bit 彩色（自动转换为单色）
  - 32-bit 彩色（自动转换为单色）

### 尺寸建议

- **推荐尺寸**: 800x480 像素（横屏）或 480x800 像素（竖屏）
- **最大文件大小**: 10 MB
- **实际显示**: 使用 ROTATE_90，逻辑尺寸为 480x800

### 制作壁纸

使用项目提供的转换工具：

```bash
# 使用 Python 工具
python tools/convert_image_to_bmp.py input.jpg wallpaper.bmp

# 使用 Java 工具
cd tools/tuya-converter
./tuya-convert.sh input.jpg wallpaper.bmp
```

## 手动管理壁纸

### 手动添加壁纸

1. 准备一个 BMP 格式的图片
2. 重命名为 `wallpaper.bmp`
3. 复制到 SD 卡根目录
4. 重启设备

### 删除壁纸

从 SD 卡删除 `wallpaper.bmp` 文件，下次启动会重新下载。

### 更新壁纸

方法1：删除旧壁纸，重启自动下载新的

方法2：直接替换 SD 卡上的 `wallpaper.bmp` 文件

## 网络要求

### 下载壁纸需要

- WiFi 连接正常
- 能访问壁纸服务器
- 服务器返回 HTTP 200 状态码

### 如果网络失败

- 如果已有壁纸：显示已保存的壁纸
- 如果无壁纸：跳过壁纸显示，直接进入文件浏览器

## 日志示例

### 首次启动（下载壁纸）

```
[ty N] Checking SD card...
[ty N] SD card initialized
[ty N] Found 20 files on SD card, using SD card mode
[ty D] Wallpaper file not found: /sdcard/wallpaper.bmp
[ty N] No wallpaper found, will download after time sync
[ty N] Attempting to sync time from network...
[ty N] Connecting to WiFi: 1519
[ty N] Network connected
[ty N] Time synced successfully
[ty N] Downloading wallpaper...
[ty N] Downloading wallpaper from: http://120.79.89.230:8001/files/wallpaper.bmp
[ty N] Downloaded 245678 bytes, saving to SD card...
[ty N] Wallpaper saved successfully: /sdcard/wallpaper.bmp
[ty N] Wallpaper downloaded, displaying...
[ty N] Displaying wallpaper: /sdcard/wallpaper.bmp
[ty N] BMP: 800x480, 24 bits per pixel, compression=0
[ty N] Converting 24-bit color BMP to monochrome
[ty N] BMP image displayed successfully
[ty N] Wallpaper displayed successfully
```

### 后续启动（已有壁纸）

```
[ty N] Checking SD card...
[ty N] SD card initialized
[ty N] Found 20 files on SD card, using SD card mode
[ty N] Displaying wallpaper: /sdcard/wallpaper.bmp
[ty N] BMP: 800x480, 24 bits per pixel, compression=0
[ty N] Converting 24-bit color BMP to monochrome
[ty N] BMP image displayed successfully
[ty N] Wallpaper displayed, waiting 3 seconds...
[ty N] Attempting to sync time from network...
[ty N] Time synced successfully
```

## 故障排除

### 壁纸不显示

1. 检查 SD 卡是否正常挂载
2. 检查 `/sdcard/wallpaper.bmp` 是否存在
3. 检查文件格式是否为 BMP
4. 查看日志中的错误信息
5. 如果文件检查失败，系统会自动尝试下载

### 下载失败

1. 检查 WiFi 连接
2. 检查服务器地址和端口
3. 确认服务器上有 wallpaper.bmp 文件
4. 检查网络超时设置（默认30秒）
5. 查看日志确认是否是文件系统检查失败导致的重复下载

### 显示异常

1. 确认图片尺寸合适（推荐 800x480）
2. 确认是 BMP 格式
3. 尝试使用转换工具重新生成

### 文件检查失败

如果看到日志 `tkl_fs_is_exist path:/sdcard/wallpaper.bmp stat failed`：
- 这是正常的，系统会自动尝试下载壁纸
- 下载成功后，后续启动应该能正常显示
- 如果持续失败，检查 SD 卡文件系统是否正常

## 技术细节

### 函数说明

#### `display_wallpaper()`

显示 SD 卡上的壁纸文件。

- **返回**: OPRT_OK 成功，其他值失败
- **副作用**: 更新屏幕显示

#### `download_wallpaper()`

从服务器下载壁纸并保存到 SD 卡。

- **前提**: 网络已连接
- **返回**: OPRT_OK 成功，其他值失败
- **副作用**: 在 SD 卡创建 wallpaper.bmp 文件

### 内存使用

- 下载时：使用 HTTP 客户端缓冲区（由 SDK 管理）
- 显示时：使用现有的图像缓冲区（g_image_buffer）
- 无额外内存开销

### 性能

- 下载时间：取决于网络速度和文件大小
  - 100 KB 文件 @ 1 Mbps ≈ 1 秒
  - 500 KB 文件 @ 1 Mbps ≈ 5 秒
- 显示时间：约 2-3 秒（电子墨水屏刷新）
- 总启动延迟：首次约 10-15 秒，后续约 5-8 秒

## 相关文件

- `EPD_4in26_network_novel.c` - 主程序，包含壁纸功能
- `sd_file_manager.c` - SD 卡文件管理，包含 BMP 显示
- `tools/convert_image_to_bmp.py` - 图片转换工具
- `tools/tuya-converter/` - Java 图片转换工具

## 未来改进

可能的功能增强：

1. 支持多张壁纸轮播
2. 支持 PNG/JPG 格式（需要解码库）
3. 壁纸缓存管理（自动清理旧壁纸）
4. 壁纸更新检查（基于时间戳或版本号）
5. 壁纸下载进度显示
6. 支持从多个服务器下载（备用服务器）
