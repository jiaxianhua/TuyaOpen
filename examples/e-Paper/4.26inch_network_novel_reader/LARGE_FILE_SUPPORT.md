# 大文件支持说明

## 文件大小限制

### 当前限制

- **最大文件大小**：50 MB
- **推荐大小**：< 20 MB
- **最小大小**：> 0 字节

### 为什么有限制？

1. **内存限制**：嵌入式设备内存有限
2. **读取性能**：大文件需要更长时间读取
3. **显示性能**：大文件分页计算更慢

## 文件大小检查

### 查看文件大小

```bash
# Linux/macOS
ls -lh /sdcard/*.txt

# 输出示例:
# -rw-r--r-- 1 user user 10.8M Dec 25 00:00 doupocangqiong.txt
```

### 文件大小分类

| 大小 | 分类 | 性能 | 建议 |
|------|------|------|------|
| < 1 MB | 小文件 | 快速 | ✅ 推荐 |
| 1-10 MB | 中等文件 | 正常 | ✅ 可用 |
| 10-20 MB | 大文件 | 较慢 | ⚠️ 注意 |
| 20-50 MB | 超大文件 | 很慢 | ⚠️ 谨慎 |
| > 50 MB | 过大 | 不支持 | ❌ 拒绝 |

## 处理大文件

### 方案 1：分割文件（推荐）

将大文件分割成多个小文件：

```bash
# 分割为每个 5 MB 的文件
split -b 5M doupocangqiong.txt doupocangqiong_part_

# 结果:
# doupocangqiong_part_aa (5 MB)
# doupocangqiong_part_ab (5 MB)
# doupocangqiong_part_ac (0.8 MB)

# 重命名为有意义的名字
mv doupocangqiong_part_aa doupocangqiong_01.txt
mv doupocangqiong_part_ab doupocangqiong_02.txt
mv doupocangqiong_part_ac doupocangqiong_03.txt
```

### 方案 2：按章节分割

更智能的分割方式：

```python
#!/usr/bin/env python3
# split_novel_by_chapter.py

import re

def split_by_chapter(input_file, output_prefix, max_size_mb=5):
    """按章节分割小说，每个文件不超过指定大小"""
    
    with open(input_file, 'r', encoding='gbk') as f:
        content = f.read()
    
    # 查找章节标记（根据实际格式调整）
    chapter_pattern = r'第[零一二三四五六七八九十百千万\d]+章'
    chapters = re.split(f'({chapter_pattern})', content)
    
    current_file = []
    current_size = 0
    file_num = 1
    max_size = max_size_mb * 1024 * 1024
    
    for i in range(0, len(chapters), 2):
        if i + 1 < len(chapters):
            chapter_title = chapters[i + 1]
            chapter_content = chapters[i + 2] if i + 2 < len(chapters) else ''
            chapter_text = chapter_title + chapter_content
        else:
            chapter_text = chapters[i]
        
        chapter_bytes = len(chapter_text.encode('gbk'))
        
        if current_size + chapter_bytes > max_size and current_file:
            # 保存当前文件
            output_file = f"{output_prefix}_{file_num:02d}.txt"
            with open(output_file, 'w', encoding='gbk') as f:
                f.write(''.join(current_file))
            print(f"Created: {output_file} ({current_size / 1024 / 1024:.2f} MB)")
            
            current_file = []
            current_size = 0
            file_num += 1
        
        current_file.append(chapter_text)
        current_size += chapter_bytes
    
    # 保存最后一个文件
    if current_file:
        output_file = f"{output_prefix}_{file_num:02d}.txt"
        with open(output_file, 'w', encoding='gbk') as f:
            f.write(''.join(current_file))
        print(f"Created: {output_file} ({current_size / 1024 / 1024:.2f} MB)")

if __name__ == '__main__':
    import sys
    if len(sys.argv) < 2:
        print("Usage: python3 split_novel_by_chapter.py <input_file> [output_prefix] [max_size_mb]")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_prefix = sys.argv[2] if len(sys.argv) > 2 else input_file.replace('.txt', '')
    max_size_mb = int(sys.argv[3]) if len(sys.argv) > 3 else 5
    
    split_by_chapter(input_file, output_prefix, max_size_mb)
```

使用方法：

```bash
# 按章节分割，每个文件最大 5 MB
python3 tools/split_novel_by_chapter.py doupocangqiong.txt doupocangqiong 5

# 输出:
# doupocangqiong_01.txt (5.0 MB) - 第1章到第50章
# doupocangqiong_02.txt (4.8 MB) - 第51章到第100章
# ...
```

### 方案 3：提取前几章

只提取小说的前几章进行测试：

```bash
# 提取前 1 MB
head -c 1048576 doupocangqiong.txt > doupocangqiong_preview.txt

# 或提取前 1000 行
head -n 1000 doupocangqiong.txt > doupocangqiong_preview.txt
```

## 性能影响

### 读取时间

| 文件大小 | 读取时间（估计） |
|---------|----------------|
| 1 MB | < 1 秒 |
| 5 MB | 1-2 秒 |
| 10 MB | 2-4 秒 |
| 20 MB | 4-8 秒 |
| 50 MB | 10-20 秒 |

### 内存使用

文件会完全加载到内存中：

- **10 MB 文件** → 需要约 10 MB RAM
- **50 MB 文件** → 需要约 50 MB RAM

如果设备内存不足，可能导致：
- 内存分配失败
- 系统崩溃
- 性能严重下降

