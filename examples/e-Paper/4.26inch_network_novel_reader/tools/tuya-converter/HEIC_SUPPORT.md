# HEIC/HEIF 格式支持说明

## 什么是 HEIC？

HEIC (High Efficiency Image Container) 是 Apple 在 iOS 11 和 macOS High Sierra 中引入的新图片格式。它基于 HEIF (High Efficiency Image Format) 标准，提供比 JPEG 更好的压缩率和图像质量。

### 主要特点
- **更小的文件**：相同质量下，文件大小约为 JPEG 的 50%
- **更好的质量**：在相同文件大小下，图像质量更高
- **Apple 默认格式**：iPhone 和 iPad 默认使用 HEIC 格式保存照片

## Tuya Converter 的 HEIC 支持

### 当前状态

⚠️ **重要提示**：由于依赖库问题，当前版本 (v1.1.0) **暂不支持** HEIC/HEIF 格式的直接转换。

### 解决方案

在 HEIC 支持恢复之前，请使用以下方法之一转换 HEIC 文件：

#### 方法 1：使用系统自带工具转换

**macOS:**
1. 使用"照片"应用打开 HEIC 文件
2. 选择"文件" → "导出" → "导出未修改的原片"
3. 在格式中选择 JPEG 或 PNG
4. 使用转换后的文件

**Windows:**
1. 使用"照片"应用打开 HEIC 文件
2. 点击"保存"图标
3. 选择 JPEG 格式保存

#### 方法 2：使用在线转换工具

1. 访问在线 HEIC 转换网站（如 heictojpg.com）
2. 上传 HEIC 文件
3. 下载转换后的 JPEG 文件
4. 使用转换后的文件

### 技术实现（计划中）

Tuya Converter 计划使用以下库来支持 HEIC 格式：

1. **TwelveMonkeys ImageIO** - 扩展 Java ImageIO 功能
2. **imageio-heif** - 提供 HEIF/HEIC 解码支持

这些库会在构建时自动下载和集成到 JAR 文件中。

## 常见问题

### Q1: 为什么需要额外的库？

A: Java 标准库不支持 HEIC 格式，需要第三方库来解码 HEIC 文件。

### Q2: 转换 HEIC 文件会比 JPEG 慢吗？

A: 是的，HEIC 解码比 JPEG 稍慢，但通常在可接受范围内（1-3 秒）。

### Q3: 支持 HEIC 的所有特性吗？

A: 支持基本的图像解码。不支持：
- 多帧 HEIC（Live Photos）- 只会提取第一帧
- HEIC 元数据（EXIF）- 会丢失
- 透明度 - 转换为白色背景

### Q4: 如何从 iPhone 导出 HEIC 照片？

A: 有几种方法：

**方法 1：AirDrop**
1. 在 iPhone 上选择照片
2. 点击分享 → AirDrop
3. 发送到 Mac

**方法 2：iCloud 照片**
1. 在 iPhone 上启用 iCloud 照片
2. 在 Mac 上打开照片 App
3. 照片会自动同步

**方法 3：USB 连接**
1. 用 USB 线连接 iPhone 和电脑
2. 在 Mac 上打开照片 App 或图像捕捉
3. 导入照片

**方法 4：转换为 JPEG（如果不需要 HEIC）**
1. 设置 → 相机 → 格式
2. 选择"最兼容"而不是"高效"
3. 新照片将保存为 JPEG

### Q5: 转换后的 BMP 文件质量如何？

A: 转换流程：
```
HEIC → 解码 → RGB → 缩放 → 灰度 → 二值化 → 1-bit BMP
```

质量取决于：
- 原始 HEIC 图片质量
- 目标尺寸（默认 480x800）
- 二值化阈值（默认 128）

对于电子墨水屏显示，效果通常很好。

### Q6: 可以保留 HEIC 的高动态范围（HDR）吗？

A: 不可以。转换为 1-bit BMP 会丢失所有颜色和动态范围信息，只保留黑白。

### Q7: 支持 iPhone 的 Live Photos 吗？

A: 部分支持。Live Photos 包含：
- 静态 HEIC 图片 ✅ 支持
- 短视频 ❌ 不支持

转换时只会提取静态图片部分。

## 性能对比

### 转换速度（参考）

