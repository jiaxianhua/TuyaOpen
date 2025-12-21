# E-Paper E-Book Reader - Deployment Guide

## Build Status

✅ **Build Completed Successfully**

The application has been compiled and is ready for deployment to the T5AI board.

## Build Artifacts

Location: `apps/e-paper-ebook-reader/.build/bin/`

- **e-paper-ebook-reader_QIO_1.0.0.bin** (2.2 MB) - QIO flash mode binary
- **e-paper-ebook-reader_UA_1.0.0.bin** (2.0 MB) - UA flash mode binary

## Flashing to T5AI Board

### Prerequisites

1. T5AI development board connected via USB
2. TuyaOpen environment activated (`source export.sh` from project root)
3. Proper USB permissions configured

### Flash Command

From the project root directory:

```bash
cd apps/e-paper-ebook-reader
tos.py flash
```

The tool will automatically select the appropriate binary and flash it to your board.

### Alternative: Manual Flash

If you need to specify the port:

```bash
tos.py flash -p /dev/ttyUSB0  # Linux
tos.py flash -p COM3          # Windows
```

## Hardware Setup

### Required Components

1. **T5AI Board** - Main controller
2. **4.26-inch e-Paper Display** (Waveshare compatible)
3. **MicroSD Card** (formatted as FAT32)
4. **3 Push Buttons** (for navigation)

### Pin Connections

#### E-Paper Display (SPI)
- VCC → 3.3V
- GND → GND
- DIN (MOSI) → GPIO configured in board
- CLK (SCK) → GPIO configured in board
- CS → GPIO configured in board
- DC → GPIO configured in board
- RST → GPIO configured in board
- BUSY → GPIO configured in board

#### SD Card (SPI)
- VCC → 3.3V
- GND → GND
- MISO → GPIO configured in board
- MOSI → GPIO configured in board
- SCK → GPIO configured in board
- CS → GPIO configured in board

#### Navigation Buttons
- Button 1 (Next Page) → BUTTON_NAME
- Button 2 (Previous Page) → BUTTON_NAME_2
- Button 3 (Menu) → BUTTON_NAME_3

*Note: Actual GPIO pin numbers are defined in the T5AI board configuration.*

## SD Card Setup

### 1. Format SD Card

Format your microSD card as **FAT32**:

```bash
# Linux
sudo mkfs.vfat -F 32 /dev/sdX1

# Windows
Use Windows Format tool, select FAT32
```

### 2. Add E-Books

1. Create a `books/` folder in the SD card root (optional, app scans root by default)
2. Copy your `.txt` files to the SD card root or books folder
3. Ensure files are:
   - Plain text format (UTF-8 encoding recommended)
   - Maximum 1 MB per file
   - Named with `.txt` extension

### 3. Sample Books

Sample books are included in `apps/e-paper-ebook-reader/assets/books/`:
- `sample_book_1.txt` - Classic literature excerpt
- `sample_book_2.txt` - Technical content
- `sample_book_3.txt` - Short story

Copy these to your SD card for testing.

## Testing Procedure

### 1. Initial Power-On

1. Insert SD card with books
2. Connect e-Paper display
3. Power on the T5AI board
4. Wait for initialization (LED should indicate activity)

### 2. Book Selection

1. The device will display a menu of available books
2. Use Button 1/2 to navigate the menu
3. Press Button 3 to select a book

### 3. Reading Navigation

- **Button 1 (Short Press)**: Next page
- **Button 2 (Short Press)**: Previous page
- **Button 3 (Long Press)**: Return to book menu

### 4. Expected Behavior

- Display should show clear, readable text
- Page numbers should appear at the bottom
- Navigation should be responsive
- Display should refresh smoothly

## Monitoring and Debugging

### Serial Monitor

To view debug logs:

```bash
tos.py monitor
```

Or specify port:

```bash
tos.py monitor -p /dev/ttyUSB0
```

### Expected Log Output

```
[I] SD Card initialized successfully
[I] Found 3 books on SD card
[I] Displaying book menu
[I] Book selected: sample_book_1.txt
[I] Loaded book: 3245 characters, 12 pages
[I] Displaying page 1/12
```

### Common Issues

#### SD Card Not Detected
- Check SD card format (must be FAT32)
- Verify SD card connections
- Try a different SD card
- Check logs for mount errors

#### Display Not Working
- Verify e-Paper connections
- Check power supply (3.3V)
- Ensure display is compatible (4.26" Waveshare)
- Check SPI configuration in board files

#### No Books Found
- Ensure `.txt` files are in SD card root
- Check file extensions (must be `.txt`)
- Verify files are under 1MB
- Check file encoding (UTF-8 recommended)

#### Buttons Not Responding
- Verify button connections
- Check button configuration in board files
- Test with serial monitor to see button events

## Performance Optimization

### Display Refresh

The application uses:
- **Full refresh**: On book load and menu display
- **Partial refresh**: On page navigation (faster, less flicker)

### Power Management

- Display enters sleep mode after inactivity
- Wake on button press
- Low power consumption in sleep mode

## Next Steps

1. **Flash the firmware** using `tos.py flash`
2. **Prepare SD card** with sample books
3. **Connect hardware** according to pin diagram
4. **Power on and test** basic functionality
5. **Monitor logs** for any issues
6. **Add your own books** and enjoy reading!

## Competition Submission

This project is designed for the Tuya Smart "智绘黑白，创享无限" E-Paper Development Challenge.

### Submission Checklist

- ✅ Complete source code
- ✅ Build instructions
- ✅ Hardware setup guide
- ✅ Sample books included
- ✅ Comprehensive documentation
- ⏳ Demo video (to be created after hardware testing)
- ⏳ Photos of working system (to be taken after hardware testing)

### Project Highlights

- **Modular architecture** with clean separation of concerns
- **Comprehensive error handling** for robust operation
- **Efficient pagination** algorithm for smooth reading
- **Low power design** with sleep mode support
- **User-friendly navigation** with intuitive button controls
- **Extensible design** for future enhancements

## Support

For issues or questions:
1. Check the troubleshooting section above
2. Review serial monitor logs
3. Consult TuyaOpen documentation
4. Join the competition technical support group

---

**Build Date**: December 21, 2024
**TuyaOpen SDK Version**: Latest
**Target Platform**: T5AI Board
**Display**: 4.26-inch e-Paper (800x480)
