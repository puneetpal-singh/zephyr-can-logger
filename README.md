# Zephyr CAN Logger

Embedded Zephyr RTOS application for an ESP32-S3 board that receives CAN frames and displays live RX information on a small ST7735R/LVGL screen.

## What It Demonstrates

- Zephyr RTOS application structure
- ESP32-S3 TWAI/CAN receive path
- CAN transceiver integration
- LVGL display UI on ST7735R SPI hardware
- UART/debug logging and PC-side CAN test workflow

## Hardware

- ESP32-S3 DevKitC
- CAN transceiver connected to ESP32-S3 TWAI pins
- ST7735R SPI display
- USB-to-CAN adapter for test traffic

## Build

```bash
west build -b esp32s3_devkitc/esp32s3/procpu -p always .
west flash
```

## Test Traffic

```bash
sudo ip link set can0 down
sudo ip link set can0 type can bitrate 500000
sudo ip link set can0 up
cansend can0 123#DEADBEEF00112233
```

## Repository Scope

Build artifacts, board-local output, generated files, and local environment folders are excluded from Git.

## Skills Shown

`Zephyr RTOS` | `ESP32-S3` | `CAN Bus` | `TWAI` | `LVGL` | `ST7735R` | `Embedded C`