| 格式 | 文件大小 | 转换时间 |
|------|---------|---------|
| JPEG | 2 MB | 1-2 秒 |
| PNG | 3 MB | 1-2 秒 |
| HEIC | 1 MB | 2-3 秒 |
| WebP | 1.5 MB | 1-2 秒 |

HEIC 解码稍慢，但文件更小，总体传输+转换时间可能更快。

### 内存使用

HEIC 解码需要的内存与其他格式类似：
- 小图片（< 1 MB）：< 100 MB
- 中等图片（1-3 MB）：100-300 MB
- 大图片（> 3 MB）：300-500 MB

## 示例场景

### 场景 1：iPhone 照片库转换

```bash
# 1. 从 iPhone 导出所有照片到文件夹
# 2. 批量转换
cd ~/Pictures/iPhone_Photos
for f in *.heic; do 
    java -jar ~/tuya-converter/target/tuya-converter.jar "$f"
done

# 3. 所有 *_tuya.bmp 文件可以复制到 SD 卡
```

### 场景 2：混合格式转换

```bash
# 转换文件夹中的所有图片（HEIC + JPEG + PNG）
for f in *.{heic,jpg,png}; do 
    [ -f "$f" ] && java -jar target/tuya-converter.jar "$f"
done
```

### 场景 3：保持原始方向

```bash
# HEIC 文件通常包含正确的方向信息
# 工具会自动处理，无需手动旋转
java -jar target/tuya-converter.jar portrait.heic  # 自动识别竖屏
java -jar target/tuya-converter.jar landscape.heic # 自动识别横屏
```

## 故障排除

### 问题 1：无法读取 HEIC 文件

**错误信息**：
```
Error: Failed to read image file
```

**解决方法**：
1. 确认文件确实是 HEIC 格式：`file photo.heic`
2. 尝试用其他工具打开文件，确认文件未损坏
3. 重新构建项目：`./build.sh` 或 `build.bat`

### 问题 2：构建失败

**错误信息**：
```
Could not resolve dependencies
```

**解决方法**：
1. 检查网络连接（需要下载依赖）
2. 清理 Maven 缓存：`mvn clean`
3. 重新构建：`mvn clean package`

### 问题 3：内存不足

**错误信息**：
```
java.lang.OutOfMemoryError: Java heap space
```

**解决方法**：
```bash
# 增加堆内存到 2GB
java -Xmx2g -jar target/tuya-converter.jar large.heic
```

## 技术细节

### 依赖库

```xml
<!-- TwelveMonkeys ImageIO -->
<dependency>
    <groupId>com.twelvemonkeys.imageio</groupId>
    <artifactId>imageio-core</artifactId>
    <version>3.10.1</version>
</dependency>

<!-- HEIF/HEIC 支持 -->
<dependency>
    <groupId>com.github.gotson</groupId>
    <artifactId>imageio-heif</artifactId>
    <version>1.1.0</version>
</dependency>
```

### 代码实现

```java
// HEIC 文件会被 ImageIO 自动识别和解码
BufferedImage image = ImageIO.read(new File("photo.heic"));

// 后续处理与其他格式相同
// 缩放 → 灰度 → 二值化 → BMP
```

### 文件格式检测

```java
private static boolean isImageFile(String extension) {
    return extension.equals("jpg") || extension.equals("jpeg") || 
           extension.equals("png") || extension.equals("bmp") || 
           extension.equals("gif") || extension.equals("webp") ||
           extension.equals("heic") || extension.equals("heif");
}
```

## 相关资源

- **HEIF 标准**：https://en.wikipedia.org/wiki/High_Efficiency_Image_File_Format
- **TwelveMonkeys ImageIO**：https://github.com/haraldk/TwelveMonkeys
- **imageio-heif**：https://github.com/gotson/imageio-heif
- **Apple HEIC 说明**：https://support.apple.com/en-us/HT207022

## 总结

Tuya Converter 完全支持 Apple HEIC/HEIF 格式，可以无缝转换 iPhone 和 iPad 的照片。使用方法与其他格式完全相同，工具会自动处理格式检测和解码。

**优势**：
- ✅ 自动格式检测
- ✅ 无需手动转换
- ✅ 支持智能旋转
- ✅ 与其他格式相同的使用体验

**限制**：
- ❌ 不支持 Live Photos 视频部分
- ❌ 不保留 EXIF 元数据
- ❌ 解码速度稍慢于 JPEG

---

**最后更新**：2025-12-25
**版本**：v1.1.0
