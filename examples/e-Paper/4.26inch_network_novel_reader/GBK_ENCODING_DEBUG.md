# GBK 编码问题诊断指南

## 问题现象

SD 卡上的中文文本文件显示乱码，即使文件名看起来是 GBK 编码。

## 可能的原因

1. **文件实际不是 GBK 编码**
   - 文件可能是 UTF-8、GB2312 或其他编码
   - 文件名不代表文件内容的编码

2. **文件有 BOM 标记**
   - UTF-8 with BOM 会导致 GBK 解析错误
   - 需要移除 BOM

3. **文件包含非 GBK 字符**
   - 某些特殊字符不在 GBK 字符集中
   - 需要清理或转换

## 诊断步骤

### 步骤 1：检查文件编码

使用提供的工具检查文件实际编码：

```bash
# 安装 chardet（如果还没安装）
pip3 install chardet

# 检查文件编码
python3 tools/check_encoding.py /path/to/fanrenxiuxianchuan.txt
```

**输出示例**：
```
文件: fanrenxiuxianchuan.txt
检测到的编码: utf-8
置信度: 99.0%

使用 utf-8 编码读取:
凡人修仙传...（正常显示）

使用 gbk 编码读取:
乱码...
```

### 步骤 2：转换为正确的 GBK 编码

如果文件不是 GBK，转换它：

```bash
# 自动检测并转换为 GBK
python3 tools/check_encoding.py fanrenxiuxianchuan.txt --convert

# 输出: fanrenxiuxianchuan_gbk.txt
```

### 步骤 3：验证转换结果

```bash
# 再次检查编码
python3 tools/check_encoding.py fanrenxiuxianchuan_gbk.txt

# 应该显示:
# 检测到的编码: GB2312 或 GBK
```

### 步骤 4：使用 Java 转换工具

如果你有 Java 转换工具：

```bash
cd tools/tuya-converter
./build.sh  # 如果还没构建

# 转换文本文件
java -jar target/tuya-converter.jar fanrenxiuxianchuan.txt

# 输出: fanrenxiuxianchuan_tuya.txt (GBK 编码)
```

## 手动转换方法

### 方法 1：使用 iconv（Linux/macOS）

```bash
# UTF-8 转 GBK
iconv -f UTF-8 -t GBK fanrenxiuxianchuan.txt > fanrenxiuxianchuan_gbk.txt

# 验证
file fanrenxiuxianchuan_gbk.txt
```

### 方法 2：使用 Python

```python
# 读取 UTF-8，写入 GBK
with open('fanrenxiuxianchuan.txt', 'r', encoding='utf-8') as f:
    content = f.read()

with open('fanrenxiuxianchuan_gbk.txt', 'w', encoding='gbk') as f:
    f.write(content)
```

### 方法 3：使用文本编辑器

**VS Code**:
1. 打开文件
2. 右下角点击编码（如 UTF-8）
3. 选择 "Save with Encoding"
4. 选择 "Chinese (Simplified, GB2312)" 或 "GBK"

**Notepad++**:
1. 打开文件
2. 编码 → 转为 ANSI（GBK）
3. 保存

## 常见编码识别

### UTF-8 特征
```
文件开头: EF BB BF (BOM)
中文字符: E4 B8 AD E6 96 87 (3字节)
```

### GBK 特征
```
无 BOM
中文字符: D6 D0 CE C4 (2字节)
```

### 检查文件头

```bash
# 查看文件前16字节（十六进制）
hexdump -C fanrenxiuxianchuan.txt | head -1

# UTF-8 with BOM: ef bb bf ...
# UTF-8 no BOM: e4 b8 ad ...
# GBK: d6 d0 ce ...
```

## 快速测试

创建一个测试文件：

```bash
# 创建 GBK 测试文件
echo "测试中文" | iconv -f UTF-8 -t GBK > test_gbk.txt

# 复制到 SD 卡
cp test_gbk.txt /sdcard/

# 在设备上测试
# 如果这个文件显示正常，说明系统支持 GBK
# 如果显示乱码，说明可能是系统问题
```

## 系统端检查

### 检查 HZK24 字体是否正确加载

