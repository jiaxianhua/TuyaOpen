# 屏幕旋转机制详解 / Screen Rotation Mechanism

## 概述

4.26 英寸电子墨水屏的物理分辨率是 **800x480**（横屏），但通过软件旋转可以实现 **480x800**（竖屏）显示。

## 旋转原理

### 1. 初始化设置

```c
// 物理屏幕：800x480（横屏）
#define DISPLAY_WIDTH 800
#define DISPLAY_HEIGHT 480

// 创建图像缓冲区，并设置旋转 90 度
Paint_NewImage(g_image_buffer, DISPLAY_WIDTH, DISPLAY_HEIGHT, ROTATE_90, WHITE);
```

### 2. 坐标转换

当设置 `ROTATE_90` 时，`Paint_SetPixel` 函数会自动进行坐标转换：

```c
void Paint_SetPixel(UWORD Xpoint, UWORD Ypoint, UWORD Color)
{
    UWORD X, Y;
    
    switch(Paint.Rotate) {
    case 0:   // 不旋转（横屏）
        X = Xpoint;
        Y = Ypoint;  
        break;
        
    case 90:  // 旋转 90 度（竖屏）
        X = Paint.WidthMemory - Ypoint - 1;  // 800 - Y - 1
        Y = Xpoint;                           // X
        break;
        
    case 180: // 旋转 180 度
        X = Paint.WidthMemory - Xpoint - 1;
        Y = Paint.HeightMemory - Ypoint - 1;
        break;
        
    case 270: // 旋转 270 度
        X = Ypoint;
        Y = Paint.HeightMemory - Xpoint - 1;
        break;
    }
    
    // 然后将像素写入缓冲区
    UDOUBLE Addr = X / 8 + Y * Paint.WidthByte;
    // ...
}
```

### 3. 逻辑尺寸 vs 物理尺寸

```c
// 初始化后
if(Rotate == ROTATE_0 || Rotate == ROTATE_180) {
    Paint.Width = Width;      // 800
    Paint.Height = Height;    // 480
} else {  // ROTATE_90 or ROTATE_270
    Paint.Width = Height;     // 480 (逻辑宽度)
    Paint.Height = Width;     // 800 (逻辑高度)
}
```

## 坐标转换示例

### ROTATE_90 (竖屏模式)

假设我们要在逻辑坐标 `(100, 200)` 处画一个点：

```
逻辑坐标（竖屏）：
- X = 100
- Y = 200
- 逻辑尺寸：480x800

转换公式：
- 物理 X = 800 - 200 - 1 = 599
- 物理 Y = 100
- 物理尺寸：800x480

结果：
逻辑 (100, 200) → 物理 (599, 100)
```

### 可视化理解

```
物理屏幕（800x480，横屏）：
┌─────────────────────────────────┐
│                                 │ 480
│         800 x 480               │
│                                 │
└─────────────────────────────────┘
            800

旋转 90 度后（逻辑 480x800，竖屏）：
┌──────────┐
│          │
│          │
│          │
│  480x800 │ 800
│          │
│          │
│          │
└──────────┘
    480
```

## 文字旋转

### 中文字符（24x24 像素）

```c
// 绘制中文字符
draw_gbk_char24(x, y, gb_high, gb_low, BLACK, WHITE);

// 内部实现
for (int row = 0; row < 24; row++) {
    for (int col = 0; col < 24; col++) {
        // 每个像素都会通过 Paint_SetPixel 自动旋转
        Paint_SetPixel(x + col, y + row, color);
    }
}
```

### 英文字符（12x24 像素）

```c
// 绘制英文字符
Paint_DrawString_EN(x, y, "Hello", &Font24, BLACK, WHITE);

// Font24 定义
// Width: 12 pixels
// Height: 24 pixels

// 每个字符的像素也会自动旋转
```

### 旋转效果

```
不旋转（ROTATE_0）：
┌────┐
│ 中 │ 24x24
└────┘
  24

旋转 90 度（ROTATE_90）：
┌────┐
│    │
│ 中 │ 24x24（自动旋转）
│    │
└────┘
  24

实际上，字符本身不需要旋转，
因为每个像素的坐标都会被 Paint_SetPixel 自动转换！
```

## 内存布局

### 缓冲区大小

```c
// 物理尺寸：800x480
// 每个像素 1 bit（黑白）
// 每字节 8 个像素

UWORD image_size = ((800 % 8 == 0) ? (800 / 8) : (800 / 8 + 1)) * 480
                 = 100 * 480
                 = 48000 字节
                 = 48 KB
```

### 像素寻址

```c
// 对于物理坐标 (X, Y)
UWORD WidthByte = 800 / 8 = 100;  // 每行 100 字节
UDOUBLE Addr = X / 8 + Y * WidthByte;

// 像素在字节中的位置
UBYTE bit_position = X % 8;

// 设置像素
if (Color == BLACK)
    Paint.Image[Addr] = Rdata & ~(0x80 >> bit_position);
else
    Paint.Image[Addr] = Rdata | (0x80 >> bit_position);
```

