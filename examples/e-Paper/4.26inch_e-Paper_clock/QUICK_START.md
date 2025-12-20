# 快速开始指南

## 5 分钟快速上手

### 步骤 1：配置 WiFi（必须）⚠️

编辑 `examples/EPD_4in26_clock.c`，找到并修改：

```c
#define DEFAULT_WIFI_SSID "your_wifi_ssid"      // 改成你的 WiFi 名称
#define DEFAULT_WIFI_PSWD "your_wifi_password"  // 改成你的 WiFi 密码
```

### 步骤 2：编译

```bash
cd examples/e-Paper/4.26inch_e-Paper_clock
tos.py build
```

### 步骤 3：烧录

```bash
tos.py flash
```

### 步骤 4：查看运行

```bash
tos.py monitor
```

## 预期效果

### 串口输出
```
Network connected, syncing time...
Time synced successfully! Offset: 28800 seconds
Server time: Thu Dec 20 12:34:56 2024
Time: 2024-12-20 12:34:56 Thursday [Synced]
Time: 2024-12-20 12:34:57 Thursday [Synced]
...
```

### 屏幕显示
```
┌─────────────────────────────────┐
│ Network Clock                   │
│ Time Synced                     │
├─────────────────────────────────┤
│                                 │
│         2024-12-20              │
│                                 │
│          12:34:56               │  ← 大字体
│                                 │
│          Thursday               │
│                                 │
│ *Synced                         │
└─────────────────────────────────┘
```

## 常见问题

### Q: 显示 "Using Local Time"？
**A**: WiFi 未连接或时间同步失败
- 检查 WiFi 配置是否正确
- 确保设备在 WiFi 信号范围内
- 查看串口输出的错误信息

### Q: 编译失败？
**A**: 检查依赖和环境
```bash
source export.sh  # 激活环境
tos.py check      # 检查依赖
```

### Q: 时间不对？
**A**: 检查时区设置
```c
// 修改时区
#define TIME_SERVER_PATH "/api/timezone/Asia/Shanghai"
```

## 下一步

- 📖 阅读 [README_CN.md](./README_CN.md) 了解详细功能
- 🔧 查看 [CHANGELOG.md](./CHANGELOG.md) 了解更新内容
- 💡 参考 [VERSION_2.0_SUMMARY.md](./VERSION_2.0_SUMMARY.md) 了解技术细节

## 需要帮助？

- 查看串口输出的详细日志
- 阅读故障排除部分（README_CN.md）
- 检查硬件连接

---

**提示**：首次运行可能需要 30 秒等待网络连接和时间同步！
