# Build Instructions for E-Paper E-Book Reader

## Quick Start

### Prerequisites
- TuyaOpen development environment installed
- T5AI Board or compatible hardware
- 4.26-inch Waveshare e-Paper display
- SD card module
- Navigation buttons

### Build Steps

1. **Enter TuyaOpen environment:**
```bash
source export.sh
```

2. **Navigate to project:**
```bash
cd apps/e-paper-ebook-reader
```

3. **Configure for your board:**
```bash
tos.py config choice
```
Select "T5AI Board" or your target board.

4. **Build the project:**
```bash
tos.py build
```

5. **Flash to device:**
```bash
tos.py flash
```

6. **Monitor output:**
```bash
tos.py monitor
```

## Hardware Setup

### E-Paper Display Connections
Connect the 4.26-inch e-Paper display to your board:
- SCLK → P02
- DIN → P04
- CS → P03
- DC → P07
- RST → P08
- BUSY → P06
- PWR → P28

### SD Card Module
Configure SD card pins according to your board's pinmux configuration.

### Buttons
- Button 1 (BUTTON_NAME): Next page / Select
- Button 2 (BUTTON_NAME_2): Previous page
- Button 3 (BUTTON_NAME_3): Menu navigation (optional)

## Preparing the SD Card

1. **Format SD card as FAT32**

2. **Copy sample books:**
```bash
cp apps/e-paper-ebook-reader/assets/books/*.txt /path/to/sdcard/
```

3. **Insert SD card into module**

## Testing

The project includes test files in the `tests/` directory:
- `test_sd_manager.c` - SD card manager tests
- `test_page_manager.c` - Page manager tests

## Troubleshooting

### Build Errors
- Ensure TuyaOpen environment is properly activated
- Check that all dependencies are installed
- Verify CMakeLists.txt paths are correct

### Runtime Errors
- Check serial monitor for error messages
- Verify hardware connections
- Ensure SD card is properly formatted and contains .txt files
- Check button configuration in board files

## Project Structure

```
apps/e-paper-ebook-reader/
├── CMakeLists.txt          # Build configuration
├── app_default.config      # Application configuration
├── README.md               # User documentation
├── BUILD_INSTRUCTIONS.md   # This file
├── src/                    # Source files
│   ├── main.c              # Application entry point
│   ├── sd_card_manager.c   # SD card operations
│   ├── page_manager.c      # Pagination logic
│   ├── display_controller.c # Display operations
│   └── navigation_controller.c # Button handling
├── include/                # Header files
├── lib/                    # E-Paper driver library
├── assets/                 # Sample content
│   └── books/              # Sample e-books
└── tests/                  # Test files
```

## Next Steps

1. Build and flash the application
2. Copy sample books to SD card
3. Power on and test navigation
4. Add your own .txt books to the SD card
5. Customize button mappings if needed

## Support

For issues:
- Check the main README.md for detailed documentation
- Review debug logs via serial monitor
- Verify hardware connections
- Ensure SD card is readable and contains .txt files

## Contest Submission

This project is created for the Tuya Smart "智绘黑白，创享无限" E-Paper Development Challenge.

Features implemented:
- ✅ SD card file reading
- ✅ Text pagination with word wrapping
- ✅ Button navigation (next/previous/menu)
- ✅ E-Paper display integration
- ✅ Low-power sleep mode
- ✅ Book selection menu
- ✅ Error handling
- ✅ Sample books included

## License

This project uses the Waveshare e-Paper driver library (MIT License) and integrates with the TuyaOpen platform.