## 实际应用

### 当前项目配置

```c
// 主程序中
Paint_NewImage(g_image_buffer, 800, 480, ROTATE_90, WHITE);

// 结果：
// - 逻辑尺寸：480x800（竖屏）
// - 物理尺寸：800x480（横屏）
// - 所有绘图操作使用逻辑坐标
// - 自动转换为物理坐标
```

### 文本显示参数

```c
// 基于逻辑尺寸（480x800）
#define CHARS_PER_LINE 37   // 字符单位数（不是像素！）
#define LINES_PER_PAGE 32   // 800 / 24 ≈ 33 行

// 字符单位系统：
// - ASCII 字符 = 1 单位 = 12 像素宽
// - GBK 字符 = 2 单位 = 24 像素宽

// 实际显示能力：
// - 纯中文：37 / 2 ≈ 18 个中文字符（432 像素）
// - 纯英文：37 个英文字符（444 像素）
// - 混合文本：例如 10 个中文 + 17 个英文 = 37 单位
// - 每页最多：32 行
```

**重要**：`CHARS_PER_LINE` 不是像素数，而是字符单位数！这样设计是为了支持中英文混排。

详细说明请参考：[CHAR_COUNTING_EXPLAINED.md](CHAR_COUNTING_EXPLAINED.md)

## 旋转模式对比

| 模式 | 角度 | 逻辑尺寸 | 物理尺寸 | 用途 |
|------|------|----------|----------|------|
| ROTATE_0 | 0° | 800x480 | 800x480 | 横屏显示 |
| ROTATE_90 | 90° | 480x800 | 800x480 | 竖屏显示（当前） |
| ROTATE_180 | 180° | 800x480 | 800x480 | 倒置横屏 |
| ROTATE_270 | 270° | 480x800 | 800x480 | 倒置竖屏 |

## 坐标转换公式总结

### ROTATE_0 (0°)
```c
物理X = 逻辑X
物理Y = 逻辑Y
```

### ROTATE_90 (90°) - 当前使用
```c
物理X = WidthMemory - 逻辑Y - 1  // 800 - Y - 1
物理Y = 逻辑X
```

### ROTATE_180 (180°)
```c
物理X = WidthMemory - 逻辑X - 1
物理Y = HeightMemory - 逻辑Y - 1
```

### ROTATE_270 (270°)
```c
物理X = 逻辑Y
物理Y = HeightMemory - 逻辑X - 1
```

## 性能考虑

### 优点
1. **透明性** - 应用层只需使用逻辑坐标
2. **灵活性** - 可以轻松切换旋转模式
3. **兼容性** - 所有绘图函数自动支持旋转

### 开销
1. **计算开销** - 每个像素都需要坐标转换
2. **内存访问** - 旋转后的内存访问模式可能不连续

但对于电子墨水屏（刷新慢），这些开销可以忽略不计。

## 如何修改旋转

### 改为横屏显示

```c
// 修改初始化
Paint_NewImage(g_image_buffer, DISPLAY_WIDTH, DISPLAY_HEIGHT, ROTATE_0, WHITE);

// 调整显示参数
#define CHARS_PER_LINE 66   // 800 / 12 ≈ 66 个英文字符
#define LINES_PER_PAGE 20   // 480 / 24 = 20 行
```

### 改为倒置竖屏

```c
// 修改初始化
Paint_NewImage(g_image_buffer, DISPLAY_WIDTH, DISPLAY_HEIGHT, ROTATE_270, WHITE);

// 显示参数保持不变（仍然是 480x800）
```

## 调试技巧

### 验证坐标转换

```c
// 测试代码
void test_rotation() {
    // 画四个角的点
    Paint_SetPixel(0, 0, BLACK);           // 左上角
    Paint_SetPixel(479, 0, BLACK);         // 右上角
    Paint_SetPixel(0, 799, BLACK);         // 左下角
    Paint_SetPixel(479, 799, BLACK);       // 右下角
    
    // 画中心点
    Paint_SetPixel(240, 400, BLACK);       // 中心
    
    EPD_4in26_Display(g_image_buffer);
}
```

### 日志输出

```c
PR_DEBUG("Logical: (%d, %d) -> Physical: (%d, %d)", 
         logical_x, logical_y, physical_x, physical_y);
```

## 总结

1. **物理屏幕**: 800x480（横屏）
2. **旋转设置**: ROTATE_90
3. **逻辑尺寸**: 480x800（竖屏）
4. **自动转换**: 所有绘图操作的坐标都会自动转换
5. **透明实现**: 应用层无需关心物理坐标

这就是为什么我们可以在 800x480 的横屏上实现 480x800 的竖屏显示！

---

**关键点**: 旋转是在像素级别实现的，每次调用 `Paint_SetPixel` 时都会进行坐标转换，因此文字、图片等所有内容都会自动旋转。
