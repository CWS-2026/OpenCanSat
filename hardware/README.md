# OpenCanSat Hardware

This directory contains the hardware design files and documentation for the OpenCanSat project.

## Hardware Overview

OpenCanSat is built around the ESP32-S3 microcontroller and integrates multiple sensors for environmental monitoring and flight data acquisition.

### Components

| Component | Model | Function |
|---|---|---|
| Microcontroller | ESP32-S3 | Main processing unit |
| IMU | MPU6050 | Acceleration and angular velocity |
| Environmental Sensor | BME280 | Temperature, humidity, and pressure |
| GPS | NEO-6M | Position and altitude |

## Wiring Diagram

The wiring diagram illustrates the connections between the ESP32-S3 and its peripheral modules.

![OpenCanSat Wiring Diagram](wiring/wiring_diagram.png)

- [Fritzing Project](wiring/OpenCanSat.fzz)
- [Wiring Diagram](wiring/wiring_diagram.png)

## Future Development

Additional hardware design files, including mechanical CAD models and circuit schematics, will be added as development progresses.
