# E-Paper E-Book Reader

An embedded e-book reader application for TuyaOpen platform using a 4.26-inch e-Paper display.

## Features

- Read .txt e-books from SD card
- Page-by-page navigation with button controls
- Low-power e-Paper display with partial refresh
- Support for books up to 1MB
- Automatic pagination with word wrapping
- Book selection menu
- Sleep mode after inactivity
- High-contrast, sunlight-readable display
- No backlight required (reads like paper)
- Retains display content when powered off

## Specifications

### Display
- **Model**: Waveshare 4.26" e-Paper Display
- **Resolution**: 800×480 pixels
- **Technology**: E-Ink (electrophoretic display)
- **Colors**: Black and white (1-bit)
- **Viewing Angle**: 180° (paper-like)
- **Refresh Time**: 2-3 seconds (full), 0.5-1 second (partial)
- **Power Consumption**: ~0W static, ~0.5W during refresh
- **Interface**: SPI (4-wire + control signals)

### Storage
- **Media**: SD/SDHC card (up to 32GB recommended)
- **Format**: FAT32 filesystem
- **File Type**: Plain text (.txt)
- **Encoding**: UTF-8, ASCII
- **Max File Size**: 1MB per book
- **Capacity**: Limited only by SD card size

### Performance
- **Startup Time**: 3-5 seconds
- **Page Turn Speed**: 0.5-1 second (partial refresh)
- **Menu Display**: 2-3 seconds (full refresh)
- **Text Capacity**: ~4,750 characters per page (Font16)
- **Memory Usage**: ~1.1MB RAM minimum

### Power
- **Operating Voltage**: 3.3V (MCU), 5V (display)
- **Active Current**: ~100-200mA during refresh
- **Sleep Current**: <1mA
- **Battery Life**: 10-20 hours active reading (estimated)

## Hardware Configuration

### Pin Mapping

**CRITICAL**: SD card uses SDIO on GPIO 2/3/4. E-Paper must use different pins to avoid conflicts.

**e-Paper Display (SPI_NUM_0)**:
- SCLK: GPIO 14
- MOSI: GPIO 16
- CS: GPIO 15
- DC: GPIO 7
- RST: GPIO 8
- BUSY: GPIO 6
- PWR: GPIO 28

**SD Card (SDIO - Hardware Fixed)**:
- CLK: GPIO 2
- CMD: GPIO 3
- DATA0: GPIO 4

**Navigation Buttons**:
- Button 1: Configured via board definition (Next page / Select)
- Button 2: Optional (Previous page)
- Button 3: Optional (Menu navigation)

### Hardware Requirements

- Tuya T5AI development board
- 4.26" e-Paper display (Waveshare compatible)
- MicroSD card (FAT32 formatted, up to 32GB)
- Push buttons for navigation (at least 1 required)

### Environmental
- **Operating Temperature**: 0°C to 50°C
- **Storage Temperature**: -25°C to 70°C
- **Humidity**: 35% to 65% RH (non-condensing)

## Compatibility

### Supported Boards
- **Primary**: Tuya T5AI Board (TUYA_T5AI_BOARD)
- **Compatible**: Any TuyaOpen board with:
  - SPI interface
  - Minimum 1.5MB RAM
  - 7+ GPIO pins for display control
  - Button support (GPIO with interrupts)
  - SD card interface (SPI or SDIO)

### Tested Platforms
- ✅ Tuya T5AI Board
- ✅ T5AI EVB (Evaluation Board)
- ⚠️ Other T5AI variants (may require pin configuration changes)

### Display Compatibility
- **Supported**: Waveshare 4.26" e-Paper (800×480)
- **Driver**: EPD_4in26 (included in lib/)
- **Other Displays**: Requires driver modification

### SD Card Compatibility
- **Tested**: SanDisk, Samsung, Kingston SD/SDHC cards
- **Recommended**: Class 10 or higher for best performance
- **Size**: 1GB to 32GB (FAT32 limit)
- **Not Supported**: SDXC cards >32GB (exFAT format)

## Hardware Requirements

- TuyaOpen compatible board (T5AI Board recommended)
- 4.26-inch Waveshare e-Paper display (800x480 resolution)
- SD card module (SPI interface)
- 2-3 navigation buttons (active-low with pull-up resistors)
- 5V power supply for e-Paper display
- Proper pin connections (see Pin Configuration below)

