# 中文文本显示指南 / Chinese Text Display Guide

## 中文显示原理

本项目使用 **HZK24 字库**显示中文，支持 **GBK 编码**。

### 工作流程
1. 读取 TXT 文件（GBK 或 UTF-8 编码）
2. 识别中文字符（双字节 GBK 编码）
3. 从 HZK24 字库获取字形数据
4. 渲染 24x24 像素的中文字符

## 支持的编码

### GBK 编码（推荐）
- ✅ 完全支持
- ✅ 包含所有常用汉字
- ✅ 兼容 ASCII
- ✅ 文件更小

### UTF-8 编码
- ⚠️ 部分支持
- ⚠️ 需要 UTF-8 到 GBK 的转换
- ⚠️ 某些字符可能无法显示

## 如何确保中文正确显示

### 1. 检查文件编码

#### Windows
```bash
# 使用记事本另存为，选择 "ANSI" 或 "GBK"
# 或使用 iconv 转换
iconv -f UTF-8 -t GBK input.txt > output.txt
```

#### Linux/Mac
```bash
# 检查文件编码
file -i yourfile.txt

# 转换 UTF-8 到 GBK
iconv -f UTF-8 -t GBK input.txt > output.txt

# 或使用 enca
enca -L zh_CN -x GBK yourfile.txt
```

#### Python 脚本
```python
# 转换编码
with open('input.txt', 'r', encoding='utf-8') as f:
    content = f.read()

with open('output.txt', 'w', encoding='gbk') as f:
    f.write(content)
```

### 2. 创建测试文件

创建一个测试文件 `test_chinese.txt`（GBK 编码）：

```
这是中文测试文件
This is English text
中英文混合显示测试
Mixed Chinese and English display test

常用汉字测试：
你好世界！
欢迎使用电子墨水屏阅读器。

特殊字符：
《》【】""''
！？，。；：
```

### 3. 验证显示

将文件复制到 SD 卡根目录：
```bash
cp test_chinese.txt /sdcard/
```

在设备上打开文件，应该能看到：
- ✅ 中文字符正常显示（24x24 像素）
- ✅ 英文字符正常显示（12x24 像素）
- ✅ 标点符号正常显示
- ✅ 自动换行

## 常见问题

### 问题 1：中文显示为乱码或方块

**原因**：
- 文件编码不是 GBK
- HZK24 字库未包含该字符

**解决方案**：
```bash
# 1. 转换文件编码为 GBK
iconv -f UTF-8 -t GBK input.txt > output.txt

# 2. 检查是否使用了生僻字
# 常用汉字（GB2312）都应该能显示
```

### 问题 2：部分中文无法显示

**原因**：
- 使用了 GBK 扩展区的生僻字
- HZK24 字库不完整

**解决方案**：
```bash
# 使用完整的 HZK24 字库
# 参考 FULL_FONT_GUIDE.md
python tools/generate_full_hzk24.py
```

### 问题 3：中英文混合显示不对齐

**原因**：
- 中文 24 像素宽，英文 12 像素宽
- 这是正常的，因为字体大小不同

**说明**：
- 中文：24x24 像素（HZK24）
- 英文：12x24 像素（Font24）
- 一个中文 = 两个英文宽度

### 问题 4：标点符号显示异常

**原因**：
- 使用了全角/半角标点混合
- 编码问题

**解决方案**：
```python
# 统一使用全角标点（中文）
text = text.replace(',', '，')
text = text.replace('.', '。')
text = text.replace('!', '！')
text = text.replace('?', '？')
```

## 字符宽度计算

### 显示宽度
- 屏幕宽度：800 像素（横屏）或 480 像素（竖屏）
- 旋转 90 度后：实际显示宽度 = 480 像素

### 每行字符数
```
竖屏模式（ROTATE_90）：
- 可用宽度：480 像素
- 中文字符：24 像素/字
- 英文字符：12 像素/字
- 每行最多：20 个中文 或 40 个英文
```

