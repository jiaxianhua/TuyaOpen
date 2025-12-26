# Requirements Document - Network File Browser

## Introduction

为电子墨水屏阅读器添加网络文件浏览功能，支持从服务器获取文件列表、显示缩略图、下载文件，并能在网络模式和 SD 卡模式之间切换。

## Glossary

- **System**: 电子墨水屏阅读器系统
- **Network_Browser**: 网络文件浏览器模块
- **SD_Browser**: SD 卡文件浏览器模块
- **File_Server**: HTTP 文件服务器 (http://120.79.89.230:8001)
- **Grid_View**: 3x3 网格视图显示
- **Thumbnail**: 缩略图 (120x120 像素)
- **Download_Progress**: 文件下载进度显示

## Requirements

### Requirement 1: 网络文件列表获取

**User Story:** 作为用户，我希望能从服务器获取文件列表，以便浏览可用的网络文件。

#### Acceptance Criteria

1. WHEN 系统启动且网络可用 THEN THE System SHALL 从 http://120.79.89.230:8001/files 获取文件列表
2. WHEN 接收到 JSON 响应 THEN THE System SHALL 解析 files 数组中的所有文件信息
3. WHEN 解析文件信息 THEN THE System SHALL 提取 filename, size, url, original_filename, filetype, thumbnail 字段
4. WHEN 文件列表为空 THEN THE System SHALL 显示 "No files available" 消息
5. WHEN 网络请求失败 THEN THE System SHALL 回退到 SD 卡模式

### Requirement 2: UTF-8 到 GBK 编码转换

**User Story:** 作为用户，我希望看到正确显示的中文文件名，以便识别文件内容。

#### Acceptance Criteria

1. WHEN 接收到 UTF-8 编码的 original_filename THEN THE System SHALL 转换为 GBK 编码
2. WHEN 转换失败 THEN THE System SHALL 使用原始 filename 作为备选
3. WHEN 显示文件名 THEN THE System SHALL 使用转换后的 GBK 字符串
4. WHEN 文件名超过显示宽度 THEN THE System SHALL 截断并添加省略号

### Requirement 3: 9宫格网格显示

**User Story:** 作为用户，我希望以网格方式查看文件，以便一次看到多个文件。

#### Acceptance Criteria

1. WHEN 显示网络文件列表 THEN THE System SHALL 使用 3x3 网格布局
2. WHEN 文件数量超过 9 个 THEN THE System SHALL 支持分页显示
3. WHEN 显示每个文件 THEN THE System SHALL 显示缩略图和文件名
4. WHEN 选中文件 THEN THE System SHALL 高亮显示该文件的网格单元
5. WHEN 网格单元为空 THEN THE System SHALL 显示空白占位符

### Requirement 4: 缩略图加载和显示

**User Story:** 作为用户，我希望看到文件的缩略图预览，以便快速识别文件内容。

#### Acceptance Criteria

1. WHEN 显示文件网格 THEN THE System SHALL 从服务器加载缩略图
2. WHEN 缩略图 URL 为 /files/thumbs/{filename} THEN THE System SHALL 使用完整 URL 下载
3. WHEN 缩略图下载成功 THEN THE System SHALL 在网格单元中显示
4. WHEN 缩略图下载失败 THEN THE System SHALL 显示默认占位图标
5. WHEN 缩略图为 BMP 格式 THEN THE System SHALL 缩放到网格单元大小

### Requirement 5: 网络文件浏览导航

**User Story:** 作为用户，我希望使用按键浏览网络文件，以便选择要下载的文件。

#### Acceptance Criteria

1. WHEN 用户单击按键 THEN THE System SHALL 移动到下一个文件
2. WHEN 用户双击按键 THEN THE System SHALL 移动到上一个文件
3. WHEN 到达最后一个文件且单击 THEN THE System SHALL 回到第一个文件
4. WHEN 到达第一个文件且双击 THEN THE System SHALL 跳到最后一个文件
5. WHEN 切换选中文件 THEN THE System SHALL 更新高亮显示

### Requirement 6: 文件下载功能

**User Story:** 作为用户，我希望下载选中的网络文件到 SD 卡，以便离线阅读。

#### Acceptance Criteria

1. WHEN 用户长按按键 THEN THE System SHALL 开始下载选中的文件
2. WHEN 下载进行中 THEN THE System SHALL 显示下载进度百分比
3. WHEN 下载完成 THEN THE System SHALL 保存文件到 /sdcard/ 目录
4. WHEN 下载失败 THEN THE System SHALL 显示错误消息
5. WHEN 文件已存在 THEN THE System SHALL 提示是否覆盖

### Requirement 7: 下载进度显示

**User Story:** 作为用户，我希望看到下载进度，以便了解下载状态。

#### Acceptance Criteria

1. WHEN 下载开始 THEN THE System SHALL 显示 "Downloading: {filename}" 消息
2. WHEN 接收数据 THEN THE System SHALL 更新进度条和百分比
3. WHEN 进度更新 THEN THE System SHALL 显示 "已下载/总大小" 信息
4. WHEN 下载完成 THEN THE System SHALL 显示 "Download complete" 消息 3 秒
5. WHEN 下载期间 THEN THE System SHALL 允许用户取消下载

### Requirement 8: 模式切换功能

**User Story:** 作为用户，我希望在网络模式和 SD 卡模式之间切换，以便访问不同来源的文件。

#### Acceptance Criteria

1. WHEN 系统启动 THEN THE System SHALL 根据网络状态选择默认模式
2. WHEN 用户触发模式切换 THEN THE System SHALL 在网络模式和 SD 卡模式之间切换
3. WHEN 切换到网络模式 THEN THE System SHALL 显示网络文件网格
4. WHEN 切换到 SD 卡模式 THEN THE System SHALL 显示 SD 卡文件列表
5. WHEN 无网络连接 THEN THE System SHALL 禁用切换到网络模式

### Requirement 9: 内存管理

**User Story:** 作为系统，我需要高效管理内存，以便在有限的 170KB 内存中运行。

#### Acceptance Criteria

1. WHEN 加载缩略图 THEN THE System SHALL 使用流式下载避免大内存分配
2. WHEN 切换页面 THEN THE System SHALL 释放不再使用的缩略图内存
3. WHEN 下载文件 THEN THE System SHALL 使用分块下载避免内存溢出
4. WHEN 解析 JSON THEN THE System SHALL 在解析后立即释放 JSON 对象
5. WHEN 内存不足 THEN THE System SHALL 显示错误并回退到安全状态

### Requirement 10: 错误处理

**User Story:** 作为用户，我希望系统能优雅处理错误，以便在出错时仍能继续使用。

#### Acceptance Criteria

1. WHEN 网络请求超时 THEN THE System SHALL 显示超时消息并重试
2. WHEN JSON 解析失败 THEN THE System SHALL 显示错误并回退到 SD 卡模式
3. WHEN 缩略图加载失败 THEN THE System SHALL 显示占位图标
4. WHEN SD 卡空间不足 THEN THE System SHALL 显示空间不足警告
5. WHEN 文件下载中断 THEN THE System SHALL 清理部分下载的文件

## Technical Notes

### API 端点

- **文件列表**: `GET http://120.79.89.230:8001/files`
- **单个文件**: `GET http://120.79.89.230:8001/files/{filename}`
- **缩略图**: `GET http://120.79.89.230:8001/files/thumbs/{filename}`

### JSON 响应格式

```json
{
  "files": [
    {
      "filename": "wallpaper.bmp",
      "size": 16262,
      "url": "/files/wallpaper.bmp",
      "original_filename": "壁纸.jpg",
      "filetype": "image/bmp",
      "upload_time": "2025-12-25T20:29:37.527957",
      "thumbnail": "/files/thumbs/wallpaper.bmp"
    }
  ]
}
```

### 网格布局

- 3x3 网格 = 9 个文件/页
- 每个单元: 约 160x160 像素（含边距）
- 缩略图: 120x120 像素
- 文件名: 最多 2 行，Font16

### 内存限制

- 可用内存: ~170 KB
- 单个缩略图: ~15 KB (120x120 BMP)
- 最多同时加载: 9 个缩略图 = ~135 KB
- 需要流式处理和及时释放

### 编码转换

- 输入: UTF-8 (JSON)
- 输出: GBK (显示)
- 使用 iconv 或自定义转换表

### 按键映射（网络模式）

- 单击: 下一个文件
- 双击: 上一个文件
- 长按: 下载文件
- 三击: 切换模式（新增）
