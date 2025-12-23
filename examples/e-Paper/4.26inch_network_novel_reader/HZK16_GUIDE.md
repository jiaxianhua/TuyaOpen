# HZK16 中文字库使用指南

本项目已添加 HZK16 中文字库支持，可以显示真实的中文字符而不是占位符。

## 什么是 HZK16？

HZK16 是一个 16x16 点阵中文字库：
- **字体大小**：16x16 像素
- **每个字符**：32 字节（16行 x 2字节/行）
- **支持字符**：GB2312 标准（约 6,763 个常用汉字）
- **文件大小**：约 260KB

## 快速开始

### 方案 1：使用内置字符（推荐用于测试）

当前实现包含少量常用汉字（你、好、中、文等）。这些字符可以直接显示，无需额外配置。

**优点**：
- 无需下载字库文件
- 编译后直接可用
- 适合快速测试

**缺点**：
- 只支持少量字符
- 其他字符显示为占位符 `[]`

### 方案 2：生成完整字库（推荐用于生产）

根据你的小说内容生成所需的字库数据。

#### 步骤 1：下载 HZK16 字库

```bash
# 从 GitHub 下载
wget https://github.com/aguegu/BitmapFont/raw/master/font/HZK16

# 或者从其他镜像下载
# 搜索 "HZK16 download" 找到下载链接
```

#### 步骤 2：准备 GBK 编码的小说

```bash
# 如果是 UTF-8，转换为 GBK
iconv -f UTF-8 -t GBK novel_utf8.txt -o novel_gbk.txt
```

#### 步骤 3：生成字库数据

```bash
cd examples/e-Paper/4.26inch_network_novel_reader

# 生成字库 C 代码
python tools/generate_hzk16_data.py novel_gbk.txt HZK16 lib/Fonts/hzk16_generated.c
```

这会：
1. 扫描小说中的所有中文字符
2. 从 HZK16 文件读取字体数据
3. 生成 `hzk16_generated.c` 文件

#### 步骤 4：使用生成的字库

编辑 `CMakeLists.txt`，确保包含生成的文件：

```cmake
set(APP_SRC
    ${APP_SRC_EXAMPLES}
    ${APP_SRC_CONFIG}
    ${APP_SRC_EPAPER}
    ${APP_SRC_FONTS}
    ${APP_SRC_GUI}
)
```

`hzk16_generated.c` 会自动替换 `hzk16.c` 中的默认实现。

#### 步骤 5：编译和烧录

```bash
tos.py build
tos.py flash
```

## 字库大小估算

每个汉字占用约 34 字节（2字节索引 + 32字节字体数据）。

| 字符数 | 大小 | 说明 |
|--------|------|------|
| 100 | 3.4 KB | 短文章 |
| 500 | 17 KB | 中篇小说 |
| 1000 | 34 KB | 长篇小说 |
| 2000 | 68 KB | 多本小说 |
| 6763 | 230 KB | 完整 GB2312 |

**建议**：
- 对于单本小说，生成专用字库（通常 < 50KB）
- 对于多本小说，可以生成完整字库（约 230KB）

## 方案 3：从 Flash/SD 卡加载（高级）

如果内存有限，可以将 HZK16 文件存储在 Flash 或 SD 卡，运行时动态读取。

### 修改 `hzk16.c`

```c
#include "tal_fs.h"  // TuyaOpen 文件系统

int hzk16_get_font_data(uint8_t gb_high, uint8_t gb_low, uint8_t *buffer)
{
    // 计算偏移
    int qu = gb_high - 0xA1;
    int wei = gb_low - 0xA1;
    
    if (qu < 0 || qu >= 94 || wei < 0 || wei >= 94) {
        return -1;
    }
    
    int offset = (qu * 94 + wei) * 32;
    
    // 从文件读取
    int fd = tal_fs_open("/flash/HZK16", TAL_FS_O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    tal_fs_lseek(fd, offset, TAL_FS_SEEK_SET);
    int ret = tal_fs_read(fd, buffer, 32);
    tal_fs_close(fd);
    
    return (ret == 32) ? 0 : -1;
}
```

### 上传 HZK16 到设备

```bash
# 使用 tyutool 上传文件到 Flash
tyutool upload HZK16 /flash/HZK16
```

## 字体效果

### 16x16 点阵效果

