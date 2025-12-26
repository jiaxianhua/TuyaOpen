# Implementation Plan: Network File Browser

## Overview

实现网络文件浏览器功能，支持从 HTTP 服务器获取文件列表、3x3 网格显示、缩略图加载、文件下载和模式切换。采用增量开发方式，每个任务都可以独立测试。

## Tasks

- [x] 1. 创建基础数据结构和配置
  - 在 `EPD_4in26_network_novel.c` 中添加网络文件浏览器相关的宏定义
  - 定义 `network_file_info_t` 和 `network_file_browser_t` 结构体
  - 定义 `browser_mode_e` 和 `mode_manager_t` 结构体
  - 更新 `novel_reader_ctx_t` 添加网络浏览器和模式管理器字段
  - _Requirements: 1.1, 8.1_

- [x] 2. 实现 HTTP 文件列表获取
  - [x] 2.1 实现 `fetch_file_list()` 函数
    - 发送 GET 请求到 `http://120.79.89.230:8001/files`
    - 接收 JSON 响应
    - 返回 JSON 字符串
    - _Requirements: 1.1_

  - [x] 2.2 编写 property test for HTTP 请求
    - **Property 1: 文件列表解析完整性**
    - **Validates: Requirements 1.2, 1.3**

- [x] 3. 实现 JSON 解析
  - [x] 3.1 实现 `parse_file_list_json()` 函数
    - 使用 cJSON 解析 JSON 响应
    - 提取 files 数组
    - 遍历每个文件对象，提取字段
    - 填充 `network_file_info_t` 数组
    - 处理缺失字段的情况
    - _Requirements: 1.2, 1.3_

  - [x] 3.2 编写 unit tests for JSON 解析
    - 测试完整 JSON
    - 测试空文件列表
    - 测试缺失字段
    - 测试格式错误的 JSON
    - _Requirements: 1.2, 1.3, 10.2_

  - [x] 3.3 编写 property test for JSON 解析错误处理
    - **Property 9: JSON 解析错误处理**
    - **Validates: Requirements 10.2**

- [x] 4. 实现 UTF-8 到 GBK 编码转换
  - [x] 4.1 创建 UTF-8 到 GBK 映射表
    - 创建 `include/utf8_gbk_table.h` 文件
    - 定义 `utf8_gbk_map_t` 结构体
    - 添加常用汉字映射表（约 1000 个常用字）
    - _Requirements: 2.1_

  - [x] 4.2 实现 `utf8_to_gbk()` 函数
    - 解析 UTF-8 字符序列
    - 查找映射表
    - 输出 GBK 编码
    - 处理未找到的字符（使用 '?' 替代）
    - _Requirements: 2.1, 2.2_

  - [x] 4.3 编写 property test for UTF-8 到 GBK 转换
    - **Property 2: UTF-8 到 GBK 转换正确性**
    - **Validates: Requirements 2.1, 2.3**

  - [x] 4.4 编写 unit tests for 编码转换
    - 测试纯 ASCII
    - 测试纯中文
    - 测试混合字符串
    - 测试边界情况
    - _Requirements: 2.1, 2.2, 2.3_

- [x] 5. Checkpoint - 确保数据获取和解析正常
  - 确保所有测试通过，询问用户是否有问题

- [x] 6. 实现网格布局计算
  - [x] 6.1 实现 `grid_calculate_layout()` 函数
    - 计算 3x3 网格的单元位置
    - 每个单元 160x160 像素
    - 计算缩略图居中位置（120x120）
    - 计算文件名显示位置
    - _Requirements: 3.1_

  - [x] 6.2 实现 `grid_index_to_position()` 函数
    - 将文件索引映射到网格行列
    - 计算页码
    - 处理边界情况
    - _Requirements: 3.2_

  - [x] 6.3 编写 property test for 网格索引映射
    - **Property 3: 网格索引映射**
    - **Validates: Requirements 3.1, 3.2**

  - [x] 6.4 编写 unit tests for 网格计算
    - 测试索引到位置映射
    - 测试分页计算
    - 测试边界索引
    - _Requirements: 3.1, 3.2_

- [x] 7. 实现网格渲染
  - [x] 7.1 实现 `render_grid_cell()` 函数
    - 绘制单元边框
    - 显示缩略图（如果已加载）
    - 显示文件名（GBK，最多2行）
    - 高亮选中的单元
    - _Requirements: 3.3, 3.4_

  - [x] 7.2 实现 `display_network_file_grid()` 函数
    - 清空屏幕
    - 绘制标题栏（显示时间）
    - 计算当前页的文件范围
    - 遍历 3x3 网格，渲染每个单元
    - 绘制状态栏（页码、操作提示）
    - 更新显示
    - _Requirements: 3.1, 3.3, 3.4, 3.5_

  - [x] 7.3 编写 unit tests for 网格渲染
    - 测试单元渲染
    - 测试空单元
    - 测试选中状态
    - _Requirements: 3.3, 3.4, 3.5_

