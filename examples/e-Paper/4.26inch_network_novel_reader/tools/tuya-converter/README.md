# Tuya E-Paper Converter

跨平台工具，用于将图片转换为 1-bit BMP 格式，文本转换为 GBK 编码，适用于 Tuya 电子墨水屏。

Cross-platform tool to convert images to 1-bit BMP and text files to GBK encoding for Tuya E-Paper displays.

## 功能 / Features

- ✅ **智能旋转**：自动旋转图片 90° 以显示更多内容
- ✅ 图片转换为 1-bit 黑白 BMP（480x800 默认）
- ✅ 文本转换为 GBK 编码
- ✅ 支持拖拽文件（Windows/macOS/Linux）
- ✅ 自动添加 `_tuya` 后缀
- ✅ 跨平台（Windows/macOS/Linux）

### 智能旋转说明

工具会自动分析图片的宽高比，如果旋转 90° 后能显示更多内容（减少黑边），则自动旋转。

**示例**：
- 横屏照片 (1920x1080) → 自动旋转为竖屏以适配 480x800 屏幕
- 竖屏照片 (1080x1920) → 保持竖屏
- 方形照片 (1000x1000) → 保持不变

**覆盖率计算**：
- 不旋转：覆盖率 60% → 40% 黑边
- 旋转 90°：覆盖率 95% → 5% 黑边
- **结果**：自动选择旋转（显示更多内容）

## 系统要求 / Requirements

- Java 11 或更高版本
- Maven 3.6+ (仅构建时需要)

## 构建 / Build

### Linux/macOS

```bash
chmod +x build.sh
./build.sh
```

### Windows

```cmd
build.bat
```

构建完成后，JAR 文件位于 `target/tuya-converter.jar`

## 使用方法 / Usage

### 命令行 / Command Line

```bash
# 转换图片（默认 480x800）
java -jar tuya-converter.jar photo.jpg

# 转换图片（自定义尺寸）
java -jar tuya-converter.jar photo.jpg 480 800

# 转换文本
java -jar tuya-converter.jar novel.txt
```

### 拖拽文件 / Drag and Drop

#### Windows

1. 创建快捷方式脚本 `tuya-convert.bat`：

```batch
@echo off
java -jar "C:\path\to\tuya-converter.jar" %*
pause
```

2. 将文件拖到 `tuya-convert.bat` 上

#### macOS

1. 创建应用程序：
   - 打开 "自动操作" (Automator)
   - 选择 "应用程序"
   - 添加 "运行 Shell 脚本"
   - 输入：
   ```bash
   for f in "$@"
   do
       java -jar /path/to/tuya-converter.jar "$f"
   done
   ```
   - 保存为 "Tuya Converter.app"

2. 将文件拖到应用图标上

#### Linux

1. 创建脚本 `tuya-convert.sh`：

```bash
#!/bin/bash
for file in "$@"; do
    java -jar /path/to/tuya-converter.jar "$file"
done
```

2. 设置可执行权限：
```bash
chmod +x tuya-convert.sh
```

3. 将文件拖到脚本上

## 支持的格式 / Supported Formats

### 图片 / Images
- JPG/JPEG
- PNG
- BMP
- GIF
- WebP

### 文本 / Text
- TXT (自动检测编码：UTF-8, GBK, 系统默认)

## 输出示例 / Output Examples

```
输入 / Input          输出 / Output
-----------------    ---------------------
photo.jpg         -> photo_tuya.bmp (1-bit, 480x800)
image.png         -> image_tuya.bmp (1-bit, 480x800)
novel.txt         -> novel_tuya.txt (GBK encoding)
```

## 技术细节 / Technical Details

### 图片转换 / Image Conversion

1. **调整大小**：使用双三次插值缩放到目标尺寸
2. **灰度化**：转换为灰度图像
3. **二值化**：阈值 128，大于 128 为白色，小于等于 128 为黑色
4. **BMP 格式**：
   - 1-bit 单色 BMP
   - 底部向上存储（标准 BMP 格式）
   - 包含黑白调色板
   - 行对齐到 4 字节边界

### 文本转换 / Text Conversion

1. **编码检测**：自动检测 UTF-8、GBK 或系统默认编码
2. **输出编码**：GBK（适用于中文显示）

## 示例 / Examples

### 转换照片

```bash
# 默认尺寸 (480x800)
java -jar tuya-converter.jar vacation.jpg
# 输出: vacation_tuya.bmp

# 自定义尺寸
java -jar tuya-converter.jar landscape.jpg 800 480
# 输出: landscape_tuya.bmp (横屏)
```

### 转换小说

```bash
java -jar tuya-converter.jar 三体.txt
# 输出: 三体_tuya.txt (GBK 编码)
```

### 批量转换

#### Linux/macOS

```bash
for file in *.jpg; do
    java -jar tuya-converter.jar "$file"
done
```

#### Windows

```batch
for %%f in (*.jpg) do (
    java -jar tuya-converter.jar "%%f"
)
```

## 故障排除 / Troubleshooting

### Java 未安装

```bash
# 检查 Java 版本
java -version

# 如果未安装，下载：
# https://adoptium.net/
```

### Maven 未安装（仅构建时）

```bash
# 检查 Maven 版本
mvn -version

# 如果未安装，下载：
# https://maven.apache.org/install.html
```

### 内存不足（大图片）

```bash
# 增加 Java 堆内存
java -Xmx2g -jar tuya-converter.jar large-image.jpg
```

### 文件编码问题

如果文本文件显示乱码：
1. 确保原始文件是 UTF-8 或 GBK 编码
2. 使用文本编辑器另存为 UTF-8
3. 重新转换

## 开发 / Development

### 项目结构

```
tuya-converter/
├── pom.xml                          # Maven 配置
├── build.sh                         # Linux/macOS 构建脚本
├── build.bat                        # Windows 构建脚本
├── README.md                        # 本文件
└── src/main/java/com/tuya/converter/
    └── TuyaConverter.java           # 主程序
```

### 编译

```bash
mvn clean compile
```

### 运行（开发模式）

```bash
mvn exec:java -Dexec.mainClass="com.tuya.converter.TuyaConverter" -Dexec.args="test.jpg"
```

### 打包

```bash
mvn clean package
```

## 许可证 / License

MIT License

## 贡献 / Contributing

欢迎提交 Issue 和 Pull Request！

## 更新日志 / Changelog

### v1.0.0 (2025-12-25)
- ✨ 初始版本
- ✅ 图片转 1-bit BMP
- ✅ 文本转 GBK
- ✅ 跨平台支持
- ✅ 拖拽文件支持

## 相关链接 / Links

- [TuyaOpen GitHub](https://github.com/tuya/tuyaopen)
- [E-Paper 示例](../../README.md)
- [图片转换 Python 版本](../convert_image_to_bmp.py)
