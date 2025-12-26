# Design Document - Network File Browser

## Overview

本设计为电子墨水屏阅读器添加网络文件浏览功能，支持从 HTTP 服务器获取文件列表、显示 3x3 网格缩略图、下载文件到 SD 卡，并能在网络模式和 SD 卡模式之间切换。

设计重点：
- 内存高效的流式处理（170KB 限制）
- UTF-8 到 GBK 编码转换
- 响应式网格布局
- 实时下载进度显示

## Architecture

### 系统架构

```
┌─────────────────────────────────────────────────────────┐
│                    Main Application                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ Mode Manager │  │Button Handler│  │Display Engine│  │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  │
│         │                 │                  │          │
│  ┌──────▼─────────────────▼──────────────────▼───────┐  │
│  │              Application State                     │  │
│  └────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
         │                                    │
    ┌────▼────────┐                    ┌─────▼──────────┐
    │  Network    │                    │   SD Card      │
    │  Browser    │                    │   Browser      │
    │  Module     │                    │   Module       │
    └────┬────────┘                    └─────┬──────────┘
         │                                    │
    ┌────▼────────────────────────────────────▼──────────┐
    │              File Manager Interface                │
    └────────────────────────────────────────────────────┘
         │                                    │
    ┌────▼────────┐                    ┌─────▼──────────┐
    │   HTTP      │                    │   SD Card      │
    │   Client    │                    │   File I/O     │
    └────┬────────┘                    └────────────────┘
         │
    ┌────▼────────┐
    │   JSON      │
    │   Parser    │
    └─────────────┘
```

### 模块职责

1. **Mode Manager**: 管理网络/SD卡模式切换
2. **Network Browser**: 网络文件列表获取、网格显示、下载
3. **SD Card Browser**: SD 卡文件列表显示（已有）
4. **HTTP Client**: HTTP 请求处理（已有）
5. **JSON Parser**: cJSON 解析文件列表
6. **Encoding Converter**: UTF-8 到 GBK 转换
7. **Grid Renderer**: 3x3 网格布局渲染
8. **Download Manager**: 文件下载和进度管理

## Components and Interfaces

### 1. Network File Info Structure

```c
#define MAX_FILENAME_LEN 128
#define MAX_URL_LEN 256
#define MAX_NETWORK_FILES 100

typedef struct {
    char filename[MAX_FILENAME_LEN];           // 服务器文件名
    char original_filename[MAX_FILENAME_LEN];  // 原始文件名（UTF-8）
    char display_name[MAX_FILENAME_LEN];       // 显示名称（GBK）
    char url[MAX_URL_LEN];                     // 文件 URL
    char thumbnail_url[MAX_URL_LEN];           // 缩略图 URL
    int size;                                  // 文件大小（字节）
    char filetype[32];                         // 文件类型
    UBYTE *thumbnail_data;                     // 缩略图数据（120x120）
    int thumbnail_loaded;                      // 缩略图是否已加载
} network_file_info_t;

typedef struct {
    network_file_info_t files[MAX_NETWORK_FILES];
    int file_count;
    int current_index;
    int current_page;        // 当前页（每页9个文件）
    int total_pages;         // 总页数
    int files_loaded;        // 文件列表是否已加载
} network_file_browser_t;
```

### 2. Mode Manager

```c
typedef enum {
    BROWSER_MODE_SD_CARD = 0,
    BROWSER_MODE_NETWORK,
    BROWSER_MODE_COUNT
} browser_mode_e;

typedef struct {
    browser_mode_e current_mode;
    int network_available;
    int sd_available;
} mode_manager_t;

// 初始化模式管理器
int mode_manager_init(mode_manager_t *mgr);

// 切换模式
int mode_manager_switch(mode_manager_t *mgr);

// 获取当前模式
browser_mode_e mode_manager_get_current(mode_manager_t *mgr);
```

### 3. Network Browser API