- [x] 8. 实现缩略图加载
  - [x] 8.1 实现 `load_thumbnail()` 函数
    - 构建缩略图 URL
    - 发送 HTTP GET 请求
    - 接收 BMP 数据
    - 分配内存存储缩略图
    - 解析 BMP 头部
    - 存储像素数据
    - _Requirements: 4.1, 4.2_

  - [x] 8.2 实现 `load_page_thumbnails()` 函数
    - 计算当前页的文件范围
    - 遍历当前页的 9 个文件
    - 并行或顺序加载缩略图
    - 处理加载失败的情况
    - _Requirements: 4.1, 4.3_

  - [x] 8.3 实现 `free_page_thumbnails()` 函数
    - 遍历指定页的文件
    - 释放缩略图内存
    - 标记为未加载
    - _Requirements: 9.2_

  - [x] 8.4 编写 property test for 缩略图内存管理
    - **Property 4: 缩略图内存管理**
    - **Validates: Requirements 9.2, 9.5**

  - [x] 8.5 编写 unit tests for 缩略图加载
    - 测试成功加载
    - 测试加载失败
    - 测试内存释放
    - _Requirements: 4.1, 4.2, 4.3, 4.4_

- [x] 9. Checkpoint - 确保网格显示正常
  - 确保所有测试通过，询问用户是否有问题

- [x] 10. 实现文件下载功能
  - [x] 10.1 实现 `download_file_with_progress()` 函数
    - 发送 HTTP GET 请求
    - 获取 Content-Length
    - 创建目标文件
    - 分块接收数据（4KB 缓冲区）
    - 每次接收后写入文件
    - 调用进度回调
    - 处理下载中断
    - _Requirements: 6.1, 6.2, 6.3_

  - [x] 10.2 实现 `display_download_progress()` 函数
    - 清空屏幕
    - 显示文件名
    - 绘制进度条
    - 显示百分比
    - 显示已下载/总大小
    - 显示取消提示
    - 更新显示（使用 Fast 模式）
    - _Requirements: 7.1, 7.2, 7.3, 7.4_

  - [x] 10.3 编写 property test for 下载进度单调性
    - **Property 5: 下载进度单调性**
    - **Validates: Requirements 7.2, 7.3**

  - [x] 10.4 编写 property test for 文件下载完整性
    - **Property 8: 文件下载完整性**
    - **Validates: Requirements 6.3**

  - [x] 10.5 编写 unit tests for 文件下载
    - 测试成功下载
    - 测试下载中断
    - 测试进度回调
    - 测试文件已存在
    - _Requirements: 6.1, 6.2, 6.3, 6.4, 6.5_

- [x] 11. 实现模式管理和切换
  - [x] 11.1 实现 `mode_manager_init()` 函数
    - 检测网络状态
    - 检测 SD 卡状态
    - 选择默认模式
    - _Requirements: 8.1_

  - [x] 11.2 实现 `mode_manager_switch()` 函数
    - 保存当前模式状态
    - 切换到另一个模式
    - 恢复新模式状态
    - 更新显示
    - _Requirements: 8.2, 8.3, 8.4_

  - [x] 11.3 编写 property test for 模式切换状态一致性
    - **Property 6: 模式切换状态一致性**
    - **Validates: Requirements 8.2, 8.3, 8.4**

  - [x] 11.4 编写 unit tests for 模式管理
    - 测试初始化
    - 测试切换
    - 测试无网络时禁用网络模式
    - _Requirements: 8.1, 8.2, 8.5_

- [x] 12. 实现按键处理（网络模式）
  - [x] 12.1 更新 `button_handler()` 函数
    - 检测三击（模式切换）
    - 保持单击、双击、长按逻辑
    - _Requirements: 5.1, 5.2, 8.2_

  - [x] 12.2 实现 `handle_network_browser_button()` 函数
    - 单击：下一个文件（循环）
    - 双击：上一个文件（循环）
    - 长按：下载文件
    - 三击：切换模式
    - 更新选中状态
    - 刷新显示
    - _Requirements: 5.1, 5.2, 5.3, 5.4, 5.5, 6.1_

  - [x] 12.3 编写 property test for 按键导航循环性
    - **Property 7: 按键导航循环性**
    - **Validates: Requirements 5.3, 5.4**

  - [x] 12.4 编写 unit tests for 按键处理
    - 测试单击导航
    - 测试双击导航
    - 测试长按下载
    - 测试三击切换
    - _Requirements: 5.1, 5.2, 5.3, 5.4, 6.1, 8.2_