## Hardware Setup

### 1. E-Paper Display Connection

The 4.26-inch Waveshare e-Paper display uses SPI interface. Connect the display to your T5AI board as follows:

```
E-Paper Display          T5AI Board
─────────────────────────────────────
VCC (3.3V/5V)     →      5V or 3.3V
GND               →      GND
DIN (MOSI)        →      P04 (SPI_MOSI)
CLK (SCLK)        →      P02 (SPI_CLK)
CS (Chip Select)  →      P03 (SPI_CS)
DC (Data/Command) →      P07 (GPIO)
RST (Reset)       →      P08 (GPIO)
BUSY              →      P06 (GPIO)
PWR (Power Ctrl)  →      P28 (GPIO)
```

**Important Notes:**
- The display requires 5V power for proper operation
- Ensure all SPI signals are properly connected
- The BUSY pin is used to detect when the display is ready
- PWR pin controls display power (active high)

### 2. SD Card Module Connection

The SD card module uses SPI interface. Configure the pins in your board's pinmux configuration:

```
SD Card Module           T5AI Board
─────────────────────────────────────
VCC (3.3V)        →      3.3V
GND               →      GND
MISO              →      SPI_MISO (board-specific)
MOSI              →      SPI_MOSI (board-specific)
SCK               →      SPI_CLK (board-specific)
CS                →      SPI_CS (board-specific)
```

**Note:** SD card pins are configured via the board's pinmux settings. Refer to your board documentation for specific pin assignments.

### 3. Button Configuration

The application supports 2-3 buttons for navigation:

```
Button Function          T5AI Board Pin    Configuration
──────────────────────────────────────────────────────────
Button 1 (Primary)       P12 (BUTTON_NAME)  Active-low, pull-up
  - Short press: Next page
  - Long press (3s): Back to menu

Button 2 (Optional)      BUTTON_NAME_2      Active-low, pull-up
  - Short press: Previous page

Button 3 (Optional)      BUTTON_NAME_3      Active-low, pull-up
  - Short press: Select/Menu navigation
```

**Button Wiring:**
```
Button → GPIO Pin
Button → GND

(Internal pull-up resistor is enabled in software)
```

