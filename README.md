# 🛰️ OpenCanSat

**An open-source ESP32-S3-based CanSat platform for environmental monitoring and flight data acquisition.**

An accessible and modular satellite-inspired platform integrating environmental sensing, motion tracking, GPS positioning, and real-time data acquisition.

**ESP32-S3** · **Arduino** · **Python** · **Open-Source Hardware**

---

![OpenCanSat 3D Model](hardware/mechanical/OpenCanSat.png)

**[Explore Hardware](hardware/README.md)** · **[View Firmware](firmware/README.md)** · **[Ground Station](ground-station/README.md)** · **[Watch 3D Demonstration](hardware/mechanical/OpenCanSat.mp4)**

---

## Overview

**OpenCanSat** is an open-source CanSat project designed to make satellite-inspired system development more accessible.

Powered by the ESP32-S3, the platform integrates environmental, motion, and GPS sensors into a compact system for collecting and analyzing flight-related data.

The current prototype transmits measurements directly to a computer through USB serial communication. A Python-based ground station receives the telemetry, displays live logs, and automatically saves measurement data in CSV format.

The repository includes integrated firmware, sensor testing programs, hardware documentation, wiring diagrams, and editable mechanical designs.

## Key Features

| Feature | Description |
|---|---|
| **Environmental Monitoring** | Temperature, humidity, and atmospheric pressure using BME280 |
| **Motion Tracking** | 3-axis acceleration and angular velocity using MPU6050 |
| **GPS Positioning** | Geographic coordinates, GPS altitude, and satellite information using NEO-6M |
| **Real-Time Telemetry** | Periodic sensor data transmission through USB serial |
| **Ground Station** | Cross-platform Python terminal receiver for Windows, macOS, and Linux |
| **CSV Data Logging** | Automatic recording of received telemetry for further analysis |
| **Open Hardware** | Wiring diagrams, component documentation, and mechanical CAD files |

## System Architecture

```text
                  OpenCanSat
            ┌──────────────────┐
            │     ESP32-S3     │
            └────────┬─────────┘
                     │
        ┌────────────┼────────────┐
        │            │            │
    ┌───▼────┐  ┌────▼───┐  ┌─────▼────┐
    │ MPU6050│  │ BME280 │  │ NEO-6M   │
    │ Motion │  │Environment│ GPS      │
    └────────┘  └────────┘  └──────────┘
                     │
              USB Serial
               115200 baud
                     │
            ┌────────▼─────────┐
            │  Ground Station  │
            │  Python Terminal │
            └────────┬─────────┘
                     │
             ┌───────┴────────┐
             │                │
         Live Logs       CSV Data Files
```

**Current communication:** USB serial. Wireless telemetry for untethered flight is not yet implemented.

## Hardware Configuration

| Component | Model | Interface |
|---|---|---|
| Microcontroller | ESP32-S3 | USB / I2C / UART |
| Inertial Measurement Unit | MPU6050 | I2C |
| Environmental Sensor | BME280 | I2C |
| GPS Receiver | NEO-6M | UART |

The MPU6050 and BME280 share an I2C bus on GPIO8 and GPIO9. The GPS receiver communicates through GPIO44 (RX) and GPIO43 (TX).

For detailed documentation, see the [hardware specifications](hardware/README.md), [pin configuration](hardware/pinout.md), and [bill of materials](hardware/BOM.csv).

### Wiring Diagram

![OpenCanSat Wiring Diagram](hardware/wiring/wiring_diagram.png)

[View Editable Fritzing Project](hardware/wiring/OpenCanSat.fzz)

## Quick Start

### 1. Set Up the Hardware

Connect the ESP32-S3 and peripheral modules according to the [wiring diagram](hardware/wiring/wiring_diagram.png).

Verify the electrical requirements of your specific modules before powering the system.

### 2. Upload the Firmware

Install the ESP32 board support package in Arduino IDE and the following libraries:

- Adafruit BME280 Library (including required dependencies)
- TinyGPSPlus

Open the [main firmware](firmware/OpenCanSat.ino/OpenCanSat.ino), select the appropriate ESP32-S3 board and USB configuration, then compile and upload.

See the [firmware documentation](firmware/README.md) for full instructions.

### 3. Launch the Ground Station

Navigate to the `ground-station` directory.

**Windows (PowerShell)**

```powershell
py -m pip install -r requirements.txt
py ground_station.py
```

**macOS / Linux**

```bash
python3 -m pip install -r requirements.txt
python3 ground_station.py
```

The ground station automatically selects the serial port if only one is detected. When multiple ports are available, it prompts you to choose one.

### 4. Receive and Save Telemetry

Incoming telemetry is displayed in the terminal:

```text
[GROUND] Connected: COM3 @ 115200 baud
[GROUND] Listening (Ctrl+C to stop)...

[15:30:01] INFO,OpenCanSat boot
[15:30:02] DATA,1000,1,0.000,0.000,1.000,...
```

The output above is illustrative.

Valid `DATA` records are **automatically saved as CSV files** in:

```text
ground-station/logs/
└── flight_YYYYMMDD_HHMMSS_microseconds.csv
```

Each CSV record includes a UTC reception timestamp, ESP32 uptime, sensor validity flags, and measured values.

No onboard SD-card logging is required.

See the [Ground Station Documentation](ground-station/README.md) for additional instructions.

## Repository Structure

```text
OpenCanSat/
├── README.md
├── LICENSE
│
├── firmware/
│   ├── README.md
│   ├── OpenCanSat.ino/
│   │   └── OpenCanSat.ino
│   └── tests/
│       ├── BME280/
│       ├── MPU6050/
│       └── NEO-6M/
│
├── ground-station/
│   ├── README.md
│   ├── ground_station.py
│   └── requirements.txt
│
└── hardware/
    ├── README.md
    ├── LICENSE
    ├── BOM.csv
    ├── pinout.md
    ├── THIRD_PARTY_NOTICES.md
    ├── wiring/
    └── mechanical/
```

## Mechanical Design

The CanSat enclosure was designed using Autodesk Fusion 360.

The repository provides editable CAD files, a 3D printing mesh, model previews, and a demonstration video.

**[View Mechanical Design Files](hardware/mechanical/)** · **[Watch Demonstration](hardware/mechanical/OpenCanSat.mp4)**

The enclosure remains a prototype and requires physical fit, structural, and flight-related validation.

## Development Status

| Development Area | Status |
|---|---|
| Individual sensor testing sketches | Implemented |
| Integrated ESP32-S3 firmware | Implemented |
| USB serial telemetry | Implemented |
| Terminal-based ground station | Implemented |
| Automatic CSV logging | Implemented |
| Wiring and mechanical CAD documentation | Available |
| Physical end-to-end hardware validation | Pending |
| Wireless telemetry | Not implemented |
| Flight-state detection | Not implemented |

**Note:** Implementation status indicates that code or design files are available in the repository. Complete operation on the physical ESP32-S3 prototype has not yet been verified.

## Contributing

Contributions are welcome!

You can support OpenCanSat by reporting issues, improving documentation, testing hardware compatibility, or contributing code and design improvements.

Please use GitHub Issues for bug reports and feature suggestions, and Pull Requests for proposed changes.

## License

OpenCanSat uses separate licenses for software and hardware.

| Material | License |
|---|---|
| Software and firmware | [MIT License](LICENSE) |
| Original hardware designs | [CERN-OHL-P-2.0](hardware/LICENSE) |
| Third-party materials | Respective original licenses |

See [Third-Party Notices](hardware/THIRD_PARTY_NOTICES.md) for attribution and additional licensing information.

---

**OpenCanSat — Open Hardware. Open Software. Open Exploration.**