```c
// 初始化网络浏览器
int network_browser_init(network_file_browser_t *browser);

// 获取文件列表
int network_browser_fetch_list(network_file_browser_t *browser);

// 加载缩略图（单个）
int network_browser_load_thumbnail(network_file_info_t *file);

// 加载当前页所有缩略图
int network_browser_load_page_thumbnails(network_file_browser_t *browser);

// 释放缩略图内存
void network_browser_free_thumbnails(network_file_browser_t *browser, int page);

// 下载选中文件
int network_browser_download_file(network_file_info_t *file, 
                                   download_progress_cb_t progress_cb);

// 清理资源
void network_browser_cleanup(network_file_browser_t *browser);
```

### 4. Encoding Converter

```c
// UTF-8 到 GBK 转换
int utf8_to_gbk(const char *utf8_str, char *gbk_str, int gbk_len);

// GBK 到 UTF-8 转换（用于发送请求）
int gbk_to_utf8(const char *gbk_str, char *utf8_str, int utf8_len);

// 检测字符串编码
int detect_encoding(const char *str);
```

### 5. Grid Renderer

```c
#define GRID_ROWS 3
#define GRID_COLS 3
#define GRID_CELL_WIDTH 160
#define GRID_CELL_HEIGHT 160
#define THUMBNAIL_SIZE 120

typedef struct {
    int x;
    int y;
    int width;
    int height;
    int selected;
} grid_cell_t;

// 计算网格单元位置
void grid_calculate_cells(grid_cell_t cells[GRID_ROWS][GRID_COLS]);

// 渲染网格
void grid_render(network_file_browser_t *browser, grid_cell_t cells[GRID_ROWS][GRID_COLS]);

// 渲染单个单元
void grid_render_cell(network_file_info_t *file, grid_cell_t *cell, int selected);
```

### 6. Download Manager

```c
typedef void (*download_progress_cb_t)(int downloaded, int total, void *user_data);

typedef struct {
    char url[MAX_URL_LEN];
    char save_path[256];
    int total_size;
    int downloaded_size;
    int is_downloading;
    download_progress_cb_t progress_cb;
    void *user_data;
} download_context_t;

// 开始下载
int download_start(download_context_t *ctx);

// 取消下载
int download_cancel(download_context_t *ctx);

// 获取下载进度
int download_get_progress(download_context_t *ctx);
```

## Data Models

### Application State

```c
typedef struct {
    // 模式管理
    mode_manager_t mode_manager;
    
    // 浏览器
    network_file_browser_t network_browser;
    file_browser_t sd_browser;  // 已有
    
    // 下载状态
    download_context_t download_ctx;
    int is_downloading;
    
    // 按键状态
    volatile int click_count;
    volatile int long_press_detected;
    volatile TIME_T last_click_time;
    volatile TIME_T action_trigger_time;
    
    // 显示缓冲区
    UBYTE *image_buffer;
    
    // 网络状态
    int network_connected;
    int time_synced;
} app_state_t;
```

### JSON Response Model