### 4. Complete Wiring Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                      T5AI Board                              │
│                                                              │
│  P02 (SPI_CLK)  ──────────────────→ CLK   ┌──────────────┐ │
│  P04 (SPI_MOSI) ──────────────────→ DIN   │              │ │
│  P03 (SPI_CS)   ──────────────────→ CS    │  4.26" EPD   │ │
│  P07 (GPIO)     ──────────────────→ DC    │  800x480     │ │
│  P08 (GPIO)     ──────────────────→ RST   │              │ │
│  P06 (GPIO)     ──────────────────→ BUSY  │              │ │
│  P28 (GPIO)     ──────────────────→ PWR   └──────────────┘ │
│                                                              │
│  SPI_MISO       ──────────────────→ MISO  ┌──────────────┐ │
│  SPI_MOSI       ──────────────────→ MOSI  │   SD Card    │ │
│  SPI_CLK        ──────────────────→ SCK   │   Module     │ │
│  SPI_CS         ──────────────────→ CS    └──────────────┘ │
│                                                              │
│  P12 (GPIO)     ←────────────────── Button 1 (Next/Menu)    │
│  BUTTON_NAME_2  ←────────────────── Button 2 (Previous)     │
│  BUTTON_NAME_3  ←────────────────── Button 3 (Select)       │
│                                                              │
│  5V/3.3V        ──────────────────→ Power Supply            │
│  GND            ──────────────────→ Ground                  │
└─────────────────────────────────────────────────────────────┘
```

## Pin Configuration Summary

### E-Paper Display Pins
| Signal | T5AI Pin | Function |
|--------|----------|----------|
| SCLK   | P02      | SPI Clock |
| DIN    | P04      | SPI MOSI (Data In) |
| CS     | P03      | SPI Chip Select |
| DC     | P07      | Data/Command Select |
| RST    | P08      | Reset (Active Low) |
| BUSY   | P06      | Busy Status (Input) |
| PWR    | P28      | Power Control (Active High) |

### SD Card Pins
Configure SD card pins in your board configuration file or via pinmux settings. The SD card uses standard SPI interface (MISO, MOSI, SCK, CS).

### Button Pins
| Button | Default Pin | Function |
|--------|-------------|----------|
| Button 1 | P12 (BUTTON_NAME) | Next page / Back to menu |
| Button 2 | BUTTON_NAME_2 | Previous page (optional) |
| Button 3 | BUTTON_NAME_3 | Select/Menu (optional) |

**Note:** Button pins are defined in the board configuration. Check `boards/T5AI/TUYA_T5AI_BOARD/tuya_t5ai_board.c` for your specific board's button configuration.

## SD Card Setup

1. Format SD card as FAT32
2. Copy .txt book files to the root directory of the SD card
3. Insert SD card into the module
4. Power on the device

### Supported File Format

- File extension: `.txt`
- Encoding: UTF-8 or ASCII
- Maximum file size: 1MB
- Location: Root directory of SD card

## Building and Flashing

1. Enter TuyaOpen build environment:
```bash
source export.sh
```

2. Navigate to the application directory:
```bash
cd apps/e-paper-ebook-reader
```

3. Configure the project:
```bash
tos.py config choice
```
Select your target board (e.g., T5AI Board).

4. Build the project:
```bash
tos.py build
```

5. Flash to device:
```bash
tos.py flash
```

6. Monitor output:
```bash
tos.py monitor
```

## Usage

### First Boot

1. **Power On**: The device initializes and performs the following sequence:
   - Display initialization (2-3 seconds)
   - SD card mount and scan
   - Book list generation
   - Menu display

2. **Expected Startup Logs:**
   ```
   [I] Initializing e-book reader...
   [I] Initializing SD card manager...
   [I] SD card mounted successfully
   [I] Found 3 books on SD card
   [I] Initializing display controller...
   [I] Display initialized successfully
   [I] Initializing navigation controller...
   [I] E-book reader initialized successfully
   ```

### Book Selection Menu

When the system starts, you'll see a menu listing all available books:

```
┌────────────────────────────────────────┐
│ E-Book Reader - Select a Book          │
├────────────────────────────────────────┤
│                                        │
│  > sample_book_1.txt                   │
│    sample_book_2.txt                   │
│    sample_book_3.txt                   │
│                                        │
│                                        │
│                                        │
│                                        │
├────────────────────────────────────────┤
│ Use buttons to select                  │
└────────────────────────────────────────┘
```

**Menu Navigation:**
- Button 1 (short press): Select highlighted book
- Button 2: Move selection up/down (if implemented)
- Button 3: Alternative selection (if implemented)

### Reading a Book

After selecting a book, the first page is displayed:

```
┌────────────────────────────────────────┐
│ sample_book_1.txt                      │
├────────────────────────────────────────┤
│                                        │
│  Lorem ipsum dolor sit amet,           │
│  consectetur adipiscing elit.          │
│  Sed do eiusmod tempor incididunt      │
│  ut labore et dolore magna aliqua.     │
│  Ut enim ad minim veniam, quis         │
│  nostrud exercitation ullamco          │
│  laboris nisi ut aliquip ex ea         │
│  commodo consequat.                    │
│                                        │
│  Duis aute irure dolor in              │
│  reprehenderit in voluptate velit      │
│  esse cillum dolore eu fugiat          │
│  nulla pariatur.                       │
│                                        │
├────────────────────────────────────────┤
│                          Page 1/5      │
└────────────────────────────────────────┘
```

**Reading Controls:**
- **Button 1 (short press)**: Next page
  - Advances to the next page
  - Shows "End of book" message on last page
  - Uses partial refresh for faster updates

- **Button 2 (short press)**: Previous page
  - Returns to previous page
  - Stays on page 1 if already at beginning
  - Uses partial refresh for faster updates

- **Button 1 (long press - 3 seconds)**: Back to menu
  - Returns to book selection menu
  - Performs full screen refresh
  - Current position is lost (will restart from page 1 on next open)

### Display Behavior

**Refresh Types:**
1. **Full Refresh** (2-3 seconds):
   - Used for: Menu display, book switching, error messages
   - Clears all ghosting
   - Complete screen redraw
   - Visible flashing effect

2. **Partial Refresh** (0.5-1 second):
   - Used for: Page navigation within a book
   - Faster updates
   - May show slight ghosting after many updates
   - Recommended for normal reading

**Visual Feedback:**
- Page turns show brief flashing (normal for e-Paper)
- BUSY LED may blink during refresh (board-dependent)
- Display retains image when powered off (e-Paper feature)

### Sleep Mode

**Automatic Sleep:**
- Activates after 30 seconds of inactivity
- Display remains visible (e-Paper retains image)
- System enters low-power mode
- Reduces power consumption

**Wake from Sleep:**
- Press any button to wake
- System resumes at current page
- Brief delay for system wake-up (~1 second)

### Error Messages

The system displays clear error messages when issues occur:

**SD Card Errors:**
```
┌────────────────────────────────────────┐
│ Error                                  │
├────────────────────────────────────────┤
│                                        │
│  SD Card Error                         │
│                                        │
│  Please check:                         │
│  - SD card is inserted                 │
│  - Card is formatted as FAT32          │
│  - Connections are secure              │
│                                        │
└────────────────────────────────────────┘
```

**No Books Found:**
```
┌────────────────────────────────────────┐
│ E-Book Reader                          │
├────────────────────────────────────────┤
│                                        │
│  No books found                        │
│                                        │
│  Please add .txt files to the          │
│  root directory of your SD card        │
│                                        │
└────────────────────────────────────────┘
```

**File Open Error:**
```
┌────────────────────────────────────────┐
│ Error                                  │
├────────────────────────────────────────┤
│                                        │
│  Cannot open file: book.txt            │
│                                        │
│  The file may be corrupted or          │
│  too large (max 1MB)                   │
│                                        │
└────────────────────────────────────────┘
```

### Expected Performance

**Typical Operation:**
- Startup time: 3-5 seconds
- Book scanning: <1 second for 10 books
- Page turn (partial refresh): 0.5-1 second
- Menu display (full refresh): 2-3 seconds
- Book loading: <1 second for 500KB file
- Sleep mode entry: Immediate
- Wake from sleep: ~1 second

**Memory Usage:**
- Display buffer: ~48KB (800x480 / 8)
- Book content: Up to 1MB
- System overhead: ~50KB
- Total RAM required: ~1.1MB minimum

**Battery Life (estimated):**
- Active reading: 10-20 hours (depends on page turn frequency)
- Sleep mode: Several days to weeks
- E-Paper display uses no power when static
- Most power consumed during refresh operations

## Sample Books

The `assets/books/` directory contains sample e-books for testing:
- `sample_book_1.txt` - Classic literature excerpt (Pride and Prejudice)
- `sample_book_2.txt` - Technical content (IoT and embedded systems)
- `sample_book_3.txt` - Short story (The Gift of the Magi)

Copy these files to your SD card root directory to test the reader.

## System Demonstration

### Visual Examples

**Note:** Due to the nature of e-Paper displays, the actual appearance may vary based on lighting conditions and viewing angle. E-Paper displays provide excellent readability in bright light and consume no power when displaying static content.

### What You Should See

1. **Startup Sequence:**
   - Brief white flash as display initializes
   - Menu appears with list of available books
   - Clear, high-contrast text on white background

2. **Reading Experience:**
   - Crisp, paper-like text rendering
   - No backlight (reads like printed paper)
   - Excellent outdoor readability
   - No eye strain from prolonged reading

3. **Page Transitions:**
   - Brief flashing during full refresh
   - Faster partial refresh for page turns
   - Slight ghosting may appear after many page turns (normal)
   - Full refresh clears all ghosting

### Display Characteristics

**E-Paper Technology:**
- **Bistable Display**: Retains image without power
- **High Contrast**: Black text on white background
- **Wide Viewing Angle**: Readable from any angle
- **Sunlight Readable**: Better visibility in bright light
- **No Backlight**: Requires ambient light to read
- **Refresh Rate**: 2-3 seconds for full refresh

**Resolution and Text Quality:**
- 800x480 pixels (4.26" diagonal)
- ~188 DPI (dots per inch)
- Font16: Approximately 50 lines × 95 characters per page
- Font20: Approximately 40 lines × 76 characters per page
- Clear, readable text at normal reading distance

### Testing Your System

To verify your system is working correctly:

1. **Display Test:**
   - Power on and observe initialization
   - Menu should appear within 5 seconds
   - Text should be clear and readable

2. **SD Card Test:**
   - Insert SD card with sample books
   - Menu should list all .txt files
   - Try selecting different books

3. **Navigation Test:**
   - Open a book and navigate through pages
   - Test next/previous page buttons
   - Test long press to return to menu

4. **Performance Test:**
   - Page turns should complete in 0.5-1 second (partial refresh)
   - Menu display should complete in 2-3 seconds (full refresh)
   - No system crashes or freezes

### Known Visual Characteristics

**Normal Behavior:**
- Flashing during refresh (inherent to e-Paper technology)
- Slight ghosting after multiple partial refreshes
- Temporary image retention (clears with full refresh)
- Slower refresh in cold temperatures

**Not Normal (Indicates Problem):**
- Completely blank display (check power and connections)
- Garbled or corrupted text (check SPI connections)
- No response to button presses (check button wiring)
- Display stuck on one image (check BUSY pin connection)

## Troubleshooting

### SD Card Issues

#### SD Card Not Detected
**Symptoms:** Error message "SD Card Error" on display, or no books found.

**Solutions:**
1. **Check Physical Connection:**
   - Ensure SD card is properly inserted into the module
   - Verify all SPI pins (MISO, MOSI, SCK, CS) are connected
   - Check for loose wires or poor solder joints

2. **Verify SD Card Format:**
   - Format SD card as FAT32 (not exFAT or NTFS)
   - Use SD cards 32GB or smaller for best compatibility
   - Try a different SD card to rule out card failure

3. **Check Power Supply:**
   - SD card modules require stable 3.3V power
   - Ensure adequate current supply (some cards draw 100mA+)
   - Check for voltage drops under load

4. **Review Debug Logs:**
   ```bash
   tos.py monitor
   ```
   Look for mount errors like:
   - `SD card mount failed: -1` (hardware issue)
   - `SD card init failed` (SPI communication problem)
   - `No filesystem found` (formatting issue)

5. **Verify SPI Configuration:**
   - Check that SPI bus is not shared with conflicting devices
   - Verify SPI clock speed is appropriate (try reducing if issues persist)
   - Ensure CS pin is correctly configured as output

#### Books Not Appearing in Menu
**Symptoms:** SD card mounts successfully but no books are listed.

**Solutions:**
1. **Check File Location:**
   - Files MUST be in the root directory of the SD card
   - Files in subdirectories will not be detected
   - Example correct structure:
     ```
     SD_CARD:/
     ├── book1.txt
     ├── book2.txt
     └── book3.txt
     ```

2. **Verify File Extension:**
   - Files must have `.txt` extension (lowercase)
   - Rename files if they have `.TXT` (uppercase) or other extensions
   - Hidden files (starting with `.`) are ignored

3. **Check File Size:**
   - Maximum file size is 1MB (1,048,576 bytes)
   - Files larger than 1MB will be skipped
   - Use `ls -lh` to check file sizes

4. **Verify File Encoding:**
   - Use UTF-8 or ASCII encoding
   - Avoid special characters in filenames
   - Test with simple ASCII filenames first

5. **Check File Permissions:**
   - Ensure files are readable (not write-protected)
   - Try copying files again to SD card

### Display Issues

#### Display Not Working / Blank Screen
**Symptoms:** Display remains white/blank, no content visible.

**Solutions:**
1. **Verify Power Supply:**
   - E-Paper display requires 5V power (not 3.3V)
   - Check voltage at display VCC pin with multimeter
   - Ensure power supply can provide sufficient current (200mA+)
   - Verify PWR control pin (P28) is functioning

2. **Check SPI Connections:**
   - Verify all 7 signal pins are connected correctly:
     - SCLK → P02
     - DIN → P04
     - CS → P03
     - DC → P07
     - RST → P08
     - BUSY → P06
     - PWR → P28
   - Check for reversed connections (MOSI/MISO swap)
   - Ensure good electrical contact (no cold solder joints)

3. **Test Display Initialization:**
   - Check debug logs for initialization errors:
     ```
     EPD_4in26_Init() failed
     Display initialization failed
     ```
   - If initialization fails, check RST and BUSY pins
   - Try power cycling the display

4. **Verify Display Model:**
   - Ensure you have the correct 4.26" display (800x480)
   - Different display models require different drivers
   - Check Waveshare part number matches

5. **Check for Hardware Damage:**
   - Inspect display ribbon cable for damage
   - Look for physical damage to display surface
   - Test with a known-good display if available

#### Display Shows Garbled Text
**Symptoms:** Text is unreadable, characters overlap, or display is corrupted.

**Solutions:**
1. **Check SPI Communication:**
   - Reduce SPI clock speed if data corruption occurs
   - Verify signal integrity with oscilloscope if available
   - Check for electromagnetic interference from nearby devices

2. **Verify Memory Allocation:**
   - Check logs for memory allocation failures
   - Ensure sufficient heap memory is available
   - Try reducing buffer sizes if memory is limited

3. **Test with Sample Books:**
   - Use provided sample books to rule out file encoding issues
   - If samples work, check encoding of your custom books

#### Display Refresh Issues
**Symptoms:** Display doesn't update, ghosting, or slow refresh.

**Solutions:**
1. **Check BUSY Pin:**
   - BUSY pin must be connected for proper timing
   - Display won't refresh correctly without BUSY signal
   - Verify BUSY pin is configured as input

2. **Verify Refresh Mode:**
   - Full refresh takes 2-3 seconds (normal)
   - Partial refresh is faster but may cause ghosting
   - Try full refresh if partial refresh causes issues

3. **Temperature Considerations:**
   - E-Paper displays are temperature-sensitive
   - Refresh may be slower in cold environments
   - Optimal operating temperature: 0-50°C

### Navigation Issues

#### Buttons Not Responding
**Symptoms:** Pressing buttons has no effect, navigation doesn't work.

**Solutions:**
1. **Check Button Connections:**
   - Verify buttons are connected to correct GPIO pins
   - Ensure buttons connect GPIO to GND when pressed
   - Check for loose connections or broken wires

2. **Verify Button Configuration:**
   - Buttons must be active-low (connect to GND)
   - Internal pull-up resistors are enabled in software
   - Check board configuration file for button definitions:
     ```c
     #define BOARD_BUTTON_PIN TUYA_GPIO_NUM_12
     #define BOARD_BUTTON_ACTIVE_LV TUYA_GPIO_LEVEL_LOW
     ```

3. **Test Button Hardware:**
   - Use multimeter to verify button continuity
   - Check that GPIO pin goes low when button is pressed
   - Test with a simple LED circuit if available

4. **Review Button Event Logs:**
   ```bash
   tos.py monitor
   ```
   Look for button events:
   - `Button event: BUTTON_NAME, event: 0` (press down)
   - `Button event: BUTTON_NAME, event: 3` (long press)
   - If no events appear, check hardware connections

5. **Check Debounce Settings:**
   - Default debounce time is 50ms
   - Increase if buttons are too sensitive
   - Decrease if buttons feel unresponsive

#### Long Press Not Working
**Symptoms:** Long press doesn't return to menu, only short press works.

**Solutions:**
1. **Hold Button Longer:**
   - Long press requires 3 seconds (3000ms)
   - Ensure you're holding button long enough
   - Watch for feedback in debug logs

2. **Check Long Press Configuration:**
   - Verify `long_start_valid_time = 3000` in code
   - Adjust timing if needed in `navigation_controller.c`

### Performance Issues

#### Slow Page Turns
**Symptoms:** Page navigation takes several seconds.

**Solutions:**
1. **This is Normal for E-Paper:**
   - Full refresh takes 2-3 seconds (by design)
   - E-Paper displays are inherently slower than LCD
   - Partial refresh is faster but may cause ghosting

2. **Optimize if Needed:**
   - Use partial refresh for page navigation
   - Reserve full refresh for book switching
   - Reduce font size to fit more text per page

#### High Memory Usage
**Symptoms:** System crashes, memory allocation failures.

**Solutions:**
1. **Reduce Book Size:**
   - Keep books under 500KB for best performance
   - Split large books into multiple files
   - Remove unnecessary whitespace from text files

2. **Check Available Memory:**
   - Monitor heap usage in debug logs
   - Ensure sufficient RAM for display buffer + book content
   - Typical requirement: ~100KB for display + book size

### Build and Flash Issues

#### Build Fails
**Symptoms:** Compilation errors, linker errors.

**Solutions:**
1. **Check Environment Setup:**
   ```bash
   source export.sh
   ```
   Ensure TuyaOpen environment is activated

2. **Verify Board Selection:**
   ```bash
   tos.py config choice
   ```
   Select correct target board (T5AI Board)

3. **Clean and Rebuild:**
   ```bash
   tos.py clean
   tos.py build
   ```

4. **Check Dependencies:**
   - Ensure all submodules are initialized
   - Verify e-Paper library is present in `lib/` directory

#### Flash Fails
**Symptoms:** Cannot flash firmware to device.

**Solutions:**
1. **Check USB Connection:**
   - Verify device is connected via USB
   - Check that correct serial port is selected
   - Try different USB cable or port

2. **Verify Device Mode:**
   - Some boards require boot mode for flashing
   - Check board documentation for flash procedure
   - Try resetting board before flashing

3. **Check Permissions:**
   - On Linux, you may need to add user to dialout group:
     ```bash
     sudo usermod -a -G dialout $USER
     ```
   - Log out and back in for changes to take effect

### Getting Help

If you continue to experience issues:

1. **Enable Debug Logging:**
   - Set log level to DEBUG in configuration
   - Monitor serial output: `tos.py monitor`
   - Save logs for troubleshooting

2. **Check Documentation:**
   - TuyaOpen documentation: https://developer.tuya.com
   - Waveshare e-Paper wiki: https://www.waveshare.com/wiki/
   - Review example projects in `examples/e-Paper/`

3. **Hardware Verification:**
   - Test each component individually
   - Use example projects to verify hardware
   - Check for hardware conflicts (shared pins, etc.)

4. **Report Issues:**
   - Include hardware setup details
   - Provide debug logs
   - Describe steps to reproduce the problem
   - Mention any modifications made to code or hardware

## Development

### Project Structure
```
apps/e-paper-ebook-reader/
├── CMakeLists.txt          # Build configuration
├── app_default.config      # Application configuration
├── README.md               # This file
├── BUILD_INSTRUCTIONS.md   # Detailed build guide
├── src/                    # Source files
│   ├── main.c              # Application entry point
│   ├── sd_card_manager.c   # SD card operations
│   ├── page_manager.c      # Pagination logic
│   ├── display_controller.c # Display operations
│   └── navigation_controller.c # Button handling
├── include/                # Header files
│   ├── sd_card_manager.h
│   ├── page_manager.h
│   ├── display_controller.h
│   └── navigation_controller.h
├── lib/                    # E-Paper driver library
│   ├── Config/             # Hardware configuration
│   ├── e-Paper/            # Display driver
│   ├── Fonts/              # Font definitions
│   └── GUI/                # Graphics primitives
├── tests/                  # Unit tests
│   ├── test_sd_manager.c
│   └── test_page_manager.c
└── assets/                 # Sample content
    └── books/              # Sample e-books
