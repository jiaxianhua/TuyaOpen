# Project Structure

## Top-Level Organization

```
tuyaopen/
├── apps/           # Complete application examples
├── boards/         # Board-specific configurations and drivers
├── examples/       # Feature-specific examples
├── platform/       # Platform abstraction and toolchains
├── src/            # Core SDK source code
├── tools/          # Build tools and utilities
└── docs/           # Documentation and images
```

## Key Directories

### `/apps` - Application Examples
Complete, ready-to-run applications organized by category:
- `games/` - LVGL-based games
- `micropython/` - MicroPython applications
- `tuya.ai/` - AI-powered applications (chatbots, robots, emoji displays)
- `tuya_cloud/` - Cloud-connected demos (camera, switch, weather)
- `tuya_t5_pixel/` - LED pixel matrix applications
- `tuya_t5_pocket/` - Pocket device applications

### `/boards` - Board Support Packages
Platform and board-specific code:
- `<PLATFORM>/<BOARD>/` - Board configuration, drivers, pin definitions
- Each platform has `Kconfig`, `OS_SERVICE_Kconfig`, `TKL_Kconfig`
- `config/` subdirectories contain default configurations

### `/examples` - Feature Examples
Focused examples demonstrating specific functionality:
- `get-started/` - Basic project templates
- `peripherals/` - Hardware drivers (GPIO, I2C, SPI, ADC, etc.)
- `protocols/` - Network protocols (HTTP, MQTT, TCP)
- `graphics/` - LVGL UI examples
- `multimedia/` - Audio (ASR, TTS, recorder, speaker)
- `system/` - OS primitives (threads, queues, mutexes)
- `wifi/` - Wi-Fi operations (AP, STA, scan)
- `ble/` - Bluetooth examples

### `/src` - SDK Core
Modular SDK components:
- `tal_*` - Tuya Abstraction Layer (OS, network, drivers, security)
- `lib*` - Third-party libraries (cJSON, HTTP, MQTT, TLS, LVGL, lwIP)
- `peripherals/` - Hardware peripheral drivers
- `tuya_cloud_service/` - Cloud connectivity services
- `tuya_ai_basic/` - AI service foundations
- `common/` - Shared utilities

### `/platform` - Platform Support
Platform-specific implementations:
- `<PLATFORM>/` - Platform SDK, toolchain, build scripts
- `platform_config.yaml` - Platform registry
- `tools/` - Cross-platform toolchains

### `/tools` - Build Infrastructure
- `cli_command/` - CLI command implementations
- `kconfiglib/` - Kconfig tools and scripts
- `cmake/` - CMake utilities
- `tyutool/` - Tuya utility tool (submodule)
- `app_template/` - Templates for new applications
- `board_template/` - Templates for new boards

## File Naming Conventions

- **C source**: `snake_case.c`
- **Headers**: `snake_case.h`
- **CMake**: `CMakeLists.txt` in each component directory
- **Config**: `Kconfig` for configuration options, `app_default.config` for defaults

## Application Structure

Each application/example follows this pattern:
```
<app_name>/
├── CMakeLists.txt          # Build configuration
├── Kconfig                 # Configuration options (optional)
├── app_default.config      # Default configuration
├── README.md               # Documentation
├── src/                    # Source files
├── include/                # Public headers (optional)
├── config/                 # Additional configs (optional)
└── assets/                 # Resources (optional)
```

## Entry Points

- **RTOS platforms**: `tuya_app_main()` function
- **Linux**: `main(int argc, char *argv[])` function
- **Common pattern**: `user_main()` contains actual application logic

## Configuration System

- **Kconfig files**: Define configuration options hierarchically
- **app_default.config**: Default values for an application
- **Build output**: `build/cache/using.config` and `build/include/tuya_kconfig.h`
