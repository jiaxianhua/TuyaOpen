# 大文件问题快速解决

## 问题

```
[01-01 00:01:00 ty E][sd_file_manager.c:248] Invalid file size: 10840334
```

文件太大（10.8 MB），超过限制。

## 快速解决方案

### 方案 1：重新编译（推荐）

代码已修改，支持最大 50 MB 文件：

```bash
# 重新编译
cd examples/e-Paper/4.26inch_network_novel_reader
tos.py build

# 刷新到设备
tos.py flash
```

### 方案 2：分割文件

如果不想重新编译，分割文件：

```bash
# 安装依赖（如果需要）
pip3 install chardet

# 分割文件（每个 5 MB）
python3 tools/split_novel_by_chapter.py /sdcard/doupocangqiong.txt doupocangqiong 5

# 结果:
# doupocangqiong_01.txt (5.0 MB)
# doupocangqiong_02.txt (4.8 MB)
# doupocangqiong_03.txt (1.0 MB)
```

### 方案 3：提取部分内容

只测试前几章：

```bash
# 提取前 5 MB
head -c 5242880 /sdcard/doupocangqiong.txt > /sdcard/doupocangqiong_preview.txt
```

## 详细步骤

### 步骤 1：检查文件大小

```bash
ls -lh /sdcard/doupocangqiong.txt
# 输出: -rw-r--r-- 1 user user 10.8M Dec 25 00:00 doupocangqiong.txt
```

### 步骤 2：分割文件

```bash
cd examples/e-Paper/4.26inch_network_novel_reader

# 分割为每个 5 MB
python3 tools/split_novel_by_chapter.py /sdcard/doupocangqiong.txt doupocangqiong 5

# 输出:
# 检测到输入文件编码: gbk
# 文件总大小: 5420167 字符
# 找到 1234 个章节
# ✓ 创建: doupocangqiong_01.txt (5.00 MB, 章节 1-500)
# ✓ 创建: doupocangqiong_02.txt (4.80 MB, 章节 501-1000)
# ✓ 创建: doupocangqiong_03.txt (1.00 MB, 章节 1001-1234)
# 完成! 共创建 3 个文件
```

### 步骤 3：复制到 SD 卡

```bash
# 复制分割后的文件
cp doupocangqiong_*.txt /sdcard/

# 验证
ls -lh /sdcard/doupocangqiong_*.txt
```

### 步骤 4：测试

在设备上测试读取分割后的文件。

## 代码修改说明

### 修改前（v1.0）

```c
if (file_size <= 0 || file_size > 1024 * 1024) {  // 1 MB 限制
    PR_ERR("Invalid file size: %ld", file_size);
    return OPRT_COM_ERROR;
}
```

### 修改后（v1.1）

```c
if (file_size <= 0 || file_size > 50 * 1024 * 1024) {  // 50 MB 限制
    PR_ERR("Invalid file size: %ld (max 50 MB)", file_size);
    return OPRT_COM_ERROR;
}
PR_NOTICE("File size: %ld bytes (%.2f MB)", file_size, file_size / (1024.0 * 1024.0));
```

## 文件大小建议

| 大小 | 建议 |
|------|------|
| < 5 MB | ✅ 最佳 |
| 5-10 MB | ✅ 良好 |
| 10-20 MB | ⚠️ 可用但较慢 |
| 20-50 MB | ⚠️ 很慢 |
| > 50 MB | ❌ 需要分割 |

## 工具使用

### split_novel_by_chapter.py

```bash
# 基本用法
python3 tools/split_novel_by_chapter.py novel.txt

# 指定输出前缀和大小
python3 tools/split_novel_by_chapter.py novel.txt output 10

# 指定编码
python3 tools/split_novel_by_chapter.py novel.txt output 5 gbk
```

### check_encoding.py

```bash
# 检查文件编码
python3 tools/check_encoding.py novel.txt

# 转换为 GBK
python3 tools/check_encoding.py novel.txt --convert
```

## 常见问题

### Q: 重新编译后还是报错？

A: 确保：
1. 代码已更新
2. 重新编译成功
3. 刷新到设备

### Q: 分割后文件名太长？

A: 使用短前缀：

```bash
python3 tools/split_novel_by_chapter.py doupocangqiong.txt dpcq 5
# 输出: dpcq_01.txt, dpcq_02.txt, ...
```

### Q: 如何合并分割的文件？

A: 在 Linux/macOS:

```bash
cat doupocangqiong_*.txt > doupocangqiong_full.txt
```

### Q: 分割会破坏章节吗？

A: 不会，工具会在章节边界分割。

## 相关文档

- **[LARGE_FILE_SUPPORT.md](LARGE_FILE_SUPPORT.md)** - 完整的大文件支持说明
- **[GBK_ENCODING_DEBUG.md](GBK_ENCODING_DEBUG.md)** - 编码问题诊断

---

**快速命令**：

```bash
# 1. 分割文件
python3 tools/split_novel_by_chapter.py /sdcard/doupocangqiong.txt doupocangqiong 5

# 2. 复制到 SD 卡
cp doupocangqiong_*.txt /sdcard/

# 3. 测试
# 在设备上打开文件
```

**最后更新**：2025-12-25
