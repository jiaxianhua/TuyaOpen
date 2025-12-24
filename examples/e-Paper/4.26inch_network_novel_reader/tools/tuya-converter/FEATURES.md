# Tuya Converter 功能特性

## 核心功能

### 1. 图片转换

#### 支持的输入格式
- ✅ JPG/JPEG
- ✅ PNG
- ✅ BMP
- ✅ GIF
- ✅ WebP
- ✅ HEIC/HEIF (Apple)

#### 转换流程
1. **读取图片** - 使用 Java ImageIO
2. **调整大小** - 双三次插值，高质量缩放
3. **灰度化** - 转换为灰度图像
4. **二值化** - 阈值 128，转为黑白
5. **生成 BMP** - 1-bit 单色 BMP 格式

#### 输出特性
- **格式**：1-bit 单色 BMP
- **默认尺寸**：480x800（竖屏）
- **可自定义**：支持任意尺寸
- **调色板**：黑白两色
- **存储方式**：底部向上（标准 BMP）
- **行对齐**：4 字节边界

### 2. 文本转换

#### 编码检测
- 自动检测 UTF-8
- 自动检测 GBK
- 回退到系统默认编码

#### 输出编码
- **固定输出**：GBK 编码
- **用途**：适用于中文电子墨水屏显示
- **兼容性**：与 HZK24 字体完美配合

### 3. 文件命名

#### 自动命名规则
```
输入文件              输出文件
-----------------    ---------------------
photo.jpg         -> photo_tuya.bmp
image.png         -> image_tuya.bmp
landscape.webp    -> landscape_tuya.bmp
iphone.heic       -> iphone_tuya.bmp
novel.txt         -> novel_tuya.txt
三体.txt          -> 三体_tuya.txt
```

#### 命名特点
- 保留原文件名
- 添加 `_tuya` 后缀
- 自动更改扩展名
- 支持中文文件名
- 不覆盖原文件

## 跨平台支持

### Windows
- ✅ 命令行运行
- ✅ 拖拽文件到 .bat
- ✅ PowerShell 批量处理
- ✅ 双击运行

### macOS
- ✅ 命令行运行
- ✅ 拖拽文件到 .sh
- ✅ 创建 Automator 应用
- ✅ Bash 批量处理

### Linux
- ✅ 命令行运行
- ✅ 拖拽文件到 .sh
- ✅ Bash 批量处理
- ✅ 桌面集成

## 使用场景

### 场景 1：电子相册
```bash
# 转换家庭照片
java -jar tuya-converter.jar family-2024.jpg

# 批量转换相册
for f in vacation/*.jpg; do
    java -jar tuya-converter.jar "$f"
done
```

### 场景 2：电子书阅读
```bash
# 转换小说
java -jar tuya-converter.jar 三体.txt

# 转换多本书
java -jar tuya-converter.jar book1.txt book2.txt book3.txt
```

### 场景 3：信息展示
```bash
# 横屏显示（适合信息面板）
java -jar tuya-converter.jar dashboard.png 800 480

# 竖屏显示（适合阅读）
java -jar tuya-converter.jar poster.jpg 480 800
```

### 场景 4：艺术展示
```bash
# 转换艺术作品
java -jar tuya-converter.jar artwork.png

# 黑白效果预览
java -jar tuya-converter.jar sketch.jpg
```

## 技术优势

### 1. 无需外部依赖
- 纯 Java 实现
- 不依赖 ImageMagick
- 不依赖 Python
- 单个 JAR 文件即可运行

### 2. 高质量转换
- 双三次插值缩放
- 抗锯齿处理
- 智能阈值二值化
- 保持图像细节

### 3. 智能编码处理
- 自动检测文本编码
- 无损转换为 GBK
- 支持中英文混排
- 避免乱码问题

### 4. 用户友好
- 简单的命令行界面
- 支持拖拽操作
- 清晰的错误提示
- 详细的使用文档

## 性能特点

### 内存使用
- 小图片：< 50 MB
- 中等图片：50-200 MB
- 大图片：200-500 MB
- 可通过 `-Xmx` 调整

### 处理速度
- 小图片（< 1 MB）：< 1 秒
- 中等图片（1-5 MB）：1-3 秒
- 大图片（> 5 MB）：3-10 秒
- 文本文件：< 1 秒

### 文件大小
- 输入 JPG（1 MB）→ 输出 BMP（~50 KB）
- 输入 PNG（2 MB）→ 输出 BMP（~50 KB）
- 1-bit BMP 非常小巧

## 限制与注意事项

### 图片限制
- 最大尺寸：受 Java 堆内存限制
- 推荐尺寸：< 4000x4000
- 超大图片：需增加堆内存

### 文本限制
- 文件大小：无限制
- 编码支持：UTF-8, GBK, 系统默认
- 特殊字符：可能需要 GBK 支持

### 格式限制
- 仅支持静态图片（GIF 只取第一帧）
- 不支持透明度（转为白色背景）
- 不支持动画

## 未来计划

### 计划功能
- [ ] GUI 界面
- [ ] 批量转换模式
- [ ] 图片预览
- [ ] 自定义阈值
- [ ] 抖动算法选项
- [ ] PNG 输出支持
- [ ] 更多文本编码

### 优化计划
- [ ] 多线程处理
- [ ] 内存优化
- [ ] 更快的转换速度
- [ ] 更好的错误处理

## 对比其他工具

### vs ImageMagick
- ✅ 无需安装外部工具
- ✅ 跨平台单文件
- ✅ 更简单的使用
- ❌ 功能较少

### vs Python 脚本
- ✅ 无需 Python 环境
- ✅ 无需安装依赖
- ✅ 更快的启动速度
- ❌ 代码较长

### vs 在线工具
- ✅ 离线使用
- ✅ 隐私保护
- ✅ 批量处理
- ✅ 自定义尺寸

## 贡献指南

欢迎贡献代码！

### 如何贡献
1. Fork 项目
2. 创建功能分支
3. 提交代码
4. 发起 Pull Request

### 代码规范
- Java 11+ 语法
- 遵循 Google Java Style
- 添加注释
- 编写测试

## 许可证

MIT License - 自由使用、修改、分发

## 联系方式

- GitHub Issues
- TuyaOpen 社区
- 技术支持邮箱