## 优化建议

### 1. 使用分页读取（未来功能）

当前实现一次性读取整个文件。未来可以实现：

```c
// 伪代码
int sd_read_text_page(const char *filepath, int page_num, char **buffer, int *size) {
    // 只读取指定页的内容
    // 每页 50 KB
    // 按需加载，节省内存
}
```

### 2. 使用流式读取

对于超大文件，可以实现流式读取：

```c
// 伪代码
int sd_read_text_stream(const char *filepath, 
                        int (*callback)(const char *chunk, int size)) {
    // 分块读取，每次 64 KB
    // 通过回调函数处理每个块
}
```

### 3. 压缩文本

使用压缩可以减小文件大小：

```bash
# 压缩文本文件
gzip doupocangqiong.txt
# 结果: doupocangqiong.txt.gz (约 30-50% 大小)

# 在设备上解压（需要支持）
```

## 错误处理

### 错误信息

```
[01-01 00:01:00 ty E][sd_file_manager.c:248] Invalid file size: 10840334
```

**含义**：文件大小 10,840,334 字节（10.8 MB）超过限制

**解决方法**：
1. 分割文件为多个小文件
2. 提取部分内容
3. 增加代码中的限制（已修改为 50 MB）

### 内存分配失败

```
[01-01 00:01:00 ty E][sd_file_manager.c:256] Memory allocation failed
```

**含义**：无法分配足够内存

**解决方法**：
1. 使用更小的文件
2. 释放其他内存
3. 重启设备

## 最佳实践

### 1. 文件准备

```bash
# 1. 检查文件大小
ls -lh novel.txt

# 2. 如果 > 10 MB，分割文件
python3 tools/split_novel_by_chapter.py novel.txt novel 5

# 3. 转换为 GBK
for f in novel_*.txt; do
    java -jar tuya-converter/target/tuya-converter.jar "$f"
done

# 4. 复制到 SD 卡
cp novel_*_tuya.txt /sdcard/
```

### 2. 命名规范

使用有意义的文件名：

```
doupocangqiong_01_chapters_001-050.txt
doupocangqiong_02_chapters_051-100.txt
doupocangqiong_03_chapters_101-150.txt
```

### 3. 测试流程

```bash
# 1. 先测试小文件
cp small_test.txt /sdcard/
# 在设备上测试

# 2. 再测试中等文件（5 MB）
cp medium_test.txt /sdcard/
# 在设备上测试

# 3. 最后测试大文件（10-20 MB）
cp large_test.txt /sdcard/
# 在设备上测试
```

## 代码修改历史

### v1.0 - 原始限制

```c
if (file_size <= 0 || file_size > 1024 * 1024) {  // 1 MB
    PR_ERR("Invalid file size: %ld", file_size);
    return OPRT_COM_ERROR;
}
```

### v1.1 - 增加限制

```c
if (file_size <= 0 || file_size > 50 * 1024 * 1024) {  // 50 MB
    PR_ERR("Invalid file size: %ld (max 50 MB)", file_size);
    return OPRT_COM_ERROR;
}
PR_NOTICE("File size: %ld bytes (%.2f MB)", file_size, file_size / (1024.0 * 1024.0));
```

## 常见问题

### Q1: 为什么不支持无限大的文件？

A: 嵌入式设备内存有限，需要一次性加载整个文件到内存。

### Q2: 可以支持 100 MB 的文件吗？

A: 理论上可以，但：
- 读取时间很长（20-40 秒）
- 内存占用很大（100 MB）
- 可能导致系统不稳定

### Q3: 如何知道设备有多少内存？

A: 查看设备规格或在代码中添加内存检查。

### Q4: 分割文件会影响阅读体验吗？

A: 不会，可以按章节分割，每个文件是完整的章节。

### Q5: 可以自动分割文件吗？

A: 可以，使用提供的 Python 脚本自动按章节分割。

## 工具脚本

### split_novel_by_chapter.py

已包含在上面的"方案 2"中。

### check_file_size.sh

```bash
#!/bin/bash
# 检查文件大小并给出建议

file=$1
if [ ! -f "$file" ]; then
    echo "File not found: $file"
    exit 1
fi

size=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file" 2>/dev/null)
size_mb=$(echo "scale=2; $size / 1024 / 1024" | bc)

echo "File: $file"
echo "Size: $size bytes ($size_mb MB)"
echo ""

if (( $(echo "$size_mb < 1" | bc -l) )); then
    echo "✅ Small file - Perfect for e-reader"
elif (( $(echo "$size_mb < 10" | bc -l) )); then
    echo "✅ Medium file - Good for e-reader"
elif (( $(echo "$size_mb < 20" | bc -l) )); then
    echo "⚠️  Large file - May be slow"
    echo "Suggestion: Consider splitting into smaller files"
elif (( $(echo "$size_mb < 50" | bc -l) )); then
    echo "⚠️  Very large file - Will be slow"
    echo "Suggestion: Split into files < 10 MB each"
else
    echo "❌ File too large - Not supported"
    echo "Suggestion: Split into files < 10 MB each"
fi
```

使用方法：

```bash
chmod +x tools/check_file_size.sh
./tools/check_file_size.sh doupocangqiong.txt
```

## 总结

- **当前限制**：50 MB（已从 1 MB 增加）
- **推荐大小**：< 10 MB
- **最佳实践**：按章节分割大文件
- **工具支持**：提供分割脚本

---

**最后更新**：2025-12-25
**版本**：v1.1
