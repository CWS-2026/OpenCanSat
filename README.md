# 🛰️ OpenCanSat

<p align="center">
  <strong>Open-source ESP32-S3 CanSat platform for environmental monitoring and flight-data acquisition</strong>
</p>

<p align="center">
  <a href="firmware/README.md">Firmware</a> ·
  <a href="ground-station/README.md">Ground Station</a> ·
  <a href="hardware/README.md">Hardware</a> ·
  <a href="hardware/mechanical/OpenCanSat.mp4">3D Model Video</a>
</p>

![OpenCanSat 3D enclosure preview](hardware/mechanical/OpenCanSat.png)

## Overview

**OpenCanSat** is an ESP32-S3-based experimental CanSat system built around motion sensing, atmospheric measurements, GPS tracking, and real-time telemetry. Its integrated firmware streams sensor readings **wirelessly over 2.4 GHz Wi-Fi UDP** to a Python ground station. The ground station shows terminal logs and **automatically saves valid telemetry to CSV files**. USB serial remains available for bench debugging.

The Wi-Fi system works without a router or an internet connection: the ESP32-S3 hosts an access point, and a Windows, macOS, or Linux laptop connects to it. **Hardware integration, flight range, power supply, and reliable operation during descent have not yet been validated.**

## Features

| Area | Current implementation |
|---|---|
| Motion | MPU6050: 3-axis acceleration and angular velocity |
| Environment | BME280: temperature, humidity, pressure |
| Position | NEO-6M: latitude, longitude, GPS altitude, satellites |
| Wireless telemetry | ESP32-S3 Wi-Fi SoftAP → UDP ground station, **5 Hz nominal** |
| Debug interface | Native USB serial, 115200 baud |
| Ground station | Terminal-only Python receiver, Windows/macOS/Linux |
| Data recording | **Automatic CSV logging on the ground-station PC** |
| Open hardware | Fritzing wiring, pin mapping, BOM, Fusion 360 model and printable mesh |

## How It Works

```text
  MPU6050 ── I2C ──┐
  BME280  ── I2C ──┼── ESP32-S3 / OpenCanSat
  NEO-6M  ── UART ─┘            │
                           Wi-Fi SoftAP
                         UDP telemetry, 5 Hz
                                │
                            Laptop (PC)
                                │
                     Python Ground Station
                         ├── Live terminal
                         └── CSV in logs/

  Optional: ESP32-S3 ── USB Serial ── PC (bench debugging)
```

### Wireless settings

| Parameter | Default |
|---|---|
| Wi-Fi SSID | `OpenCanSat` |
| Wi-Fi password | `OpenCanSat1234` (**development-only; change before field use**) |
| ESP32-S3 AP address | `192.168.4.1` |
| UDP destination | Discovered from the ground station's `HELLO` message to port `4210` |
| Telemetry interval | 200 ms (5 Hz nominal) |
| CSV path | `ground-station/logs/flight_*.csv` |

The telemetry schema remains **`DATA` + 17 comma-separated fields**; the CSV adds `received_at_utc`. GPS refreshes at its own receiver rate, so repeated GPS readings can occur between 5 Hz telemetry packets.

## Quick Start

1. Wire the ESP32-S3 and sensors using the [pinout](hardware/pinout.md) and [wiring diagram](hardware/wiring/wiring_diagram.png). Verify power and module voltage requirements.
2. Install the Arduino ESP32 board package, **Adafruit BME280 Library** (and dependencies), and **TinyGPSPlus**. Open [`firmware/OpenCanSat.ino/OpenCanSat.ino`](firmware/OpenCanSat.ino/OpenCanSat.ino) in Arduino IDE, select your ESP32-S3 board and native USB CDC settings, then compile and upload.
3. Power the ESP32-S3. On your laptop, connect to the `OpenCanSat` Wi-Fi network. Keep the laptop connected even if the network has **no internet access**.
4. From the `ground-station/` directory, start the ground station:

   **Windows:**
   ```powershell
   py ground_station.py
   ```

   **macOS / Linux:**
   ```bash
   python3 ground_station.py
   ```

5. Watch `INFO`/`DATA` messages. The station records each valid `DATA` packet immediately to `ground-station/logs/`. Press `Ctrl+C` to stop.

To use USB instead, install `requirements.txt` and run `ground_station.py --transport serial`. See the [ground-station instructions](ground-station/README.md).

## Hardware

| Component | Model | Purpose |
|---|---|---|
| Main controller | ESP32-S3 | Sensors and Wi-Fi UDP telemetry |
| IMU | MPU6050 | Acceleration / angular velocity |
| Environment | BME280 | Temperature / humidity / pressure |
| GPS | NEO-6M | Position and altitude |

![OpenCanSat wiring diagram](hardware/wiring/wiring_diagram.png)

[CAD files and 3D model video](hardware/README.md) · [Bill of materials](hardware/BOM.csv) · [Pin mapping](hardware/pinout.md)

## Repository Layout

```text
OpenCanSat/
├── README.md
├── LICENSE
├── firmware/
│   ├── README.md
│   ├── OpenCanSat.ino/
│   │   └── OpenCanSat.ino
│   └── tests/
│       ├── BME280/
│       ├── MPU6050/
│       └── NEO-6M/
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
    └── mechanical/
```

## Flight Readiness & Range Limitations

**Wi-Fi being implemented does not mean it is flight-qualified.** Real usable range depends on the board antenna, enclosure, orientation/rotation, interference, line of sight, and receiving laptop. A brief connection on the ground is not proof of reliable reception throughout a drop. This version has **no retransmission or onboard storage**; packets lost over the air cannot be recovered afterward.

**Before any drop:**

1. Add and verify a properly regulated **independent power source**; the current BOM only lists the controller and sensors, not a flight battery.
2. Confirm normal operation after disconnecting USB, then measure packet gaps and reception stability with both devices stationary.
3. Repeat communication tests at increasing separation up to at least the **maximum expected slant range**, with the real enclosure fitted and with multiple orientations.
4. Check the enclosure, parachute/recovery setup, environmental safety, and applicable site/competition rules.
5. If continuous telemetry is critical, consider a **suitable long-range radio such as LoRa** and/or onboard nonvolatile logging (neither is currently implemented).

The terminal reports **estimated** packet gaps from `device_ms`; these are diagnostic estimates, not guaranteed radio loss measurements. Do not choose a flight altitude based on an assumed Wi-Fi range.

## Development Status

| Item | Status |
|---|---|
| Integrated sensor firmware | Code available |
| Wi-Fi SoftAP / UDP wireless telemetry | Code available; physical test pending |
| USB debugging | Code available; physical test pending |
| Cross-platform ground station | Code available |
| CSV recording + link-gap estimates | Code available; PC-side testing possible |
| Wiring, pin map, mechanical CAD | Available |
| Flight battery / self-powered operation | Not documented or validated |
| Sustained flight-range qualification | Not validated |
| LoRa / other long-range telemetry | Not implemented |
| Onboard recovery of lost data | Not implemented |

## Contributing

Feedback, test results, bug reports, documentation improvements, and pull requests are welcome. When filing an issue, please include your ESP32-S3 board revision, Arduino-ESP32 core version, relevant wiring, and serial/ground-station logs.

## License

- [MIT License](LICENSE): project software and firmware.
- [CERN-OHL-P-2.0](hardware/LICENSE): original hardware designs owned by project contributors.
- [Third-Party Notices](hardware/THIRD_PARTY_NOTICES.md): third-party Fritzing parts and other materials retain their respective rights and license conditions.

---

*OpenCanSat — Open Hardware. Open Software. Open Exploration.*