```
█ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █
█ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ █
█ ░ ░ ░ ░ █ █ █ █ █ ░ ░ ░ ░ ░ █
█ ░ ░ ░ █ ░ ░ ░ ░ ░ █ ░ ░ ░ ░ █
█ ░ ░ █ ░ ░ ░ ░ ░ ░ ░ █ ░ ░ ░ █
█ ░ █ ░ ░ ░ ░ ░ ░ ░ ░ ░ █ ░ ░ █
█ █ █ █ █ █ █ █ █ █ █ █ █ █ ░ █
█ ░ █ ░ ░ ░ ░ ░ ░ ░ ░ ░ █ ░ ░ █
█ ░ ░ █ ░ ░ ░ ░ ░ ░ ░ █ ░ ░ ░ █
█ ░ ░ ░ █ ░ ░ ░ ░ ░ █ ░ ░ ░ ░ █
█ ░ ░ ░ ░ █ █ █ █ █ ░ ░ ░ ░ ░ █
█ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ ░ █
█ █ █ █ █ █ █ █ █ █ █ █ █ █ █ █
```

这是"中"字的 16x16 点阵。

## 常见问题

### Q: 为什么有些字符显示为 `[]`？

A: 这些字符不在字库中。解决方法：
1. 重新生成字库，包含这些字符
2. 或者使用完整的 GB2312 字库

### Q: 如何添加更多字符？

A: 
```bash
# 合并多个文本文件
cat novel1.txt novel2.txt > all_novels.txt

# 重新生成字库
python tools/generate_hzk16_data.py all_novels.txt HZK16 lib/Fonts/hzk16_generated.c
```

### Q: 字库太大，内存不够怎么办？

A: 三个选择：
1. 只生成当前小说需要的字符
2. 使用方案 3，从 Flash/SD 卡动态加载
3. 使用更小的字体（如 HZK12，12x12 像素）

### Q: 支持繁体字吗？

A: HZK16 主要支持简体字（GB2312）。繁体字需要：
1. 使用 Big5 编码的字库
2. 或者使用 Unicode 字库（如 unifont）

### Q: 可以使用其他字体吗？

A: 可以，但需要相应的字库文件：
- **HZK12**：12x12 像素，更小但不太清晰
- **HZK24**：24x24 像素，更清晰但占用更多空间
- **ASC16**：16x8 ASCII 字体
- **Unifont**：16x16 Unicode 字体（支持多语言）

## 性能优化

### 1. 字符查找优化

当前使用线性搜索。对于大字库，可以使用二分查找：

```c
// 确保字符按 GBK 编码排序
int hzk16_get_font_data(uint8_t gb_high, uint8_t gb_low, uint8_t *buffer)
{
    int left = 0;
    int right = HZK16_EMBEDDED_COUNT - 1;
    
    while (left <= right) {
        int mid = (left + right) / 2;
        uint16_t target = (gb_high << 8) | gb_low;
        uint16_t current = (hzk16_embedded[mid].gb_high << 8) | 
                          hzk16_embedded[mid].gb_low;
        
        if (current == target) {
            memcpy(buffer, hzk16_embedded[mid].data, 32);
            return 0;
        } else if (current < target) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    
    return -1;
}
```

### 2. 缓存最近使用的字符

```c
#define CACHE_SIZE 16

static struct {
    uint8_t gb_high;
    uint8_t gb_low;
    uint8_t data[32];
    int valid;
} font_cache[CACHE_SIZE];

static int cache_index = 0;
```

### 3. 预渲染常用字符

对于特别常用的字符（如"的"、"了"、"是"），可以预渲染到位图。

## 工具脚本

### 统计字符频率

```bash
python tools/analyze_chars.py novel.txt
```

输出：
```
Top 20 most frequent characters:
的: 1234 times
了: 987 times
是: 765 times
...
```

### 提取唯一字符

```bash
python tools/extract_unique_chars.py novel.txt unique_chars.txt
```

### 验证字库完整性

```bash
python tools/verify_font.py novel.txt hzk16_generated.c
```

## 参考资料

- [HZK16 字库格式](https://www.cnblogs.com/amanlikethis/p/3595417.html)
- [GB2312 编码表](https://zh.wikipedia.org/wiki/GB_2312)
- [点阵字体原理](https://blog.csdn.net/weixin_42837024/article/details/81945540)
- [HZK16 下载](https://github.com/aguegu/BitmapFont)

## 总结

| 方案 | 优点 | 缺点 | 适用场景 |
|------|------|------|----------|
| 内置少量字符 | 简单，无需配置 | 字符有限 | 测试、演示 |
| 生成专用字库 | 大小可控，速度快 | 需要预处理 | 单本小说 |
| 完整 GB2312 | 支持所有常用字 | 占用空间大 | 多本小说 |
| 动态加载 | 节省内存 | 速度较慢 | 内存受限设备 |

**推荐**：对于大多数应用，使用"生成专用字库"方案，平衡了大小和功能。
