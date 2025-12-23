# 内置小说使用指南 / Embedded Novel Guide

## 功能说明

项目已修改为使用**内置小说**模式：
1. 设备启动后自动连接 WiFi
2. WiFi 连接成功后，显示"Press button to load novel"
3. **按下按钮**加载内置的小说内容
4. 加载完成后自动显示第一页
5. 短按按钮翻到下一页，长按按钮返回上一页

## 如何添加自己的小说

### 步骤 1: 准备 txt 文件

将你的小说文本文件（UTF-8 编码）放到项目根目录：
```
examples/e-Paper/4.26inch_network_novel_reader/your_novel.txt
```

### 步骤 2: 转换为 C 数组

运行转换脚本：
```bash
cd examples/e-Paper/4.26inch_network_novel_reader
python3 tools/txt_to_c_array.py your_novel.txt 50
```

参数说明：
- `your_novel.txt` - 你的小说文件
- `50` - 最大大小（KB），默认 50KB

**注意**：由于固件大小限制，建议不超过 100KB。

### 步骤 3: 重新编译

```bash
tos.py build
tos.py flash
```

## 文件大小建议

| 大小 | 说明 | 大约页数 |
|------|------|---------|
| 50KB | 推荐 | ~40 页 |
| 100KB | 最大 | ~80 页 |
| 200KB+ | 不推荐 | 可能导致内存不足 |

## 中文显示说明

当前版本使用 ASCII 字体显示，中文会显示为乱码或方框。

### 解决方案

要正确显示中文，需要：

1. **添加中文字体库**（需要额外的 Flash 空间）
2. **使用 UTF-8 编码**
3. **修改显示函数**使用中文字体

由于中文字体库通常很大（几 MB），建议：
- 使用简化的中文字体（只包含常用汉字）
- 或者使用图片方式显示文字
- 或者使用拼音/英文内容

## 当前限制

1. **字体限制**：只支持 ASCII 字符（英文、数字、标点）
2. **大小限制**：建议不超过 100KB
3. **内存限制**：设备 RAM 有限，太大的文件可能导致崩溃

## 使用流程

```
1. 设备启动
   ↓
2. 连接 WiFi (自动)
   ↓
3. 显示 "WiFi Connected! Press button to load novel"
   ↓
4. 按下按钮
   ↓
5. 加载内置小说 (从固件中)
   ↓
6. 显示第一页
   ↓
7. 短按 = 下一页
   长按 = 上一页
```

## 示例：使用英文小说

如果要显示英文小说（完美支持）：

```bash
# 下载一本英文小说（例如从 Project Gutenberg）
wget http://www.gutenberg.org/files/1342/1342-0.txt -O pride_and_prejudice.txt

# 转换为 C 数组
python3 tools/txt_to_c_array.py pride_and_prejudice.txt 80

# 编译烧录
tos.py build
tos.py flash
```

## 故障排除

### 问题 1: 编译错误 "embedded_novel_data undeclared"

**原因**：没有运行转换脚本生成 `embedded_novel.c`

**解决**：
```bash
python3 tools/txt_to_c_array.py 乱世为王.txt 50
```

### 问题 2: 显示乱码

**原因**：中文字符无法用 ASCII 字体显示

**解决**：
- 使用英文内容
- 或添加中文字体支持（需要修改代码）

### 问题 3: 设备重启或崩溃

**原因**：文件太大，内存不足

**解决**：
```bash
# 减小文件大小到 30KB
python3 tools/txt_to_c_array.py your_novel.txt 30
```

### 问题 4: 按钮没反应

**原因**：按钮 GPIO 配置不正确

**解决**：参考 [BUTTON_SETUP.md](BUTTON_SETUP.md) 配置正确的 GPIO

## 技术细节

### 生成的文件

转换脚本会生成两个文件：
- `examples/embedded_novel.c` - 包含小说数据的 C 数组
- `examples/embedded_novel.h` - 头文件声明

### 数据格式

```c
const uint8_t embedded_novel_data[] = {
    0x0a, 0x2d, 0x2d, 0x2d, ...  // 小说内容的字节数组
};

const uint32_t embedded_novel_data_size = 51200;  // 数据大小
```

### 内存使用

- **Flash**: 存储小说数据（编译到固件中）
- **RAM**: 运行时复制到内存（约等于文件大小）
- **显示缓冲**: ~48KB（固定）

## 进阶：添加中文字体支持

如果你需要显示中文，需要：

1. 添加中文字体库到 `lib/Fonts/`
2. 修改 `display_page()` 函数使用中文字体
3. 处理 UTF-8 编码的字符串

这需要较多的修改和额外的 Flash 空间（中文字体通常 2-5MB）。

## 参考资料

- [README.md](README.md) - 项目总体说明
- [BUTTON_SETUP.md](BUTTON_SETUP.md) - 按钮配置指南
- [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md) - 技术细节

---

**提示**：如果你的小说是中文的，建议先转换为拼音或使用英文小说测试功能。
