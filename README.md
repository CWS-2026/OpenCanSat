# OpenCanSat

**An open-source ESP32-S3-based CanSat platform for environmental monitoring and flight data acquisition.**

## Overview

OpenCanSat is an open-source CanSat project designed for environmental monitoring and flight data acquisition.

Built around the ESP32-S3 microcontroller, the project aims to provide an accessible and modular platform for developing and experimenting with CanSat systems.

The project is currently under development, with an initial focus on hardware validation and individual sensor testing.

## Features

The current hardware configuration is designed to support:

- **Motion Sensing** — 3-axis acceleration and angular velocity measurement using the MPU6050.
- **Environmental Monitoring** — Temperature, humidity, and atmospheric pressure measurement using the BME280.
- **GPS Tracking** — Geographic position and altitude acquisition using the NEO-6M.
- **Modular Development** — Independent sensor testing and future system integration.

## Project Structure

| Directory | Description |
|---|---|
| [`firmware/`](firmware/) | Firmware source code and sensor testing programs |
| [`hardware/`](hardware/) | Hardware design files, wiring diagrams, and documentation |

For more information about the hardware configuration, see the [Hardware Documentation](hardware/README.md).

## Development Status

OpenCanSat is currently in the early stages of development.

### Current Development

- Individual sensor testing
- Hardware connection verification
- Initial wiring diagram design

### Planned Development

- Integrated sensor data acquisition
- Flight data logging
- Flight-state monitoring
- Mechanical design documentation

## Getting Started

1. Prepare an ESP32-S3 development board and the required sensors.
2. Refer to the [Hardware Documentation](hardware/README.md) for wiring information.
3. Open the sensor testing programs in `firmware/tests/` using Arduino IDE.
4. Install the required Arduino libraries.
5. Upload the firmware and verify the sensor output through the Serial Monitor.

## Contributing

Contributions, suggestions, and improvements are welcome.

Feel free to open an issue or submit a pull request.

## License

OpenCanSat uses separate licenses for software
and hardware:

- **Firmware:** MIT License
- **Original Hardware Designs:** CERN Open Hardware
  Licence Version 2 – Permissive (CERN-OHL-P-2.0)

Third-party materials retain their original
licensing terms.

For more information, see:

- [Software License](LICENSE)
- [Hardware License](hardware/LICENSE)
- [Third-Party Notices](hardware/THIRD_PARTY_NOTICES.md)
