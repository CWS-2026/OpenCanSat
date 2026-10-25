# OpenCanSat

**An open-source ESP32-S3-based CanSat platform for environmental monitoring and flight data acquisition.**

## Overview

OpenCanSat is an open-source CanSat prototype that integrates an ESP32-S3 with motion, environmental, and GPS sensors. Its current implementation collects sensor readings and streams them directly to a computer over USB serial. A cross-platform Python ground station displays the incoming telemetry and automatically saves valid data packets to CSV files for subsequent analysis.

The project is under active development. The repository provides integrated firmware, individual sensor tests, wiring documentation, and editable mechanical CAD files. **End-to-end operation on physical hardware has not yet been verified.**

## Hardware

| Component | Model | Purpose |
|---|---|---|
| Microcontroller | ESP32-S3 | Sensor acquisition and USB serial telemetry |
| IMU | MPU6050 | 3-axis acceleration and angular velocity |
| Environmental sensor | BME280 | Temperature, humidity, and atmospheric pressure |
| GPS receiver | NEO-6M | Position, GPS altitude, and satellite count |

See the [hardware documentation](hardware/README.md), [pin mapping](hardware/pinout.md), and [bill of materials](hardware/BOM.csv).

## Features

- Integrated sampling of MPU6050, BME280, and NEO-6M data.
- One comma-separated `DATA` telemetry record approximately every second (115200-baud USB serial).
- Basic sensor-availability flags and GPS-fix indication; unavailable values are represented as `nan`.
- Terminal-only ground station for Windows, macOS, and Linux.
- **Automatic CSV recording on the ground-station computer**, with a UTC reception timestamp for each valid data packet.
- Individual Arduino sensor-test sketches and original mechanical CAD files.

**Communication scope:** the current system uses a **USB cable**, not a wireless radio link. It is intended for bench testing and tethered data acquisition. Untethered flight telemetry requires additional communication hardware and software.

## Repository Structure

```text
OpenCanSat/
├── README.md
├── LICENSE
├── firmware/
│   ├── README.md
│   ├── OpenCanSat.ino/
│   │   └── OpenCanSat.ino
│   └── tests/
│       ├── BME280/BME280.ino
│       ├── MPU6050/MPU6050.ino
│       └── NEO-6M/NEO-6M.ino
├── ground-station/
│   ├── README.md
│   ├── ground_station.py
│   └── requirements.txt
└── hardware/
    ├── README.md
    ├── LICENSE
    ├── BOM.csv
    ├── pinout.md
    ├── THIRD_PARTY_NOTICES.md
    ├── wiring/
    │   ├── OpenCanSat.fzz
    │   └── wiring_diagram.png
    └── mechanical/
        ├── OpenCanSat.f3d
        ├── OpenCanSat.step
        ├── OpenCanSat.stl
        ├── OpenCanSat.png
        └── OpenCanSat.mp4
```

## Getting Started

1. Connect the ESP32-S3 and sensors according to the [wiring diagram](hardware/wiring/wiring_diagram.png) and [pin mapping](hardware/pinout.md).
2. Follow the [firmware instructions](firmware/README.md) to install Arduino libraries and upload the integrated sketch.
3. Connect the board to your computer using a USB **data** cable.
4. Follow the [ground-station instructions](ground-station/README.md) to install Python dependencies and start the receiver.
5. Watch the live `INFO` and `DATA` records in the terminal. Valid `DATA` records are automatically saved under `ground-station/logs/`.

## Telemetry and Data Logging

The firmware emits startup/status messages prefixed with `INFO,` and measurement records prefixed with `DATA,`. A measurement record includes ESP32 uptime, sensor status flags, accelerometer and gyroscope measurements, temperature, humidity, pressure, and GPS information.

The ground station prints all received messages but writes **only valid `DATA` records** to CSV. Each saved row also includes the PC's UTC reception timestamp. See the [ground-station README](ground-station/README.md) for the complete column schema and file locations.

No SD-card or onboard persistent logging is implemented.

## Development Status

**Implemented in the repository:**

- Individual sensor test sketches and integrated sensor firmware.
- USB serial telemetry and terminal-based receiver with automatic CSV logging.
- Wiring diagram, pin mapping, BOM, mechanical CAD files, and licensing notices.

**Not yet verified or implemented:**

- Physical end-to-end test of all sensors, serial reception, and logged CSV values.
- Wireless telemetry for an untethered flight.
- Automatic flight-state detection and flight-test validation.

## Contributing

Issues, bug reports, documentation fixes, and pull requests are welcome. When reporting a hardware issue, include your ESP32-S3 board variant, Arduino ESP32 core version, wiring, and relevant serial logs where possible.

## License

- **Software and firmware:** [MIT License](LICENSE).
- **Original hardware designs:** [CERN Open Hardware Licence Version 2 – Permissive (CERN-OHL-P-2.0)](hardware/LICENSE).
- **Third-party Fritzing parts and other external materials:** retain their respective licenses. See [Third-Party Notices](hardware/THIRD_PARTY_NOTICES.md).
