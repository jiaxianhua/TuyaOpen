# Tuya Converter - 使用总结

## ✅ 已完成的功能

### 核心功能
- ✅ **图片转换**：JPG/PNG/BMP/GIF/WebP → 1-bit BMP
- ✅ **文本转换**：任意编码 → GBK 编码
- ✅ **智能旋转**：自动旋转 90° 以显示更多内容
- ✅ **高质量缩放**：双三次插值
- ✅ **自动命名**：添加 `_tuya` 后缀
- ✅ **跨平台**：Windows/macOS/Linux

### 智能旋转说明

工具会自动计算两种方向的屏幕覆盖率：
- **不旋转**：原始方向适配屏幕
- **旋转 90°**：旋转后适配屏幕

如果旋转后覆盖率提升 > 5%，则自动旋转。

**示例**：
```
横屏照片 (1920x1080) 适配竖屏 (480x800)
→ 不旋转：覆盖率 60%（上下黑边）
→ 旋转 90°：覆盖率 95%（几乎全屏）
→ 结果：自动旋转 ✓
```

## 🚀 快速开始

### 1. 构建项目

**Windows:**
```cmd
build.bat
```

**Linux/macOS:**
```bash
chmod +x build.sh
./build.sh
```

### 2. 使用方式

#### 方式 1：命令行（推荐用于批量处理）

```bash
# 转换图片（默认 480x800）
java -jar target/tuya-converter.jar photo.jpg

# 转换图片（自定义尺寸）
java -jar target/tuya-converter.jar photo.jpg 800 480

# 转换文本
java -jar target/tuya-converter.jar novel.txt
```

#### 方式 2：拖拽文件（推荐用于单个文件）

**Windows:**
1. 拖文件到 `tuya-convert.bat` 上
2. 自动转换并显示结果

**Linux/macOS:**
1. 运行一次：`chmod +x tuya-convert.sh`
2. 拖文件到 `tuya-convert.sh` 上
3. 自动转换并显示结果

## 📝 使用示例

### 示例 1：转换照片

```bash
# 输入：vacation.jpg (1920x1080)
java -jar target/tuya-converter.jar vacation.jpg

# 输出：vacation_tuya.bmp (480x800, 1-bit)
# 自动旋转：是（横屏→竖屏）
```

### 示例 1.5：转换 iPhone 照片

```bash
# 输入：iphone_photo.heic (Apple HEIC 格式)
java -jar target/tuya-converter.jar iphone_photo.heic

# 输出：iphone_photo_tuya.bmp (480x800, 1-bit)
# 自动旋转：根据照片方向自动判断
```

### 示例 2：转换小说

```bash
# 输入：三体.txt (UTF-8 编码)
java -jar target/tuya-converter.jar 三体.txt

# 输出：三体_tuya.txt (GBK 编码)
# 可直接在电子墨水屏上显示中文
```

### 示例 3：批量转换

**Windows (PowerShell):**
```powershell
Get-ChildItem *.jpg | ForEach-Object { 
    java -jar target/tuya-converter.jar $_.FullName 
}
```

**Linux/macOS:**
```bash
for f in *.jpg; do 
    java -jar target/tuya-converter.jar "$f"
done
```

### 示例 4：横屏显示

```bash
# 适用于信息面板、仪表盘等
java -jar target/tuya-converter.jar dashboard.png 800 480
```

## 📂 文件结构

```
tuya-converter/
├── pom.xml                          # Maven 配置
├── build.sh                         # Linux/macOS 构建脚本
├── build.bat                        # Windows 构建脚本
├── tuya-convert.sh                  # Linux/macOS 拖拽助手
├── tuya-convert.bat                 # Windows 拖拽助手
├── README.md                        # 完整文档
├── QUICKSTART.md                    # 快速开始
├── FEATURES.md                      # 功能特性
├── USAGE_SUMMARY.md                 # 本文件
├── .gitignore                       # Git 忽略规则
└── src/main/java/com/tuya/converter/
    └── TuyaConverter.java           # 主程序（单文件）
```

## 🔧 技术细节

### 图片转换流程

```
输入图片 (JPG/PNG/...)
    ↓
读取图片
    ↓
智能旋转判断（计算覆盖率）
    ↓
调整大小（双三次插值）
    ↓
转为灰度图
    ↓
二值化（阈值 128）
    ↓
生成 1-bit BMP
    ↓
输出文件 (*_tuya.bmp)
```

### 文本转换流程

```
输入文本 (UTF-8/GBK/...)
    ↓
自动检测编码
    ↓
读取内容
    ↓
转换为 GBK 编码
    ↓
输出文件 (*_tuya.txt)
```

### BMP 格式规范

- **位深度**：1-bit（单色）
- **调色板**：黑色 (0,0,0) + 白色 (255,255,255)
- **存储方式**：底部向上（标准 BMP）
- **行对齐**：4 字节边界
- **文件大小**：480x800 ≈ 48 KB

## ❓ 常见问题

### Q1: 为什么图片会自动旋转？

A: 工具会自动选择显示更多内容的方向。如果横屏照片在竖屏上显示会有大量黑边，工具会自动旋转 90° 以减少黑边。

### Q2: 如何禁用自动旋转？

A: 目前不支持禁用。如果需要特定方向，可以先手动旋转图片，或者修改源代码中的 `shouldRotateImage` 方法。

### Q3: 文本显示乱码怎么办？

A: 确保原始文件是 UTF-8 或 GBK 编码。如果是其他编码，请先用文本编辑器转换为 UTF-8。

### Q4: 图片太大导致内存不足？

A: 增加 Java 堆内存：
```bash
java -Xmx2g -jar target/tuya-converter.jar large-image.jpg
```

### Q5: 如何批量转换文件夹中的所有图片？

A: 参考上面的"示例 3：批量转换"。

### Q6: 输出的 BMP 文件为什么这么小？

A: 1-bit BMP 只存储黑白信息，每个像素只占 1 bit，所以文件非常小（480x800 ≈ 48 KB）。

### Q7: 支持透明背景吗？

A: 不支持。透明部分会转换为白色背景。

### Q8: 可以输出 PNG 格式吗？

A: 目前只支持 BMP 格式，因为电子墨水屏通常使用 BMP。

## 📚 相关文档

- **完整文档**：[README.md](README.md)
- **快速开始**：[QUICKSTART.md](QUICKSTART.md)
- **功能特性**：[FEATURES.md](FEATURES.md)
- **项目主页**：[../../README.md](../../README.md)

## 🎯 下一步

1. **测试工具**：转换几张图片和文本文件
2. **验证输出**：检查生成的 BMP 和 TXT 文件
3. **部署到设备**：将文件复制到 SD 卡
4. **查看效果**：在电子墨水屏上显示

## 💡 提示

- 图片建议使用高对比度的照片，效果更好
- 文本文件建议使用 UTF-8 编码保存
- 批量转换时建议使用命令行方式
- 单个文件转换时建议使用拖拽方式

## 🐛 问题反馈

如果遇到问题：
1. 查看 [README.md](README.md) 的故障排除部分
2. 检查 Java 版本（需要 11+）
3. 检查 Maven 版本（需要 3.6+）
4. 提交 Issue 到 GitHub

## 📄 许可证

MIT License - 自由使用、修改、分发

---

**最后更新**：2025-12-25
**版本**：v1.0.0
