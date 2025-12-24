# Tuya Converter v1.1.0 更新说明

## 🎉 新功能：Apple HEIC/HEIF 格式支持

现在可以直接转换 iPhone 和 iPad 的照片了！

### ✨ 主要更新

1. **支持 HEIC/HEIF 格式**
   - 直接转换 iPhone/iPad 照片
   - 无需先转换为 JPEG
   - 使用方法与其他格式完全相同

2. **自动格式检测**
   - 工具自动识别 HEIC 文件
   - 自动解码和转换
   - 无需额外配置

3. **完整文档**
   - 新增 HEIC_SUPPORT.md 详细说明
   - 更新所有相关文档
   - 添加使用示例

## 🚀 快速开始

### 1. 重新构建（必需）

```bash
# Linux/macOS
./build.sh

# Windows
build.bat
```

### 2. 转换 HEIC 文件

```bash
# 单个文件
java -jar target/tuya-converter.jar photo.heic

# 批量转换
for f in *.heic; do 
    java -jar target/tuya-converter.jar "$f"
done

# 拖拽文件
# 直接拖到 tuya-convert.sh/bat 上
```

## 📝 支持的格式

**图片**：
- JPG/JPEG ✅
- PNG ✅
- BMP ✅
- GIF ✅
- WebP ✅
- **HEIC/HEIF ✅ (新增)**

**文本**：
- TXT ✅

## 💡 使用示例

### 示例 1：转换 iPhone 照片

```bash
# 从 iPhone 导出照片后
java -jar target/tuya-converter.jar IMG_1234.heic
# 输出: IMG_1234_tuya.bmp
```

### 示例 2：批量转换相册

```bash
# 转换整个文件夹
cd ~/Pictures/iPhone_Photos
for f in *.heic; do 
    java -jar ~/tuya-converter/target/tuya-converter.jar "$f"
done
```

### 示例 3：混合格式

```bash
# 同时转换 HEIC 和 JPEG
for f in *.{heic,jpg}; do 
    [ -f "$f" ] && java -jar target/tuya-converter.jar "$f"
done
```

## ⚠️ 注意事项

### HEIC 格式限制

1. **Live Photos**
   - 只提取静态图片部分
   - 不支持视频部分

2. **元数据**
   - EXIF 信息会丢失
   - 位置、日期等信息不保留

3. **性能**
   - 解码稍慢于 JPEG（2-3 秒）
   - 但文件更小，总体时间可能更快

4. **透明度**
   - 不支持透明背景
   - 转换为白色背景

## 🔧 技术细节

### 新增依赖

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

### 代码更新

```java
// 扩展支持的格式
private static boolean isImageFile(String extension) {
    return extension.equals("jpg") || extension.equals("jpeg") || 
           extension.equals("png") || extension.equals("bmp") || 
           extension.equals("gif") || extension.equals("webp") ||
           extension.equals("heic") || extension.equals("heif");  // 新增
}
```

## 📚 相关文档

- **HEIC 详细说明**：[HEIC_SUPPORT.md](HEIC_SUPPORT.md)
- **完整文档**：[README.md](README.md)
- **快速开始**：[QUICKSTART.md](QUICKSTART.md)
- **更新日志**：[CHANGELOG.md](CHANGELOG.md)

## ❓ 常见问题

### Q: 需要重新构建吗？
A: 是的，必须重新构建以包含新的依赖库。

### Q: 旧版本的 JAR 能用吗？
A: 不能，旧版本不支持 HEIC 格式。

### Q: 如何从 iPhone 导出 HEIC 照片？
A: 使用 AirDrop、iCloud 照片、USB 连接或照片 App。详见 [HEIC_SUPPORT.md](HEIC_SUPPORT.md)。

### Q: 转换速度如何？
A: HEIC 解码稍慢（2-3 秒），但文件更小，总体时间可能更快。

### Q: 支持 Live Photos 吗？
A: 部分支持，只提取静态图片，不包含视频。

## 🎯 下一步

1. **重新构建项目**
   ```bash
   ./build.sh  # 或 build.bat
   ```

2. **测试 HEIC 转换**
   ```bash
   java -jar target/tuya-converter.jar test.heic
   ```

3. **批量转换照片**
   ```bash
   for f in *.heic; do 
       java -jar target/tuya-converter.jar "$f"
   done
   ```

4. **复制到 SD 卡**
   ```bash
   cp *_tuya.bmp /path/to/sdcard/
   ```

## 🙏 反馈

如有问题或建议，请：
- 查看 [HEIC_SUPPORT.md](HEIC_SUPPORT.md) 故障排除部分
- 提交 GitHub Issue
- 联系 TuyaOpen 社区

---

**版本**：v1.1.0  
**发布日期**：2025-12-25  
**维护者**：TuyaOpen Team  
**许可证**：MIT License
