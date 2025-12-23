# 快速开始 - 内置小说模式

## 一键生成并编译

```bash
cd examples/e-Paper/4.26inch_network_novel_reader

# 1. 转换小说为 C 数组（50KB）
python3 tools/txt_to_c_array.py 乱世为王.txt 50

# 2. 编译
tos.py build

# 3. 烧录
tos.py flash

# 4. 查看输出
tos.py monitor
```

## 使用方法

1. **设备启动** → 自动连接 WiFi
2. **WiFi 连接成功** → 显示 "Press button to load novel"
3. **按下按钮** → 加载并显示小说第一页
4. **短按按钮** → 下一页
5. **长按按钮（3秒）** → 上一页

## 重要提示

⚠️ **中文显示问题**：当前使用 ASCII 字体，中文会显示为乱码。

**解决方案**：
- 使用英文小说（完美支持）
- 或等待添加中文字体支持

## 修改小说内容

```bash
# 替换为你的小说文件
python3 tools/txt_to_c_array.py your_novel.txt 50

# 重新编译
tos.py build
tos.py flash
```

## 文件大小限制

- **推荐**: 50KB（约 40 页）
- **最大**: 100KB（约 80 页）
- **超过**: 可能导致内存不足

详细说明请查看 [EMBEDDED_NOVEL_GUIDE.md](EMBEDDED_NOVEL_GUIDE.md)
