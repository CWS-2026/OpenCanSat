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

## License

Original hardware designs developed by the
OpenCanSat contributors are licensed under the
CERN Open Hardware Licence Version 2 –
Permissive (CERN-OHL-P-2.0).

This license applies only to materials for which
the OpenCanSat contributors hold the necessary rights.

Third-party components and graphical assets,
including Fritzing parts, remain subject to their
respective licenses.

See [LICENSE](LICENSE) for the hardware license
and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
for third-party attribution and licensing information.
