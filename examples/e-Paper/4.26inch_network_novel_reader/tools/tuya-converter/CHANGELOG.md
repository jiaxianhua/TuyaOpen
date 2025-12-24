# Tuya Converter - 更新日志

## v1.1.0 (2025-12-25)

### 🎉 新功能

#### Apple HEIC/HEIF 格式支持
- ✅ 支持 `.heic` 和 `.heif` 文件格式
- ✅ 可直接转换 iPhone 和 iPad 的照片
- ✅ 自动解码 HEIC 格式
- ✅ 与其他格式相同的使用体验

### 📚 文档更新
- ✅ 新增 `HEIC_SUPPORT.md` - HEIC 格式详细说明
- ✅ 更新 `README.md` - 添加 HEIC 格式说明
- ✅ 更新 `FEATURES.md` - 添加 HEIC 到支持列表
- ✅ 更新 `QUICKSTART.md` - 添加 HEIC 示例
- ✅ 更新 `USAGE_SUMMARY.md` - 添加 iPhone 照片转换示例
- ✅ 更新 `PROJECT_STATUS.md` - 更新功能列表

### 🔧 技术改进
- ✅ 添加 TwelveMonkeys ImageIO 依赖
- ✅ 添加 imageio-heif 依赖
- ✅ 更新版本号到 1.1.0
- ✅ 扩展文件格式检测

### 📦 依赖更新

新增依赖：
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

### 🎯 使用示例

```bash
# 转换 iPhone HEIC 照片
java -jar target/tuya-converter.jar iphone_photo.heic

# 批量转换 HEIC 文件
for f in *.heic; do 
    java -jar target/tuya-converter.jar "$f"
done

# 拖拽 HEIC 文件到 tuya-convert.sh/bat
```

### 📝 支持的格式（更新后）

**图片格式**：
- JPG/JPEG ✅
- PNG ✅
- BMP ✅
- GIF ✅
- WebP ✅
- **HEIC/HEIF ✅ (新增)**

**文本格式**：
- TXT ✅

### ⚠️ 已知限制

HEIC 格式的限制：
- ❌ 不支持 Live Photos 视频部分（只提取静态图片）
- ❌ 不保留 EXIF 元数据
- ❌ 解码速度稍慢于 JPEG（约 2-3 秒）
- ❌ 不支持透明度（转换为白色背景）

### 🔄 迁移指南

从 v1.0.0 升级到 v1.1.0：

1. **重新构建项目**：
   ```bash
   ./build.sh  # Linux/macOS
   build.bat   # Windows
   ```

2. **无需修改使用方式**：
   - HEIC 文件使用方法与其他格式完全相同
   - 拖拽、命令行、批量处理都支持

3. **新功能自动启用**：
   - 构建后自动支持 HEIC 格式
   - 无需额外配置

### 📖 相关文档

- [HEIC_SUPPORT.md](HEIC_SUPPORT.md) - HEIC 格式详细说明
- [README.md](README.md) - 完整使用文档
- [QUICKSTART.md](QUICKSTART.md) - 快速开始指南

---

## v1.0.0 (2025-12-25)

### 🎉 初始版本发布

#### 核心功能
- ✅ 图片转换为 1-bit BMP
- ✅ 文本转换为 GBK 编码
- ✅ 智能旋转算法
- ✅ 高质量缩放（双三次插值）
- ✅ 自动文件命名（添加 _tuya 后缀）

#### 支持的格式
- JPG/JPEG
- PNG
- BMP
- GIF
- WebP
- TXT

#### 跨平台支持
- ✅ Windows（命令行 + 拖拽）
- ✅ macOS（命令行 + 拖拽）
- ✅ Linux（命令行 + 拖拽）

#### 文档
- ✅ README.md - 完整文档
- ✅ QUICKSTART.md - 快速开始
- ✅ FEATURES.md - 功能特性
- ✅ USAGE_SUMMARY.md - 使用总结
- ✅ PROJECT_STATUS.md - 项目状态

#### 构建工具
- ✅ build.sh - Linux/macOS 构建脚本
- ✅ build.bat - Windows 构建脚本
- ✅ tuya-convert.sh - Linux/macOS 拖拽助手
- ✅ tuya-convert.bat - Windows 拖拽助手

#### 技术实现
- ✅ 纯 Java 实现（Java 11+）
- ✅ Maven 构建系统
- ✅ 单个 JAR 文件
- ✅ 无外部依赖（v1.0.0）

---

## 版本规划

### v1.2.0 (计划中)
- [ ] GUI 界面
- [ ] 批量转换模式
- [ ] 图片预览
- [ ] 自定义阈值

### v1.3.0 (计划中)
- [ ] 抖动算法选项
- [ ] PNG 输出支持
- [ ] 更多文本编码
- [ ] 配置文件支持

### v2.0.0 (长期计划)
- [ ] 多线程处理
- [ ] 内存优化
- [ ] 插件系统
- [ ] Web 界面

---

**维护者**：TuyaOpen Team  
**许可证**：MIT License  
**项目主页**：https://github.com/tuya/tuyaopen
