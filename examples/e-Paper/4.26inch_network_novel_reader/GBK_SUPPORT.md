# GBK 编码支持说明

本项目已添加对 GBK 编码中文小说的支持。

## 主要特性

### 1. GBK 字符识别

```c
static int is_gbk_lead_byte(unsigned char c)
{
    return (c >= 0x81 && c <= 0xFE);
}
```

- 检测 GBK 首字节范围：0x81-0xFE
- GBK 第二字节范围：0x40-0xFE（除了 0x7F）

### 2. 字符长度计算

```c
static int get_char_byte_len(const char *str, int pos, int max_len)
```

- ASCII 字符：1 字节
- GBK 中文字符：2 字节
- 自动检测并返回正确的字节长度

### 3. 智能分页算法

```c
static int calculate_page_offsets(void)
```

**分页逻辑：**
- 每行最多 40 个字符单位
- ASCII 字符占 1 个单位
- GBK 中文字符占 2 个单位
- 每页最多 22 行
- 自动处理换行符（\n, \r, \r\n）
- 跳过控制字符

**示例：**
```
每行可以显示：
- 40 个英文字符
- 20 个中文字符
- 或混合（如：10个中文 + 20个英文）
```

### 4. 显示渲染

```c
static void display_page(void)
```

**当前实现：**
- ASCII 字符：使用 Font16 正常显示
- GBK 字符：显示为占位符 `[]`

**为什么使用占位符？**
- 完整的 GBK 字库文件很大（约 2-4MB）
- 需要额外的字库数据和渲染代码
- 占位符可以验证分页逻辑正确性

## 配置参数

```c
#define CHARS_PER_LINE 40      // 每行字符单位数
#define LINES_PER_PAGE 22      // 每页行数
#define BYTES_PER_PAGE 2000    // 估算的每页字节数
```

## 小说 URL 配置

```c
#define NOVEL_URL "http://120.79.89.230/fanren.txt"
```

**要求：**
- 必须是 HTTP 协议（不支持 HTTPS）
- 文件必须是 GBK 编码
- 建议文件大小 < 1MB

## GBK 编码转换

### 从 UTF-8 转换为 GBK

**方法 1：使用 iconv（Linux/Mac）**
```bash
iconv -f UTF-8 -t GBK input.txt -o output.txt
```

**方法 2：使用 Python**
```python
# 读取 UTF-8 文件并转换为 GBK
with open('input.txt', 'r', encoding='utf-8') as f:
    content = f.read()

with open('output.txt', 'wb') as f:
    f.write(content.encode('gbk'))
```

**方法 3：使用在线工具**
- 搜索 "UTF-8 to GBK converter"
- 上传文件并下载转换后的版本

### 验证编码

**Linux/Mac:**
```bash
file -i your_novel.txt
# 应该显示: charset=unknown-8bit 或 charset=iso-8859-1
```

**Python:**
```python
with open('your_novel.txt', 'rb') as f:
    data = f.read(100)
    print(data)
    # 中文字符应该显示为 \x?? 格式的字节
```

## 添加完整 GBK 字库支持

如果你想显示真实的中文字符而不是占位符，需要：

### 1. 准备 GBK 字库

**选项 A：使用点阵字库**
- 下载 GBK 16x16 点阵字库（HZK16）
- 文件大小约 260KB
- 每个汉字占 32 字节

**选项 B：使用 TrueType 字体**
- 需要字体渲染库（如 FreeType）
- 更灵活但占用更多资源

### 2. 修改显示代码

在 `display_page()` 函数中：

```c
// 当前代码（占位符）
if (is_gbk_lead_byte(c) && i + 1 < line_len) {
    Paint_DrawString_EN(x_pos, y_pos, "[]", &Font16, WHITE, BLACK);
    x_pos += 16;
    i += 2;
}

// 改为（使用字库）
if (is_gbk_lead_byte(c) && i + 1 < line_len) {
    unsigned char gb_code[2] = {line_buf[i], line_buf[i+1]};
    draw_gbk_char(x_pos, y_pos, gb_code, &Font16, WHITE, BLACK);
    x_pos += 16;
    i += 2;
}
```

### 3. 实现 GBK 字符绘制

```c
void draw_gbk_char(int x, int y, unsigned char *gb_code, 
                   sFONT* Font, UWORD fg, UWORD bg)
{
    // 计算字库偏移
    unsigned char high = gb_code[0] - 0x81;
    unsigned char low = gb_code[1] - 0x40;
    if (low > 0x7E) low--;
    
    int offset = (high * 190 + low) * 32;  // 16x16 = 32 bytes
    
    // 从字库读取点阵数据
    unsigned char *font_data = &hzk16_data[offset];
    
    // 绘制 16x16 点阵
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16; col++) {
            int byte_idx = row * 2 + col / 8;
            int bit_idx = 7 - (col % 8);
            
            if (font_data[byte_idx] & (1 << bit_idx)) {
                Paint_SetPixel(x + col, y + row, fg);
            } else if (bg != FONT_BACKGROUND) {
                Paint_SetPixel(x + col, y + row, bg);
            }
        }
    }
}
```

## 性能优化

### 内存使用

- **显示缓冲区**：约 48KB（800x480/8）
- **小说内容**：取决于文件大小
- **分页索引**：每页 4 字节（100页 = 400字节）

### 下载优化

如果小说很大，可以考虑：

1. **分段下载**：只下载当前章节
2. **压缩传输**：使用 gzip 压缩
3. **缓存到 Flash**：下载后保存到本地

### 显示优化

- **局部刷新**：只刷新变化的区域
- **快速模式**：使用墨水屏的快速刷新模式
- **预渲染**：提前渲染下一页

## 测试建议

### 1. 测试 ASCII 文本

```c
#define NOVEL_URL "http://example.com/english.txt"
```

验证基本功能正常。

### 2. 测试 GBK 文本

```c
#define NOVEL_URL "http://120.79.89.230/fanren.txt"
```

验证 GBK 分页正确。

### 3. 测试混合文本

创建包含中英文混合的测试文件：
```
第一章 Introduction
这是一个测试 This is a test
中文English混合Mixed content
```

### 4. 边界测试

- 空文件
- 单行文件
- 超长行（无换行符）
- 特殊字符（标点符号）

## 常见问题

### Q: 为什么中文显示为方框？

A: 当前版本使用占位符 `[]` 表示 GBK 字符。要显示真实中文，需要添加 GBK 字库。

### Q: 如何判断文件是 GBK 编码？

A: 
1. 用十六进制编辑器查看，中文字符应该是两个字节
2. 第一个字节在 0x81-0xFE 范围
3. 第二个字节在 0x40-0xFE 范围（除了 0x7F）

### Q: 支持 UTF-8 吗？

A: 当前版本不支持。UTF-8 使用可变长度编码（1-4字节），需要不同的解析逻辑。

### Q: 分页不准确怎么办？

A: 调整这些参数：
```c
#define CHARS_PER_LINE 40  // 减少以避免行溢出
#define LINES_PER_PAGE 22  // 减少以避免页溢出
```

### Q: 下载很慢怎么办？

A: 
1. 检查网络连接
2. 使用更小的测试文件
3. 考虑使用内嵌小说功能

## 参考资料

- [GBK 编码标准](https://zh.wikipedia.org/wiki/GBK)
- [HZK16 字库格式](https://www.cnblogs.com/amanlikethis/p/3595417.html)
- [墨水屏驱动文档](https://www.waveshare.com/wiki/4.26inch_e-Paper)
