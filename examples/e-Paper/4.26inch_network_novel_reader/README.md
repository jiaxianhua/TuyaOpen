# 4.26inch E-Paper Network Novel Reader

A network-enabled e-paper novel reader with SD card support that can fetch text content from HTTP URLs or read files directly from SD card, displaying them with button-controlled pagination.

## Features

### Core Features
- **Network Fetching**: Downloads novels from HTTP URLs via WiFi
- **SD Card Support**: Browse and read TXT files and view BMP images from SD card
- **E-Paper Display**: Shows text content on 4.26" e-paper display (800x480)
- **File Browser**: Visual interface for browsing SD card files
- **Button Navigation**: 
  - **File Browser**: Short press = Next file, Long press = Open file
  - **Text Reader**: Short press = Next page, Long press = Close file
  - **Image Viewer**: Long press = Close image
- **Automatic Pagination**: Splits content into readable pages
- **Chinese Support**: Full GBK Chinese character display with HZK24 fonts
- **Low Power**: E-paper display only updates when changing pages

### SD Card Features (NEW!)
- ✅ **File Browser**: Browse TXT and BMP files on SD card
- ✅ **Text Reading**: Read TXT files with automatic pagination
- ✅ **Image Viewing**: Display BMP images (monochrome)
- ✅ **Auto Sample Files**: Creates sample files on first run
- ✅ **Image Converter**: Python tool to convert photos to BMP
- ⏳ **PNG/JPG Support**: Coming soon

## Quick Start

### 1. SD Card Setup (Optional)
If you have an SD card module:
1. Connect SD card module to your board
2. Format SD card as FAT32
3. Copy TXT files and BMP images to root directory
4. See [SD_CARD_QUICKSTART.md](SD_CARD_QUICKSTART.md) for details

### 2. WiFi Configuration
Update these settings in `examples/EPD_4in26_network_novel.c`:

```c
// WiFi Configuration
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"

// Novel URL (if not using SD card)
#define NOVEL_URL "http://example.com/novel.txt"
```

### Button Configuration

The project uses **GPIO 17** as the default button pin. If your board uses a different GPIO:

1. Open `board_button_config.h`
2. Change `BOARD_BUTTON_PIN` to your GPIO number:
   ```c
   #define BOARD_BUTTON_PIN TUYA_GPIO_NUM_17  // Change this
   ```

See [BUTTON_SETUP.md](BUTTON_SETUP.md) for detailed button configuration guide.

## Hardware Requirements

- Tuya T5AI board with 4.26" e-paper display
- WiFi connectivity (for network novel fetching)
- At least one button (configured via BUTTON_NAME in board config)
- SD card module (optional, for local file browsing)

## Usage Modes

### Mode 1: SD Card File Browser (Recommended)
1. Insert SD card with TXT/BMP files
2. Power on device
3. Browse files with short button press
4. Open files with long button press (3s)
5. Read/view content, long press to return

### Mode 2: Network Novel Reader
1. Configure WiFi credentials
2. Set novel URL
3. Device downloads and displays novel
4. Use buttons to navigate pages

### Mode 3: Embedded Novel (Fallback)
- If no SD card or network, displays built-in sample novel
- Useful for testing and demonstration

## Button Controls Summary

| Mode | Short Press | Long Press (3s) |
|------|-------------|-----------------|
| **File Browser** | Next file | Open file |
| **Text Reader** | Next page | Close file / Previous page |
| **Image Viewer** | - | Close image |

## Configuration

### Display Settings

You can adjust these parameters for different text layouts:

```c
#define CHARS_PER_LINE 50    // Characters per line
#define LINES_PER_PAGE 25    // Lines per page
#define BUTTON_LONG_PRESS_TIME 3000  // Long press duration (ms)
```

## Building

1. **Setup environment**:
   ```bash
   source export.sh
   ```

2. **Configure project**:
   ```bash
   tos.py config
   ```
   - Select platform: `T5AI`
   - Select board: `T5AI_BOARD_EX_MODULE_NONE`
   - Select example: `4.26inch_network_novel_reader`

3. **Build**:
   ```bash
   tos.py build
   ```

4. **Flash to device**:
   ```bash
   tos.py flash
   ```

5. **Monitor output**:
   ```bash
   tos.py monitor
   ```

## Usage

1. Power on the device
2. Wait for WiFi connection (displays "Connecting to WiFi...")
3. Novel downloads automatically (displays "Downloading novel...")
4. First page displays automatically
5. Use button to navigate:
   - **Short press**: Go to next page
   - **Long press**: Go to previous page

## Novel Format

The reader supports plain text files (`.txt`) with:
- UTF-8 or ASCII encoding
- Maximum size: 1MB
- Automatic line wrapping
- Page info displayed at top

## Example Novel URLs

For testing, you can use public domain texts:

```c
// Project Gutenberg example
#define NOVEL_URL "http://www.gutenberg.org/files/1342/1342-0.txt"  // Pride and Prejudice

// Or host your own text file
#define NOVEL_URL "http://192.168.1.100/mynovel.txt"
```

## Troubleshooting

### WiFi Connection Failed
- Check SSID and password are correct
- Ensure WiFi network is 2.4GHz (not 5GHz)
- Check signal strength

### Download Failed
- Verify URL is accessible from your network
- Check URL uses HTTP (not HTTPS)
- Ensure file size is under 1MB
- Test URL in browser first

### Button Not Working
- Check BUTTON_NAME is defined in board configuration
- Verify button GPIO is correctly configured
- Check button debounce settings

### Display Issues
- Ensure e-paper display is properly connected
- Check SPI pins configuration
- Verify display initialization succeeds

## Code Structure

```
4.26inch_network_novel_reader/
├── CMakeLists.txt              # Build configuration
├── app_default.config          # Board and feature config
├── README.md                   # This file
├── examples/
│   ├── main.c                  # Entry point
│   ├── EPD_Test.h              # Header file
│   └── EPD_4in26_network_novel.c  # Main implementation
└── lib/                        # E-paper driver libraries
    ├── Config/                 # Hardware configuration
    ├── e-Paper/                # E-paper drivers
    ├── Fonts/                  # Font data
    └── GUI/                    # Graphics functions
```

## Key Functions

- `init_network()`: Connects to WiFi
- `fetch_novel()`: Downloads content from URL
- `display_page()`: Renders current page to e-paper
- `button_handler()`: Handles button press events
- `EPD_network_novel_test()`: Main application loop

## Customization

### Change Font Size

Edit in `display_page()`:
```c
Paint_DrawString_EN(10, y_pos, line_buf, &Font16, WHITE, BLACK);
// Change &Font16 to &Font12, &Font20, or &Font24
```

### Adjust Page Layout

Modify these constants:
```c
#define CHARS_PER_LINE 50    // Increase for more text per line
#define LINES_PER_PAGE 25    // Increase for more lines per page
```

### Add More Buttons

In `init_button()`, create additional buttons:
```c
#ifdef BUTTON_NAME_2
    tdl_button_create(BUTTON_NAME_2, &button_cfg, &button_hdl_2);
    // Register events for second button
#endif
```

## Performance

- **WiFi Connection**: ~5-10 seconds
- **Novel Download**: Depends on file size and network speed
- **Page Refresh**: ~2 seconds (e-paper refresh time)
- **Memory Usage**: ~800KB for display buffer + novel content

## License

Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.

## Support

For issues and questions:
- Check TuyaOpen documentation
- Review example code in `examples/e-Paper/`
- Consult board-specific documentation in `boards/T5AI/`
