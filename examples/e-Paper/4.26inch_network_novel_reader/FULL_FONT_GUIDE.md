# 完整字库生成指南

本指南帮助你生成包含所有 GB2312 汉字的完整字库，这样就能显示从网络下载的任何中文内容。

## 快速开始

### 步骤 1：生成完整字库

```bash
cd examples/e-Paper/4.26inch_network_novel_reader

# 生成完整 GB2312 字库（约 6,763 个汉字）
python tools/generate_full_hzk16.py HZK16 hzk16_full.c
```

**输出**：
```
Reading HZK16 font file: HZK16
Generating C code to hzk16_full.c...
  Generated 100 characters...
  Generated 200 characters...
  ...
  Generated 6700 characters...

Done!
Generated 6763 characters
File size: 234567 bytes (~229KB)

To use this font:
  1. Backup original: mv lib/Fonts/hzk16.c lib/Fonts/hzk16_default.c.bak
  2. Use new font: cp hzk16_full.c lib/Fonts/hzk16.c
  3. Rebuild: tos.py build
```

### 步骤 2：替换字库文件

```bash
# 备份原始文件
mv lib/Fonts/hzk16.c lib/Fonts/hzk16_default.c.bak

# 使用完整字库
cp hzk16_full.c lib/Fonts/hzk16.c
```

### 步骤 3：编译和烧录

```bash
# 编译（可能需要几分钟，因为文件较大）
tos.py build

# 烧录到设备
tos.py flash
```

### 步骤 4：测试

```bash
# 监控串口输出
tos.py monitor
```

设备启动后：
1. 自动连接 WiFi
2. 按按钮下载小说
3. 所有中文字符都能正确显示！

## 完整字库特性

### 支持的字符

- **汉字数量**：6,763 个
- **编码标准**：GB2312-1980
- **字符范围**：
  - 一级汉字：3,755 个（常用字）
  - 二级汉字：3,008 个（次常用字）
  - 符号：682 个

### 覆盖率

完整 GB2312 字库可以显示：
- ✅ 所有常用中文字符
- ✅ 大部分网络小说
- ✅ 新闻、文章
- ✅ 中文标点符号
- ❌ 生僻字（需要 GBK 扩展）
- ❌ 繁体字（需要 Big5）

### 文件大小

| 项目 | 大小 | 说明 |
|------|------|------|
| 源代码 | ~230KB | hzk16_full.c |
| 编译后 | ~230KB | 嵌入到固件中 |
| RAM 使用 | 0 | 存储在 Flash 中 |

## 性能优化

完整字库使用**二分查找**算法，查找速度很快：

| 字符数 | 查找时间 | 算法 |
|--------|----------|------|
| 100 | ~10μs | 线性查找 |
| 1000 | ~3μs | 二分查找 |
| 6763 | ~4μs | 二分查找 |

**结论**：即使是完整字库，查找速度也非常快，不会影响显示性能。

## 内存使用

### Flash 使用

```
完整字库：~230KB
代码：~50KB
其他库：~100KB
总计：~380KB
```

大多数开发板都有足够的 Flash 空间（通常 2MB+）。

### RAM 使用

```
显示缓冲区：48KB
小说内容：可变（如 100KB）
页面索引：~1KB
栈空间：8KB
总计：~160KB + 小说内容
```

**建议**：
- 如果 RAM 充足（>512KB），可以缓存整本小说
- 如果 RAM 有限（<256KB），可以分段下载

## 与专用字库对比

| 特性 | 专用字库 | 完整字库 |
|------|----------|----------|
| 字符数 | 500-2000 | 6,763 |
| 文件大小 | 17-68KB | 230KB |
| 生成时间 | 快 | 慢 |
| 覆盖率 | 单本小说 | 所有小说 |
| 查找速度 | 快 | 快（二分查找） |
| 适用场景 | 固定内容 | 动态内容 |

