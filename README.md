# Zephyr CAN Logger

Zephyr RTOS application for an ESP32-S3 board that receives CAN frames and shows the latest frame plus RX statistics on an ST7735R/LVGL display.

## Hardware

- ESP32-S3 DevKitC
- CAN transceiver connected to ESP32-S3 TWAI
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
