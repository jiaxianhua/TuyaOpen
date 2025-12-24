# SD Card File Browser Guide

## Overview

This e-paper reader now supports SD card file browsing and display. You can read TXT files and view BMP images directly from an SD card.

## Features

- **File Browser**: Browse files on SD card with visual interface
- **Text Reading**: Read and paginate TXT files (supports GBK Chinese)
- **Image Viewing**: Display BMP images (monochrome)
- **Button Control**: Navigate with simple button presses

## Supported File Types

| Type | Extension | Description |
|------|-----------|-------------|
| Text | `.txt` | Plain text files (ASCII/GBK) |
| Image | `.bmp` | Bitmap images (1-bit monochrome recommended) |
| Image | `.png` | PNG images (coming soon) |
| Image | `.jpg` | JPEG images (coming soon) |

## Button Controls

### File Browser Mode
- **Short Press**: Move to next file in list
- **Long Press (3s)**: Open selected file

### Text Reader Mode
- **Short Press**: Next page
- **Long Press (3s)**: Close file and return to browser

### Image Viewer Mode
- **Long Press (3s)**: Close image and return to browser

## SD Card Setup

### 1. Hardware Connection

Connect SD card module to your board:
- CLK → SD_CLK pin
- CMD → SD_CMD pin
- D0 → SD_D0 pin
- D1 → SD_D1 pin (optional for 4-bit mode)
- D2 → SD_D2 pin (optional for 4-bit mode)
- D3 → SD_D3 pin (optional for 4-bit mode)

### 2. File Preparation

#### Text Files
- Use UTF-8 or GBK encoding
- Keep files under 1MB for best performance
- Line breaks will be handled automatically

Example `sample.txt`:
```
这是一个示例文本文件。
This is a sample text file.

支持中英文混合显示。
Supports mixed Chinese and English.
```

#### BMP Images
- Use 1-bit monochrome BMP format for best results
- Recommended size: 800x480 pixels (full screen)
- Smaller images will be displayed at top-left corner

Convert images to monochrome BMP:
```bash
# Using ImageMagick
convert input.jpg -resize 800x480 -monochrome output.bmp

# Using Python PIL
from PIL import Image
img = Image.open('input.jpg')
img = img.resize((800, 480))
img = img.convert('1')  # 1-bit monochrome
img.save('output.bmp')
```

### 3. SD Card Structure

```
/sdcard/
├── sample1.txt      # Sample text file
├── readme.txt       # Instructions
├── smiley.bmp       # Sample image
├── photo1.bmp       # Your photos
├── photo2.bmp
├── novel.txt        # Your novels
└── ...
```

## Default Sample Files

The application automatically creates sample files on first run:

1. **sample1.txt** - Chinese/English mixed text demo
2. **readme.txt** - Instructions in English
3. **smiley.bmp** - 16x16 pixel smiley face icon

## Usage Flow

1. **Power On** → System initializes SD card
2. **File Browser** → Shows list of supported files
3. **Navigate** → Short press to move through files
4. **Open File** → Long press to open selected file
5. **View Content** → Read text or view image
6. **Return** → Long press to return to file browser

## Troubleshooting

### SD Card Not Detected
- Check hardware connections
- Verify SD card is formatted (FAT32 recommended)
- Check SD card is properly inserted
- Review logs for mount errors

### Files Not Showing
- Ensure files have supported extensions (.txt, .bmp)
- Check file names don't start with '.'
- Verify files are in root directory
- Maximum 50 files supported

### Text Display Issues
- Use GBK or UTF-8 encoding
- Ensure HZK16/HZK24 fonts are included
- Check file size is under 1MB
- Verify text content is readable

### Image Display Issues
- Convert to 1-bit monochrome BMP
- Check image dimensions (max 800x480)
- Verify BMP header is valid
- Use standard BMP format (no compression)

## Configuration

### Enable SD Card Support

In `app_default.config`:
```
CONFIG_ENABLE_FILESYSTEM=y
CONFIG_ENABLE_SDCARD=y
```

### SD Card Mount Path

Default: `/sdcard`

Change in `sd_file_manager.h`:
```c
#define SDCARD_MOUNT_PATH "/sdcard"
```

### Maximum Files

Default: 50 files

Change in `sd_file_manager.h`:
```c
#define MAX_FILES 50
```

## Performance Tips

1. **File Size**: Keep text files under 500KB for fast loading
2. **Image Size**: Use 800x480 or smaller for full-screen display
3. **File Count**: Limit to 50 files for responsive browsing
4. **Format**: Use monochrome BMP for fastest image display

## Future Enhancements

- [ ] PNG/JPG image support with dithering
- [ ] Subdirectory navigation
- [ ] File sorting options
- [ ] Bookmark support for text files
- [ ] Image zoom and pan
- [ ] File information display (size, date)

## Example Code

### Reading a Text File
```c
char *content = NULL;
int size = 0;
if (sd_read_text_file("/sdcard/myfile.txt", &content, &size) == OPRT_OK) {
    // Process content
    tal_free(content);
}
```

### Displaying a BMP Image
```c
if (sd_display_bmp_image("/sdcard/photo.bmp") == OPRT_OK) {
    EPD_4in26_Display(g_image_buffer);
}
```

### Scanning Files
```c
file_browser_t browser;
if (sd_scan_files(&browser) == OPRT_OK) {
    for (int i = 0; i < browser.file_count; i++) {
        PR_NOTICE("File: %s (type=%d)", 
                  browser.files[i].name, 
                  browser.files[i].type);
    }
}
```

## References

- [TuyaOpen Filesystem Documentation](../../peripherals/sd/)
- [E-Paper Display Guide](./README.md)
- [Button Configuration](./BUTTON_SETUP.md)
- [Chinese Font Support](./GBK_SUPPORT.md)
