我将按照以下步骤为 SD 卡示例项目添加网络时间同步功能：

1.  **修改配置文件 (`app_default.config`)**
    *   添加必要的网络和 LWIP 配置项，以支持 WiFi 连接和 HTTP 请求。这些配置将参考 `4.26inch_e-Paper_clock` 项目。

2.  **修改源代码 (`examples/peripherals/sd/src/example_sd.c`)**
    *   **添加头文件**：引入 `http_client_interface.h`, `netmgr.h`, `netconn_wifi.h`, `time.h` 等必要的头文件。
    *   **添加宏定义**：定义时间服务器 URL (`www.baidu.com`)、WiFi SSID/密码配置（默认使用 "1519"）以及 HTTP 超时时间。
    *   **添加辅助函数**：
        *   `parse_http_date`: 用于解析 HTTP 响应头中的时间字符串。
        *   `sync_time_from_http`: 发起 HTTP GET 请求并同步系统时间。
        *   `link_status_callback`: 监听网络连接状态，在连接成功时触发时间同步。
    *   **修改 `__example_sd_task` 函数**：
        *   在任务开始处（文件系统挂载之后，主循环之前）添加网络初始化代码。
        *   初始化基础服务：`tal_kv`, `tal_sw_timer`, `tal_workq`, `tuya_tls`, `tuya_register_center`。
        *   初始化 `netmgr` 并连接 WiFi。
        *   添加一个等待循环，阻塞等待时间同步成功（最多等待 15 秒），并在日志中输出同步状态。

这样，当 App 启动时，会优先尝试连接网络并同步时间，完成后再进入 SD 卡文件浏览界面。