```json
{
  "files": [
    {
      "filename": "string",
      "size": integer,
      "url": "string",
      "original_filename": "string",
      "filetype": "string",
      "upload_time": "string",
      "thumbnail": "string"
    }
  ]
}
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: 文件列表解析完整性

*For any* valid JSON response from the server, parsing should extract all file entries and populate the network_file_info_t array with correct field values.

**Validates: Requirements 1.2, 1.3**

### Property 2: UTF-8 到 GBK 转换正确性

*For any* UTF-8 encoded original_filename, the conversion to GBK should produce a valid GBK string that displays correctly with HZK fonts.

**Validates: Requirements 2.1, 2.3**

### Property 3: 网格索引映射

*For any* file index in range [0, file_count), the grid position calculation should map to a valid cell in [0, GRID_ROWS) x [0, GRID_COLS) and page number.

**Validates: Requirements 3.1, 3.2**

### Property 4: 缩略图内存管理

*For any* page switch operation, all thumbnails from the previous page should be freed before loading new thumbnails, ensuring total memory usage stays within limits.

**Validates: Requirements 9.2, 9.5**

### Property 5: 下载进度单调性

*For any* ongoing download, the downloaded_size should be monotonically increasing and never exceed total_size.

**Validates: Requirements 7.2, 7.3**

### Property 6: 模式切换状态一致性

*For any* mode switch operation, the system should save the current state, switch modes, and restore the appropriate state for the new mode.

**Validates: Requirements 8.2, 8.3, 8.4**

### Property 7: 按键导航循环性

*For any* file index, pressing next from the last file should wrap to the first file, and pressing previous from the first file should wrap to the last file.

**Validates: Requirements 5.3, 5.4**

### Property 8: 文件下载完整性

*For any* downloaded file, the saved file size should equal the Content-Length from the HTTP response.

**Validates: Requirements 6.3**

### Property 9: JSON 解析错误处理

*For any* malformed JSON response, the parser should return an error code without crashing and the system should fallback to SD card mode.

**Validates: Requirements 10.2**

### Property 10: 内存分配边界

*For any* memory allocation operation (thumbnails, JSON buffer, download buffer), the total allocated memory should not exceed available memory minus safety margin.

**Validates: Requirements 9.1, 9.3, 9.5**

## Error Handling

### Error Categories

1. **Network Errors**
   - Connection timeout → Retry 3 times, then fallback to SD mode
   - HTTP error codes → Display error message, stay in current mode
   - DNS resolution failure → Fallback to SD mode

2. **Parsing Errors**
   - Invalid JSON → Log error, fallback to SD mode
   - Missing required fields → Skip file entry, continue parsing
   - Encoding conversion failure → Use original filename

3. **Memory Errors**
   - Allocation failure → Free cached thumbnails, retry
   - Out of memory → Display warning, limit thumbnail loading
   - Buffer overflow → Truncate data, log warning

4. **Download Errors**
   - Connection interrupted → Save partial file, allow resume
   - Disk full → Display error, delete partial file
   - File already exists → Prompt user for overwrite

5. **Display Errors**
   - Thumbnail load failure → Show placeholder icon
   - Grid render failure → Fallback to list view
   - Font rendering error → Use ASCII fallback

### Error Recovery Strategy

```c
typedef enum {
    ERROR_SEVERITY_INFO,     // 继续执行
    ERROR_SEVERITY_WARNING,  // 降级功能
    ERROR_SEVERITY_ERROR,    // 回退到安全状态
    ERROR_SEVERITY_FATAL     // 重启应用
} error_severity_e;

typedef struct {
    error_severity_e severity;
    int error_code;
    const char *message;
    int (*recovery_handler)(void *ctx);
} error_info_t;

// 错误处理
int handle_error(error_info_t *error);
```

## Testing Strategy

### Unit Tests

1. **UTF-8 到 GBK 转换**
   - 测试纯 ASCII 字符串
   - 测试纯中文字符串
   - 测试混合字符串
   - 测试边界情况（空字符串、超长字符串）

2. **JSON 解析**
   - 测试完整 JSON 响应
   - 测试空文件列表
   - 测试缺失字段
   - 测试格式错误的 JSON

3. **网格计算**
   - 测试索引到网格位置映射
   - 测试分页计算
   - 测试边界索引

4. **下载进度**
   - 测试进度回调
   - 测试取消下载
   - 测试下载完成

### Property-Based Tests

每个 correctness property 都应该有对应的 property-based test，使用随机生成的输入验证属性。

配置：
- 最少 100 次迭代
- 使用 fast-check 或类似的 PBT 库
- 标签格式：**Feature: network-file-browser, Property {N}: {property_text}**

### Integration Tests

1. **端到端文件浏览**
   - 启动 → 获取列表 → 显示网格 → 导航 → 下载

2. **模式切换**
   - 网络模式 → SD 卡模式 → 网络模式

3. **错误恢复**
   - 网络断开 → 重连 → 继续操作

4. **内存压力测试**
   - 加载大量缩略图
   - 频繁切换页面
   - 监控内存使用

## Implementation Notes

### UTF-8 到 GBK 转换实现

由于嵌入式环境可能没有 iconv，需要实现简化的转换：

```c
// 使用查找表方式
typedef struct {
    uint32_t utf8_code;
    uint16_t gbk_code;
} utf8_gbk_map_t;

