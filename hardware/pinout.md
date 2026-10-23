# Pin Configuration

This document describes the pin assignments and communication interfaces used in OpenCanSat.

## Pin Mapping

| Component | Signal | ESP32-S3 Pin |
|---|---|---|
| MPU6050 | SDA | GPIO 8 |
| MPU6050 | SCL | GPIO 9 |
| BME280 | SDA | GPIO 8 |
| BME280 | SCL | GPIO 9 |
| NEO-6M | TX | GPIO 44 (RX) |
| NEO-6M | RX | GPIO 43 (TX) |

## Communication Interfaces

- **I2C:** MPU6050 and BME280
- **UART:** NEO-6M GPS
- **GPS Baud Rate:** 9600 bps

## Notes

- The MPU6050 and BME280 share the same I2C bus.
- GPS TX and RX must be connected to the corresponding ESP32-S3 RX and TX pins.
- GPIO 43 and GPIO 44 may conflict with UART0 serial debugging.
- Power connections must follow the voltage requirements of each module.

Pin assignments may be updated during development.