```

### Code Architecture

The application follows a modular design with clear separation of concerns:

```
┌─────────────────────────────────────────┐
│         Main Controller (main.c)        │
│    State machine & event coordination   │
└─────────────────────────────────────────┘
         │         │         │         │
         ▼         ▼         ▼         ▼
    ┌────────┐ ┌──────┐ ┌─────────┐ ┌──────────┐
    │SD Card │ │ Page │ │ Display │ │Navigation│
    │Manager │ │Manager│ │Controller│ │Controller│
    └────────┘ └──────┘ └─────────┘ └──────────┘
         │         │         │         │
         ▼         ▼         ▼         ▼
    ┌─────────────────────────────────────────┐
    │     Hardware Abstraction Layer (TAL)    │
    │  tkl_fs | tal_mem | EPD_4in26 | tdl_btn │
    └─────────────────────────────────────────┘
```

### Customization Options

#### 1. Display Settings

**Font Size** (in `display_controller.c`):
```c
// Change font for better readability or more text per page
#define DISPLAY_FONT Font16  // Options: Font12, Font16, Font20, Font24
```

**Page Layout** (in `display_controller.c`):
```c
#define HEADER_HEIGHT 30     // Adjust header size
#define FOOTER_HEIGHT 30     // Adjust footer size
#define MARGIN_LEFT 10       // Adjust left margin
#define MARGIN_RIGHT 10      // Adjust right margin
```

#### 2. Pagination Settings

**Characters Per Page** (in `page_manager.c`):
```c
// Automatically calculated based on display size and font
// Override by modifying page_manager_calculate_pages()
```

**Word Wrapping** (in `page_manager.c`):
```c
// Modify word wrapping algorithm in page_manager_get_current_page()
// Current: breaks at word boundaries
// Alternative: break at any character (for languages without spaces)
```

#### 3. Navigation Settings

**Button Timing** (in `navigation_controller.c`):
```c
TDL_BUTTON_CFG_T button_cfg = {
    .long_start_valid_time = 3000,    // Long press duration (ms)
    .long_keep_timer = 1000,          // Long press repeat rate
    .button_debounce_time = 50,       // Debounce time (ms)
    .button_repeat_valid_count = 2,   // Repeat threshold
    .button_repeat_valid_time = 500   // Repeat rate (ms)
};
```

**Button Mapping** (in `navigation_controller.c`):
```c
// Remap button functions by modifying navigation_button_handler()
// Example: swap next/previous buttons
```

#### 4. Sleep Settings

**Sleep Timeout** (in `main.c`):
```c
#define SLEEP_TIMEOUT_MS 30000  // 30 seconds (adjust as needed)
```

#### 5. File Handling

**Maximum File Size** (in `sd_card_manager.c`):
```c
#define MAX_FILE_SIZE (1024 * 1024)  // 1MB (increase if more RAM available)
```

**Supported Extensions** (in `sd_card_manager.c`):
```c
// Add support for other text formats
// Modify sd_manager_scan_books() to accept .md, .log, etc.
```

### Adding New Features

#### Example: Add Bookmark Support

1. **Modify Data Structures** (`main.c`):
```c
typedef struct {
    // ... existing fields ...
    int bookmark_page;  // Add bookmark field
} app_context_t;
```

2. **Add Save/Load Functions** (`main.c`):
```c
void save_bookmark(const char *filename, int page);
int load_bookmark(const char *filename);
```

3. **Add Button Handler** (`navigation_controller.c`):
```c
// Add new navigation event for bookmark
NAV_EVENT_SAVE_BOOKMARK
```

4. **Update Display** (`display_controller.c`):
```c
// Show bookmark indicator on display
void display_show_bookmark_icon(int x, int y);
```

#### Example: Add Multiple Font Support

1. **Modify Display Controller** (`display_controller.h`):
```c
typedef enum {
    FONT_SMALL,
    FONT_MEDIUM,
    FONT_LARGE
} font_size_t;

