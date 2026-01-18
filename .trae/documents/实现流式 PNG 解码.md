## 目标
- 在 T5AI 上支持“大 PNG”预览，不再依赖 lodepng 的整图解压/整图缓存（避免 5MB+ 连续内存需求）。
- 直接边解码边绘制到 1bit 墨水屏缓冲，峰值内存维持在“2 条扫描线 + 少量解压窗口”。

## 现状与根因（基于你提供的日志）
- lodepng 解码会 `realloc ~ (w*4+1)*h` 的整图扫描线缓冲（你例子 1434×914 触发 5,243,618 bytes），即便最终输出 1bpp 也无法避免。
- 系统 heap 很小（~200KB），大图必须走 PSRAM，但 PSRAM 也会因为“需要一块连续 5.2MB”而失败。

## 技术方案（流式解码）
- 新增一个“专用 PNG 流式解码器”，不再使用 lodepng。
- 支持：
  - PNG 基础块解析：`IHDR/IDAT/IEND`（可选支持 `PLTE/tRNS`）
  - 非交错（interlace=0）
  - 8-bit 深度（bitdepth=8）
  - 颜色类型：优先支持 `RGB(2)`、`RGBA(6)`、`GRAY(0)`；若你需要再扩展 `PALETTE(3)`。
- 解码流程：
  1) 读取并解析 IHDR 得到宽高、色彩类型、bitdepth。
  2) 计算绘制区域：复用现有 `fit_aspect`，确定 draw_w/draw_h/off_x/off_y。
  3) 顺序读取 IDAT（可跨多个 chunk），将压缩数据喂给“流式 zlib/deflate 解压器”。
  4) 解压输出按 PNG 规范产出每行：`filter_byte + scanline_bytes`。
  5) 对每行做 unfilter（filter 0~4），仅保留两行缓冲：`prev_row` + `cur_row`。
  6) 将当前行按最近邻缩放映射到目标行，逐像素转换为灰度/黑白并 `Paint_SetPixel` 输出。

## deflate/zlib 实现选择
- 引入一个小型、可嵌入、可“增量输出”的 inflate 实现（例如 miniz 的 tinfl 或等价 permissive 代码），实现 `inflate_push(input_chunk, on_output_bytes)` 回调式输出。
- 只需要解压（inflate），不需要压缩（deflate）。

## 代码落点与改动范围
- 新增：`examples/peripherals/sd/src/png_stream_decoder.{c,h}`（PNG 解析 + unfilter + 缩放 + 绘制）。
- 新增：`examples/peripherals/sd/src/third_party/<tiny_inflate>/*`（流式 inflate 代码，带许可证文件）。
- 修改：`examples/peripherals/sd/src/sd_image_view.c`
  - `draw_png_1bit()` 改为优先走流式解码；
  - 对不支持的 PNG（交错、非 8bit、未知 color type）可选择：返回错误或回退到现有 lodepng（小图）。

## 日志与可观测性
- 保留你现有的 heap/psram 水位日志；
- 额外打印：IHDR 信息（w/h/ct/bitdepth/interlace）、每行 filter 类型统计、inflate 错误码。

## 验证方式
- 用你当前失败的 1434×914 PNG 复测：确认不再出现 5MB realloc，解码成功并能显示。
- 用小 PNG / PDF 转出的页面图（建议先用 JPG）对照：确认 UI 流程不受影响。
- 压测：连续翻页/切换目录，观察 heap/psram 水位是否稳定、无泄漏。

## 风险与限制
- 第一版会明确限制：仅支持非交错 + 8bit + 常见色彩类型；
- 如果你的 PNG 来自 PDF 渲染，通常满足这些条件（RGBA8/RGB8），命中率高；
- 若遇到调色板/灰度低位深/交错 PNG，再补齐对应路径。

## 交付物
- 可在 PSRAM 仍有余量但无法提供 5MB 连续块时，依然能解码大 PNG。
- 仍建议 PDF 页面优先转 JPG（你已要求且脚本会同步保持为 JPG）。

确认后我将开始落代码并提交，优先先跑通：RGBA8/RGB8 非交错 PNG → 1bit 绘制。