查看日志中是否有字体加载信息：

```
[01-01 00:00:00 ty N] HZK24 font loaded successfully
```

如果没有，检查：
1. `lib/Fonts/hzk24.c` 是否包含完整字体数据
2. 编译时是否包含了字体文件

### 检查字符绘制代码

在 `GUI_Paint.c` 中，`Paint_DrawString_CN` 函数应该：
1. 正确识别 GBK 双字节字符
2. 正确计算字符在 HZK24 中的偏移
3. 正确绘制字模

## 调试输出

在 `EPD_4in26_network_novel.c` 中添加调试输出：

```c
// 在显示文本前添加
PR_DEBUG("First 20 bytes (hex):");
for (int i = 0; i < 20 && i < content_size; i++) {
    printf("%02X ", (unsigned char)content[i]);
}
printf("\n");

// 检查是否是 GBK
if ((unsigned char)content[0] >= 0x81 && (unsigned char)content[0] <= 0xFE) {
    PR_DEBUG("Looks like GBK encoding");
} else if ((unsigned char)content[0] == 0xEF && 
           (unsigned char)content[1] == 0xBB && 
           (unsigned char)content[2] == 0xBF) {
    PR_DEBUG("UTF-8 with BOM detected!");
} else {
    PR_DEBUG("Unknown encoding");
}
```

## 解决方案总结

### 最可能的问题：文件不是真正的 GBK

**解决方法**：
```bash
# 1. 检查编码
python3 tools/check_encoding.py fanrenxiuxianchuan.txt

# 2. 转换为 GBK
python3 tools/check_encoding.py fanrenxiuxianchuan.txt --convert

# 3. 使用转换后的文件
cp fanrenxiuxianchuan_gbk.txt /sdcard/fanrenxiuxianchuan.txt

# 4. 重新测试
```

### 如果转换后仍然乱码

1. **检查 HZK24 字体**
   ```bash
   # 确保使用完整字体
   ./build_with_full_font.sh
   ```

2. **检查字符范围**
   - GBK 范围：0x8140-0xFEFE
   - 确保文本中的字符都在此范围内

3. **检查系统日志**
   - 查看是否有字体加载错误
   - 查看是否有字符绘制错误

## 预防措施

### 使用 Java 转换工具

始终使用 Java 转换工具处理文本文件：

```bash
# 转换所有文本文件
for f in *.txt; do
    java -jar tuya-converter/target/tuya-converter.jar "$f"
done

# 只使用 *_tuya.txt 文件
cp *_tuya.txt /sdcard/
```

### 验证文件

在复制到 SD 卡前验证：

```bash
# 检查编码
file *.txt

# 应该显示:
# xxx_tuya.txt: ISO-8859 text (GBK)
```

## 常见错误

### 错误 1：UTF-8 文件当作 GBK

**现象**：每个中文字符显示为 3 个乱码字符

**原因**：UTF-8 中文是 3 字节，GBK 是 2 字节

**解决**：转换为 GBK

### 错误 2：UTF-8 with BOM

**现象**：第一个字符是乱码，后面正常或全部乱码

**原因**：BOM (EF BB BF) 被当作字符

**解决**：移除 BOM 或转换为 GBK

### 错误 3：GB2312 vs GBK

**现象**：部分生僻字显示为方框

**原因**：GB2312 字符集小于 GBK

**解决**：使用 GBK 或 GB18030

## 工具汇总

1. **check_encoding.py** - 检测和转换编码
2. **tuya-converter.jar** - Java 转换工具
3. **iconv** - Linux/macOS 命令行工具
4. **hexdump** - 查看文件十六进制内容
5. **file** - 检测文件类型和编码

## 快速参考

```bash
# 检查编码
python3 tools/check_encoding.py file.txt

# 转换为 GBK
python3 tools/check_encoding.py file.txt --convert

# 或使用 Java 工具
java -jar tuya-converter/target/tuya-converter.jar file.txt

# 验证
file file_gbk.txt
hexdump -C file_gbk.txt | head -1

# 复制到 SD 卡
cp file_gbk.txt /sdcard/
```

---

**最后更新**：2025-12-25