void display_set_font(font_size_t size);
```

2. **Implement Font Switching** (`display_controller.c`):
```c
void display_set_font(font_size_t size) {
    switch(size) {
        case FONT_SMALL:  current_font = &Font12; break;
        case FONT_MEDIUM: current_font = &Font16; break;
        case FONT_LARGE:  current_font = &Font20; break;
    }
}
```

3. **Add Button Control** (`navigation_controller.c`):
```c
// Long press button 2 to cycle fonts
```

### Testing

The project includes unit tests for core functionality:

```bash
# Build with tests enabled
tos.py build -DENABLE_TESTS=ON

# Run tests
./build/ebook_reader_tests
```

**Test Coverage:**
- SD card file operations
- Page pagination algorithms
- Display rendering
- Navigation event handling

### Debugging

**Enable Debug Logging:**
```c
// In app_default.config or via menuconfig
CONFIG_TAL_LOG_LEVEL=DEBUG
```

**Common Debug Points:**
```c
// Add debug prints in your code
PR_DEBUG("Current page: %d/%d", current_page, total_pages);
PR_DEBUG("Button pressed: %s", button_name);
PR_DEBUG("File size: %zu bytes", file_size);
```

**Monitor Serial Output:**
```bash
tos.py monitor
# Press Ctrl+] to exit
```

### Performance Optimization

**Memory Optimization:**
- Load books in chunks instead of full file
- Use smaller display buffer for partial refresh
- Free unused memory after initialization

**Speed Optimization:**
- Use partial refresh for page navigation
- Cache frequently accessed data
- Optimize pagination algorithm

**Power Optimization:**
- Increase sleep timeout
- Reduce display refresh frequency
- Use deep sleep mode when possible

### Contributing

To contribute improvements:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly on hardware
5. Submit a pull request

**Coding Standards:**
- Follow Linux kernel style (see `.clang-format`)
- 4-space indentation
- 120 character line limit
- Document all public functions
- Add unit tests for new features

## License

This project uses the Waveshare e-Paper driver library, which is provided under the MIT License.

## Support

For issues and questions:
- Check the TuyaOpen documentation
- Review the e-Paper driver documentation
- Check debug logs for error messages

## Credits

- E-Paper driver: Waveshare Electronics
- Platform: TuyaOpen
- Contest: Tuya Smart "智绘黑白，创享无限" E-Paper Development Challenge
