# Quick Start Guide - Network Novel Reader

## 5-Minute Setup

### Step 1: Configure WiFi and Novel URL

Edit `examples/EPD_4in26_network_novel.c` (lines 30-35):

```c
#define WIFI_SSID "YourWiFiName"        // ← Change this
#define WIFI_PASSWORD "YourPassword"     // ← Change this
#define NOVEL_URL "http://example.com/novel.txt"  // ← Change this
```

### Step 2: Build and Flash

```bash
# Setup environment
source export.sh

# Configure (select T5AI platform and this example)
tos.py config

# Build
tos.py build

# Flash to device
tos.py flash

# Monitor output
tos.py monitor
```

### Step 3: Use the Reader

1. **Power on** - Device connects to WiFi automatically
2. **Wait** - Novel downloads (watch serial monitor)
3. **Read** - First page displays automatically
4. **Navigate**:
   - **Short press button** → Next page
   - **Long press button (3s)** → Previous page

## Testing with Sample Novel

For quick testing, use a public domain text:

```c
// Pride and Prejudice from Project Gutenberg
#define NOVEL_URL "http://www.gutenberg.org/files/1342/1342-0.txt"
```

Or create your own `novel.txt` and host it locally:

```bash
# On your computer, create a simple HTTP server
python3 -m http.server 8000

# Then use your computer's IP
#define NOVEL_URL "http://192.168.1.100:8000/novel.txt"
```

## Expected Serial Output

```
[NOTICE] EPD Network Novel Reader Start
[NOTICE] Initializing hardware...
[NOTICE] Initializing e-Paper...
[NOTICE] Initializing network...
[NOTICE] Connecting to WiFi: YourWiFiName
[NOTICE] Network connected
[NOTICE] Network initialized successfully
[NOTICE] Fetching novel from: http://...
[NOTICE] Fetched 50000 bytes, 40 pages
[NOTICE] Initializing button...
[NOTICE] Button initialized
[NOTICE] Displaying page 1/40
[NOTICE] Entering main loop. Use button to navigate:
[NOTICE]   Short press = Next page
[NOTICE]   Long press = Previous page
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| WiFi won't connect | Check SSID/password, ensure 2.4GHz network |
| Download fails | Verify URL in browser first, use HTTP not HTTPS |
| Button doesn't work | Check BUTTON_NAME defined in board config |
| Display blank | Check e-paper connections, verify SPI pins |

## Next Steps

- Adjust text layout in code (CHARS_PER_LINE, LINES_PER_PAGE)
- Change font size (&Font12, &Font16, &Font20, &Font24)
- Add more buttons for additional features
- Implement bookmarks or chapter navigation

## Button Configuration

The button is defined in your board configuration. Common locations:

- `boards/T5AI/<your_board>/board_com_api.c`
- Look for `BUTTON_NAME` definition

Example:
```c
#define BUTTON_NAME "button_1"  // Usually GPIO button
```

## File Size Limits

- Maximum novel size: **1MB**
- Recommended: **100-500KB** for best performance
- Larger files take longer to download and use more memory

## Display Specifications

- Resolution: 800x480 pixels
- Current settings: 50 chars/line × 25 lines/page
- Font: 16pt (adjustable)
- Refresh time: ~2 seconds per page

## Support

For more details, see:
- `README.md` - Full documentation
- `README_CN.md` - 中文文档
- TuyaOpen documentation
- Example code comments
