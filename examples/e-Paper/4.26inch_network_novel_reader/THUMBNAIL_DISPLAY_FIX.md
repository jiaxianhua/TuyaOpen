# Thumbnail Display Fix

## Problem
Thumbnails were downloading and saving successfully to SD card, but `GUI_ReadBmp()` failed to open them with error "Cann't open the file!". The program would hang after the first thumbnail download attempt.

## Root Cause
`GUI_ReadBmp()` in `lib/GUI/GUI_BMPfile.c` uses standard C library `fopen()` instead of TuyaOpen's `tkl_fopen()`. The standard `fopen()` cannot access files on the SD card filesystem, which requires TuyaOpen's filesystem APIs.

```c
// GUI_ReadBmp() - DOESN'T WORK with SD card
FILE *fp;
if((fp = fopen(path, "rb")) == NULL) {  // ❌ Standard fopen() fails
    Debug("Cann't open the file!\n");
    exit(0);
}
```

## Solution
Created a new function `sd_display_bmp_image_at()` that:
1. Uses `tkl_fopen()` to open files on SD card
2. Accepts X and Y position parameters for grid layout
3. Supports 1-bit, 24-bit, and 32-bit BMP formats
4. Handles both bottom-up and top-down BMP orientations

### Files Modified

#### 1. `src/sd_file_manager.c`
Added new function `sd_display_bmp_image_at()`:
- Uses `tkl_fopen()`, `tkl_fread()`, `tkl_fclose()` for SD card access
- Accepts `x_offset` and `y_offset` parameters
- Draws BMP at specified position using `Paint_SetPixel(x_offset + x, y_offset + y, color)`
- Supports monochrome (1-bit) and color (24/32-bit) BMPs with grayscale conversion

#### 2. `include/sd_file_manager.h`
Added function declaration:
```c
int sd_display_bmp_image_at(const char *filepath, int x_offset, int y_offset);
```

#### 3. `examples/EPD_4in26_network_novel.c`
Updated `load_and_draw_thumbnail()` to use new function:
```c
// OLD: Used GUI_ReadBmp() which doesn't work with SD card
UBYTE result = GUI_ReadBmp(temp_file, thumb_x, thumb_y);

// NEW: Uses sd_display_bmp_image_at() with tkl_fopen()
int result = sd_display_bmp_image_at(temp_file, thumb_x, thumb_y);
```

## How It Works

### Thumbnail Loading Flow
1. **Download**: HTTP client downloads thumbnail from server
2. **Save**: Write to `/sdcard/thumb_<filename>.bmp` using `tkl_fopen()`
3. **Display**: `sd_display_bmp_image_at()` reads BMP and draws at grid position
4. **Cleanup**: Delete temp file with `tkl_fs_remove()`

### Grid Positioning
- Grid: 3x3 cells, each 160x160 pixels
- Thumbnail: 120x120 pixels, centered in cell
- Position calculation:
  ```c
  int thumb_x = cell->x + (cell->width - THUMBNAIL_SIZE) / 2;
  int thumb_y = cell->y + 5;
  ```

### Error Handling
If thumbnail loading fails:
- `thumbnail_loaded` flag remains 0
- Placeholder drawn: gray box with "No Image" text
- Program continues to next thumbnail

## Testing
1. Build and flash updated code
2. Enter network browser mode
3. Verify thumbnails download and display in grid
4. Check that failed thumbnails show placeholder
5. Verify program doesn't hang

## Technical Details

### Why tkl_fopen() vs fopen()?
- **tkl_fopen()**: TuyaOpen filesystem API, works with SD card mounted via `tkl_fs_mount()`
- **fopen()**: Standard C library, only works with host filesystem (not SD card)

### BMP Format Support
- **1-bit monochrome**: Direct pixel mapping
- **24-bit RGB**: Grayscale conversion using `(R*299 + G*587 + B*114) / 1000`
- **32-bit RGBA**: Same as 24-bit, alpha channel ignored
- **Compression**: Supports BI_RGB (0) and BI_BITFIELDS (3)

### Memory Efficiency
- Reads BMP row-by-row (not entire file)
- Allocates only one row buffer at a time
- Frees buffer immediately after use
- Temp files deleted after display

## Related Files
- `SD_CRASH_FIX.md` - Documents SD card initialization fixes
- `NETWORK_MODE_ENABLED.md` - Network file browser feature
- `api.md` - Server API documentation

## Status
✅ **FIXED** - Thumbnails now display correctly in network file grid