- [x] 13. 集成到主程序
  - [x] 13.1 更新 `EPD_network_novel_test()` 主函数
    - 初始化模式管理器
    - 根据网络状态选择初始模式
    - 如果网络可用，获取文件列表
    - 显示网络文件网格或 SD 卡列表
    - 进入主循环
    - _Requirements: 8.1, 8.3, 8.4_

  - [x] 13.2 更新主循环处理网络模式
    - 检查当前模式
    - 如果是网络模式，调用 `handle_network_browser_button()`
    - 如果是 SD 卡模式，使用现有逻辑
    - _Requirements: 8.2, 8.3, 8.4_

  - [x] 13.3 编写 integration tests
    - 测试启动到网络模式
    - 测试启动到 SD 卡模式
    - 测试模式切换
    - 测试完整下载流程
    - _Requirements: 8.1, 8.2, 8.3, 8.4_

- [x] 14. 实现错误处理和恢复
  - [x] 14.1 实现网络错误处理
    - 连接超时重试
    - HTTP 错误码处理
    - 回退到 SD 卡模式
    - _Requirements: 10.1, 10.2_

  - [x] 14.2 实现内存错误处理
    - 分配失败时释放缓存
    - 内存不足警告
    - 限制缩略图加载
    - _Requirements: 9.5, 10.3_

  - [x] 14.3 实现下载错误处理
    - 连接中断处理
    - 磁盘空间不足
    - 文件已存在提示
    - _Requirements: 10.4, 10.5_

  - [x] 14.4 编写 property test for 内存分配边界
    - **Property 10: 内存分配边界**
    - **Validates: Requirements 9.1, 9.3, 9.5**

  - [x] 14.5 编写 unit tests for 错误处理
    - 测试网络超时
    - 测试内存不足
    - 测试磁盘空间不足
    - _Requirements: 10.1, 10.2, 10.3, 10.4, 10.5_

- [x] 15. Checkpoint - 确保所有功能正常
  - 确保所有测试通过，询问用户是否有问题

- [x] 16. 优化和性能调优
  - [x] 16.1 优化缩略图加载速度
    - 实现并行加载（如果可能）
    - 使用 Fast 刷新模式
    - 缓存策略优化
    - _Requirements: 9.1, 9.2_

  - [x] 16.2 优化内存使用
    - 减少临时缓冲区大小
    - 及时释放不用的资源
    - 监控内存峰值
    - _Requirements: 9.1, 9.2, 9.3, 9.5_

  - [x] 16.3 优化显示刷新
    - 使用局部刷新（如果支持）
    - 减少不必要的全屏刷新
    - 优化网格渲染顺序
    - _Requirements: 3.1, 3.3_

  - [x] 16.4 编写 performance tests
    - 测试文件列表获取时间
    - 测试缩略图加载时间
    - 测试页面切换时间
    - 测试内存使用峰值

- [x] 17. 创建文档和示例
  - [x] 17.1 创建 `NETWORK_BROWSER_GUIDE.md`
    - 功能说明
    - 使用方法
    - 按键操作
    - 配置选项
    - 故障排除

  - [x] 17.2 创建 `NETWORK_BROWSER_API.md`
    - API 端点说明
    - JSON 格式
    - 缩略图要求
    - 服务器配置

  - [x] 17.3 更新 `README.md`
    - 添加网络浏览器功能说明
    - 更新功能列表
    - 添加截图（如果有）

- [x] 18. Final checkpoint - 完整测试
  - 确保所有测试通过，询问用户是否有问题

## Notes

- 所有测试任务都是必需的，确保代码质量和正确性
- 每个 checkpoint 都是一个自然的停止点，可以进行测试和验证
- 建议按顺序执行任务，因为后面的任务依赖前面的实现
- UTF-8 到 GBK 转换是关键功能，需要仔细测试
- 内存管理非常重要，需要严格控制在 170KB 以内
- 使用 Fast 刷新模式可以提高响应速度
- 缩略图应该由服务器端生成为 120x120 单色 BMP，减少客户端处理

## Memory Budget

- 文件列表结构: ~20 KB
- 缩略图缓存 (9个): ~135 KB
- JSON 临时缓冲区: ~10 KB
- 下载缓冲区: 4 KB
- 其他: ~1 KB
- **总计: ~170 KB** (接近限制，需要严格管理)

## Testing Strategy

- Unit tests: 验证单个函数的正确性
- Property tests: 验证通用属性在所有输入下成立
- Integration tests: 验证模块间交互
- Performance tests: 验证响应时间和内存使用

每个 property test 应该：
- 运行至少 100 次迭代
- 使用随机生成的输入
- 标签格式：**Feature: network-file-browser, Property {N}: {property_text}**