### 当前配置
```c
#define CHARS_PER_LINE 37  // 混合中英文时的字符单位数
#define LINES_PER_PAGE 32  // 每页行数
```

## 编码转换工具

### 批量转换脚本

创建 `convert_to_gbk.sh`：
```bash
#!/bin/bash
# 批量转换 UTF-8 文件到 GBK

for file in *.txt; do
    if [ -f "$file" ]; then
        echo "Converting $file..."
        iconv -f UTF-8 -t GBK "$file" > "${file%.txt}_gbk.txt"
    fi
done

echo "Conversion complete!"
```

### Python 转换工具

创建 `convert_encoding.py`：
```python
#!/usr/bin/env python3
import sys
import os

def convert_to_gbk(input_file, output_file=None):
    """Convert text file to GBK encoding"""
    if output_file is None:
        base, ext = os.path.splitext(input_file)
        output_file = f"{base}_gbk{ext}"
    
    try:
        # Read with UTF-8
        with open(input_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Write with GBK
        with open(output_file, 'w', encoding='gbk') as f:
            f.write(content)
        
        print(f"✓ Converted: {input_file} -> {output_file}")
        return True
    except Exception as e:
        print(f"✗ Error: {e}")
        return False

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python convert_encoding.py <input_file> [output_file]")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else None
    
    convert_to_gbk(input_file, output_file)
```

使用方法：
```bash
python convert_encoding.py input.txt output.txt
```

## 测试清单

在 SD 卡上测试以下内容：

- [ ] 纯中文文本
- [ ] 纯英文文本
- [ ] 中英文混合
- [ ] 中文标点符号
- [ ] 英文标点符号
- [ ] 数字
- [ ] 特殊符号
- [ ] 长文本（多页）
- [ ] 短文本（单页）
- [ ] 空行处理

## 性能建议

1. **文件大小**：< 500KB 为佳
2. **编码格式**：GBK 优于 UTF-8
3. **字符集**：常用汉字（GB2312）显示最快
4. **行长度**：建议每行 < 40 字符

## 参考文档

- [GBK_SUPPORT.md](./GBK_SUPPORT.md) - GBK 编码支持
- [HZK16_GUIDE.md](./HZK16_GUIDE.md) - HZK16 字库指南
- [FULL_FONT_GUIDE.md](./FULL_FONT_GUIDE.md) - 完整字库指南

## 示例文件

项目包含的示例文件：
- `sample1.txt` - 中英文混合示例（自动生成）
- `readme.txt` - 英文说明（自动生成）
- `乱世为王.txt` - 完整中文小说示例

## 技术细节

### GBK 字符识别
```c
// 判断是否为 GBK 首字节
static int is_gbk_lead_byte(unsigned char c)
{
    return (c >= 0x81 && c <= 0xFE);
}

// 获取字符字节长度
static int get_char_byte_len(const char *str, int pos, int max_len)
{
    unsigned char c = (unsigned char)str[pos];
    
    // ASCII
    if (c < 0x80) return 1;
    
    // GBK (2 bytes)
    if (is_gbk_lead_byte(c) && pos + 1 < max_len) {
        unsigned char c2 = (unsigned char)str[pos + 1];
        if (c2 >= 0x40 && c2 <= 0xFE && c2 != 0x7F) {
            return 2;
        }
    }
    
    return 1;
}
```

### 中文渲染
```c
// 使用 HZK24 渲染中文
draw_gbk_char24(x, y, gb_high, gb_low, BLACK, WHITE);

// 使用 Font24 渲染英文
Paint_DrawString_EN(x, y, ascii_str, &Font24, BLACK, WHITE);
```

---

如有问题，请检查：
1. 文件编码是否为 GBK
2. HZK24 字库是否完整
3. 字符是否在 GB2312 范围内
4. 日志输出的调试信息
