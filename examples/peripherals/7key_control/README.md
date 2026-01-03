# 7-Key Control Example

This example demonstrates how to use 7 buttons for control input on TuyaOpen platform.

## Hardware Connection

| Button | Function | GPIO Pin | Connection |
|--------|----------|----------|------------|
| COM    | Common   | GND      | Connect to GND |
| UP     | Up       | P27      | Connect to MCU IO |
| DOWN   | Down     | P31      | Connect to MCU IO |
| LEFT   | Left     | P36      | Connect to MCU IO |
| RIGHT  | Right    | P30      | Connect to MCU IO |
| MID    | Middle   | P37      | Connect to MCU IO |
| SET    | Set      | P32      | Connect to MCU IO |
| RET    | Return   | P39      | Connect to MCU IO |

## Features

- Detects single click events for all 7 buttons
- Detects long press events for all 7 buttons
- Configurable debounce time and long press duration
- Active low button logic (pressed = LOW)

## Build and Flash

```bash
# Configure the project
tos.py config

# Build
tos.py build

# Flash to device
tos.py flash

# Monitor output
tos.py monitor
```

## Expected Output

When you press buttons, you should see output like:
```
UP: single click
DOWN: single click
LEFT: long press
...
```
