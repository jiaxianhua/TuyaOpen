# Tuya Converter 快速开始

## 5 分钟上手指南

### 1. 安装 Java

检查是否已安装：
```bash
java -version
```

如果未安装，下载 Java 11+：
- https://adoptium.net/

### 2. 构建项目

#### Windows
```cmd
build.bat
```

#### Linux/macOS
```bash
chmod +x build.sh
./build.sh
```

### 3. 使用

#### 方法 1：命令行

```bash
# 转换图片
java -jar target/tuya-converter.jar photo.jpg

# 转换文本
java -jar target/tuya-converter.jar novel.txt
```

#### 方法 2：拖拽文件（推荐）

**Windows:**
1. 双击 `tuya-convert.bat`（或拖文件到上面）

**Linux/macOS:**
1. 运行：`chmod +x tuya-convert.sh`
2. 拖文件到 `tuya-convert.sh` 上

### 4. 查看结果

```
输入文件              输出文件
-----------------    ---------------------
photo.jpg         -> photo_tuya.bmp
novel.txt         -> novel_tuya.txt
```

## 常见问题

### Q: 图片太大/太小？

A: 指定尺寸：
```bash
java -jar target/tuya-converter.jar photo.jpg 480 800
```

### Q: 文本显示乱码？

A: 确保原文件是 UTF-8 或 GBK 编码

### Q: 找不到 Java？

A: 安装 Java 后重启终端/命令提示符

## 下一步

- 阅读完整文档：[README.md](README.md)
- 批量转换：参考 README 中的批量转换示例
- 自定义尺寸：根据你的屏幕调整宽高参数

## 示例

### 转换照片用于电子相册

```bash
# 竖屏 (480x800)
java -jar target/tuya-converter.jar family.jpg

# 横屏 (800x480)
java -jar target/tuya-converter.jar landscape.jpg 800 480
```

### 转换小说

```bash
java -jar target/tuya-converter.jar 三体.txt
# 输出 GBK 编码，可直接在电子墨水屏上显示中文
```

### 批量转换文件夹中的所有图片

**Windows (PowerShell):**
```powershell
Get-ChildItem *.jpg | ForEach-Object { java -jar target/tuya-converter.jar $_.FullName }
```

**Linux/macOS:**
```bash
for f in *.jpg; do java -jar target/tuya-converter.jar "$f"; done
```

## 技术支持

遇到问题？
1. 查看 [README.md](README.md) 的故障排除部分
2. 提交 Issue 到 GitHub
3. 查看 [TuyaOpen 文档](https://github.com/tuya/tuyaopen)