// 常用汉字映射表（约 6000 个常用字）
static const utf8_gbk_map_t utf8_gbk_table[] = {
    // ... 映射表数据
};

int utf8_to_gbk_simple(const char *utf8, char *gbk, int gbk_len) {
    // 1. 解析 UTF-8 字符
    // 2. 查找映射表
    // 3. 输出 GBK 编码
}
```

### 内存优化策略

1. **延迟加载缩略图**
   - 只加载当前页的 9 个缩略图
   - 切换页面时释放旧缩略图

2. **流式下载**
   - 使用 4KB 缓冲区分块下载
   - 边下载边写入 SD 卡

3. **JSON 解析优化**
   - 使用 cJSON_Parse 后立即提取数据
   - 提取完成后立即 cJSON_Delete

4. **缩略图压缩**
   - 服务器端生成 120x120 单色 BMP
   - 客户端直接显示，无需缩放

### 网格布局计算

```
Screen: 480x800 (ROTATE_90)

Grid Layout (3x3):
┌────────────────────────────────────────────────┐
│  Title Bar (30px)                              │
├────────────────────────────────────────────────┤
│  ┌──────┐  ┌──────┐  ┌──────┐                 │
│  │ 160  │  │ 160  │  │ 160  │  Row 1          │
│  │  x   │  │  x   │  │  x   │                 │
│  │ 160  │  │ 160  │  │ 160  │                 │
│  └──────┘  └──────┘  └──────┘                 │
│  ┌──────┐  ┌──────┐  ┌──────┐                 │
│  │      │  │      │  │      │  Row 2          │
│  │      │  │      │  │      │                 │
│  │      │  │      │  │      │                 │
│  └──────┘  └──────┘  └──────┘                 │
│  ┌──────┐  ┌──────┐  ┌──────┐                 │
│  │      │  │      │  │      │  Row 3          │
│  │      │  │      │  │      │                 │
│  │      │  │      │  │      │                 │
│  └──────┘  └──────┘  └──────┘                 │
├────────────────────────────────────────────────┤
│  Status Bar (30px)                             │
└────────────────────────────────────────────────┘

Cell: 160x160 (含边距)
Thumbnail: 120x120 (居中)
Filename: 2 lines x Font16
```

### 下载进度显示

```
┌────────────────────────────────────────────────┐
│  Downloading: 文件名.txt                       │
│                                                │
│  ████████████████░░░░░░░░░░░░░░░░░░░░  45%    │
│                                                │
│  Downloaded: 2.3 MB / 5.1 MB                   │
│                                                │
│  Speed: 128 KB/s                               │
│                                                │
│  [Long press to cancel]                        │
└────────────────────────────────────────────────┘
```

### API 调用序列

```
1. 启动
   ↓
2. 检查网络
   ↓
3. GET /files → JSON
   ↓
4. 解析 JSON → network_file_info_t[]
   ↓
5. UTF-8 → GBK 转换文件名
   ↓
6. 显示网格（第1页）
   ↓
7. 并行加载 9 个缩略图
   ↓
8. 更新显示
   ↓
9. 等待按键
   ↓
10. 长按 → 下载文件
    ↓
11. GET /files/{filename}
    ↓
12. 流式下载 + 进度更新
    ↓
13. 保存到 /sdcard/
    ↓
14. 显示完成消息
```

## Performance Considerations

### 响应时间目标

- 文件列表获取: < 3 秒
- 单个缩略图加载: < 1 秒
- 页面切换: < 2 秒
- 文件下载: 取决于文件大小和网速
- 模式切换: < 1 秒

### 内存使用目标

- 文件列表: ~20 KB (100 files)
- 缩略图缓存: ~135 KB (9 thumbnails)
- JSON 缓冲区: ~10 KB (临时)
- 下载缓冲区: 4 KB (流式)
- 总计: ~170 KB (接近限制)

### 优化建议

1. 使用 EPD_4in26_Display_Fast() 加快刷新
2. 缩略图使用 1-bit BMP 减少内存
3. 实现缩略图 LRU 缓存
4. 下载时显示简化界面节省内存
