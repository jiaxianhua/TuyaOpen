# Technology Stack

## Build System

- **Primary**: CMake 3.16+ with custom toolchain files per platform
- **CLI Tool**: `tos.py` - Python-based command-line interface for all operations
- **Configuration**: Kconfig-based configuration system (menuconfig)

## Languages & Standards

- **C/C++**: Primary development language
- **Python 3.6+**: Build tools and scripts
- **Code Style**: Linux kernel style (see `.clang-format`)
  - 4-space indentation
  - 120 character line limit
  - Braces on same line for control statements, new line for functions
  - Pointer alignment right (`int *ptr`)

## Core Libraries & Frameworks

- **LVGL**: Graphics library (v8 and v9 support)
- **lwIP**: Lightweight TCP/IP stack
- **mbedTLS**: TLS/SSL implementation
- **cJSON**: JSON parsing
- **coreMQTT/coreHTTP**: AWS IoT Core libraries
- **MicroPython**: Optional embedded Python support
- **FlashDB/littlefs**: Key-value storage and filesystem

## Common Commands

### Environment Setup
```bash
# Activate virtual environment and install dependencies
source export.sh  # Linux/Mac
export.bat        # Windows

# Exit environment
exit
```

### Build Operations
```bash
# Configure project (interactive menu)
tos.py config

# Build project
tos.py build

# Clean build artifacts
tos.py clean

# Flash to device
tos.py flash

# Monitor serial output
tos.py monitor

# Create new project/app
tos.py new

# Check code formatting
tos.py check

# Update platform/dependencies
tos.py update
```

### CMake Build (Advanced)
```bash
# Out-of-source build required
mkdir build && cd build
cmake -DTOS_PROJECT_PLATFORM=<platform> -DTOS_PROJECT_BOARD=<board> ..
cmake --build .
```

## Platform-Specific Details

- **Toolchains**: Platform-specific toolchains in `platform/tools/` and `platform/<PLATFORM>/`
- **Debug Serial**: UART configuration varies by platform (see README supported platforms table)
- **Framework Options**: `base` (default) or `arduino` via `TOS_FRAMEWORK` variable

## Dependencies

All Python dependencies managed via `requirements.txt` and installed automatically by `export.sh`.