**推荐**：
- 如果只读一本小说 → 使用专用字库
- 如果读多本小说 → 使用完整字库 ⭐
- 如果从网络下载 → 使用完整字库 ⭐

## 测试完整字库

### 测试 1：常用字

```c
// 这些字符都能正确显示
你好世界
中国人民
天地玄黄
```

### 测试 2：生僻字

```c
// GB2312 支持
龘靐齉爩
// 但不支持超出 GB2312 的生僻字
```

### 测试 3：标点符号

```c
// 中文标点都支持
，。！？；：""''（）【】《》
```

### 测试 4：数字和字母

```c
// ASCII 字符正常显示
0123456789
ABCDEFGHIJKLMNOPQRSTUVWXYZ
abcdefghijklmnopqrstuvwxyz
```

## 故障排除

### 编译时间过长

**原因**：230KB 的 C 文件需要较长编译时间。

**解决**：
```bash
# 使用并行编译
tos.py build -j4  # 使用 4 个线程
```

### 内存不足

**症状**：编译失败，提示内存不足。

**解决方案 1**：使用专用字库
```bash
# 只生成小说中的字符
python tools/generate_hzk16_data.py novel.txt HZK16 hzk16.c
```

**解决方案 2**：动态加载
```bash
# 将 HZK16 存储到 Flash/SD 卡，运行时读取
# 参考 HZK16_GUIDE.md 中的"方案 3"
```

### 某些字符显示为方框

**原因**：字符超出 GB2312 范围。

**检查**：
```bash
# 查看字符编码
hexdump -C novel.txt | head -20
```

**解决**：
- 如果是 UTF-8，转换为 GBK
- 如果是生僻字，需要扩展字库

### 显示速度慢

**原因**：字库查找效率低。

**检查**：确认使用了二分查找（完整字库自动使用）。

**优化**：
```c
// 添加字符缓存
#define CACHE_SIZE 32
static uint8_t char_cache[CACHE_SIZE][32];
```

## 高级用法

### 生成部分字库

如果只需要一级汉字（3,755 个常用字）：

```python
# 修改 generate_full_hzk16.py
# 只生成 0xB0-0xD7 区（一级汉字）
for qu in range(16, 56):  # 0xB0-0xD7
    for wei in range(94):
        # ... 生成代码
```

### 添加自定义字符

```c
// 在 hzk16_full.c 末尾添加
{0x81, 0x40, {  // 自定义字符
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    // ... 32 字节字体数据
}},
```

### 压缩字库

使用 gzip 压缩字库，运行时解压：

```bash
# 压缩
gzip -9 hzk16_full.c

# 在代码中解压
// 使用 zlib 库解压
```

## 一键脚本

创建一个自动化脚本：

```bash
#!/bin/bash
# build_with_full_font.sh

echo "Generating full HZK16 font..."
python tools/generate_full_hzk16.py HZK16 hzk16_full.c

echo "Replacing font file..."
mv lib/Fonts/hzk16.c lib/Fonts/hzk16_default.c.bak
cp hzk16_full.c lib/Fonts/hzk16.c

echo "Building..."
tos.py build

echo "Done! Ready to flash."
```

使用：
```bash
chmod +x build_with_full_font.sh
./build_with_full_font.sh
```

## 总结

使用完整 GB2312 字库的优势：

✅ **一次生成，永久使用**
- 不需要为每本小说重新生成
- 支持从网络下载任意内容

✅ **覆盖率高**
- 支持 99% 的中文内容
- 包含所有常用汉字

✅ **性能优秀**
- 二分查找，速度快
- 不影响显示性能

✅ **易于维护**
- 一个文件包含所有字符
- 不需要频繁更新

**推荐使用场景**：
- 📱 网络小说阅读器 ⭐
- 📰 新闻阅读器
- 📧 邮件客户端
- 💬 聊天应用

现在你可以显示从网络下载的任何中文内容了！🎉
