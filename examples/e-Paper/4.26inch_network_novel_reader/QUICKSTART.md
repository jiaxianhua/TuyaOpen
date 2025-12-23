# 快速开始指南

5分钟快速上手墨水屏网络小说阅读器。

## 前提条件

- ✅ TuyaOpen 开发环境已安装
- ✅ T5AI 开发板 + 4.26寸墨水屏
- ✅ WiFi 网络（2.4GHz）
- ✅ 1个按钮已连接

## 步骤 1：修改配置（2分钟）

编辑 `examples/EPD_4in26_network_novel.c`：

```c
// 第 1 步：修改 WiFi 配置
#define WIFI_SSID "你的WiFi名称"
#define WIFI_PASSWORD "你的WiFi密码"

// 第 2 步：确认小说 URL（默认已配置好）
#define NOVEL_URL "http://120.79.89.230/fanren.txt"
```

**就这么简单！** 其他配置都已经设置好了。

## 步骤 2：编译和烧录（2分钟）

```bash
# 激活环境
source export.sh

# 进入项目目录
cd examples/e-Paper/4.26inch_network_novel_reader

# 配置（首次运行）
tos.py config
# 选择：T5AI -> T5AI_BOARD_EX_MODULE_NONE

# 编译
tos.py build

# 烧录
tos.py flash
```

## 步骤 3：使用（1分钟）

1. **上电**：设备自动启动
2. **等待**：屏幕显示 "Connecting to WiFi..."
3. **连接成功**：显示 "WiFi Connected! Press button to load novel"
4. **按按钮**：开始下载小说
5. **阅读**：
   - **短按** = 下一页
   - **长按（3秒）** = 上一页

## 完成！🎉

你的墨水屏小说阅读器已经可以使用了！

---

## 常见问题速查

### WiFi 连接失败？

```bash
# 查看日志
tos.py monitor

# 检查：
# 1. SSID 和密码是否正确
# 2. WiFi 是 2.4GHz（不是 5GHz）
# 3. 信号强度是否足够
```

### 按钮没反应？

检查 `lib/Config/board_button_config.h`：

```c
#define BOARD_BUTTON_PIN TY_GPIOA_6  // 确认 GPIO 引脚
#define BOARD_BUTTON_ACTIVE_LV TUYA_GPIO_LEVEL_LOW  // 确认电平
```

### 想用自己的小说？

1. **准备 GBK 编码的文本文件**：

```bash
# UTF-8 转 GBK
iconv -f UTF-8 -t GBK my_novel.txt -o my_novel_gbk.txt
```

2. **上传到 HTTP 服务器**（不支持 HTTPS）

3. **修改 URL**：

```c
#define NOVEL_URL "http://your-server.com/my_novel_gbk.txt"
```

4. **重新编译和烧录**

### 想用内嵌小说（无需网络）？

```bash
# 转换文本为 C 数组
python tools/txt_to_c_array.py your_novel.txt

# 重新编译
tos.py build
tos.py flash
```

---

## 下一步

- 📖 阅读 [README_CN.md](README_CN.md) 了解详细功能
- 🔧 阅读 [GBK_SUPPORT.md](GBK_SUPPORT.md) 了解 GBK 编码
- 🎨 阅读 [BUTTON_SETUP.md](BUTTON_SETUP.md) 了解按钮配置

## 技术支持

遇到问题？

1. 查看日志：`tos.py monitor`
2. 检查硬件连接
3. 参考文档：`README_CN.md`
4. 查看示例代码注释

---

**享受阅读！** 📚
