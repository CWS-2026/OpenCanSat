<div align="center">

# 🛰️ OpenCanSat

**An open-source ESP32-S3 CanSat platform for environmental monitoring and flight data acquisition.**

**Wi-Fi Web Ground Station · Live Sensor Data · CSV Export · Open Hardware**

[**Get Started**](#quick-start) · [**Firmware**](firmware/README.md) · [**Hardware**](hardware/README.md) · [**3D Model Video**](hardware/mechanical/OpenCanSat.mp4)

</div>

![OpenCanSat enclosure preview](hardware/mechanical/OpenCanSat.png)

## Overview

**OpenCanSat** is an educational and experimental CanSat platform built around the **ESP32-S3**. It combines motion sensing, environmental measurements, and GPS in a compact, editable open-hardware design.

The ESP32-S3 creates its **own Wi-Fi network and hosts a web ground station**. A phone, tablet, or laptop can connect directly to the CanSat and open the dashboard in a browser—**no router, internet connection, or separate Python ground-station program required**.

The dashboard displays live measurements and lets users **download collected readings as a CSV file**. The firmware also prints measurements over USB serial for debugging.

> **Development status:** The Wi-Fi web-server firmware and hardware design files are available as a prototype. Flight range, sensor integration, power stability, and reliable operation during descent have **not yet been fully validated**.

## Key Features

| Area | Implementation |
|---|---|
| **Motion sensing** | MPU6050: 3-axis acceleration (g) and angular velocity (deg/s) |
| **Environmental sensing** | BME280: temperature (°C), humidity (%), pressure (hPa) |
| **GPS** | NEO-6M: latitude, longitude, altitude, satellites |
| **Wireless connection** | ESP32-S3 2.4 GHz Wi-Fi access point |
| **Web ground station** | Live browser dashboard served directly by the CanSat |
| **Data acquisition** | Nominal 5 Hz (one sample every 200 ms) |
| **CSV export** | Download readings collected by the browser |
| **Debugging** | USB serial output at 115200 baud |
| **Open hardware** | Fritzing wiring, pin map, BOM, Fusion 360 CAD and 3D models |

## How It Works

```text
 MPU6050 ── I2C ──┐
 BME280  ── I2C ──┼── ESP32-S3 / OpenCanSat
 NEO-6M  ── UART ─┘           │
                         Wi-Fi SoftAP
                         HTTP Web Server
                              │
                    Phone / Tablet / Laptop
                              │
                      Web Ground Station
                       ├── Live readings
                       └── Download CSV

 Optional: ESP32-S3 ── USB Serial ── Computer (debug logs)
```

### Wi-Fi Connection

| Setting | Value |
|---|---|
| Network name (SSID) | `ICanDo<NUMBER>` (e.g. `ICanDo1`) |
| Wi-Fi password | `cbsh@CS1` |
| Website | **http://opencansat.local/** |
| Fallback website | **http://192.168.4.1/** |
| Sampling interval | 200 ms (nominal 5 Hz) |

The Wi-Fi number can be fixed for each team's device by editing `AP_NUMBER_OVERRIDE` in the firmware. The default value `0` generates a number from the ESP32-S3 chip identifier. Some phones cannot resolve `.local` names on a local hotspot; use `192.168.4.1` in that case.

## Quick Start

### 1. Prepare the Hardware

Connect the MPU6050, BME280, and NEO-6M to the ESP32-S3 using the [wiring diagram](hardware/wiring/wiring_diagram.png) and [pin configuration](hardware/pinout.md). Confirm supply voltages and wiring before power-on.

### 2. Upload the Firmware

Install **Arduino IDE**, the **ESP32 by Espressif Systems** board package, **Adafruit BME280 Library** (and its dependencies), and **TinyGPSPlus**.

Open [`OpenCanSat.ino`](firmware/OpenCanSat.ino/OpenCanSat.ino), set the device number if needed, select the correct ESP32-S3 board and USB port, then compile and upload.

See the detailed [firmware instructions](firmware/README.md), including USB CDC settings and upload troubleshooting.

### 3. Connect Your Phone

1. Power the ESP32-S3.
2. Open Wi-Fi settings and select `ICanDo1` (or your device's actual `ICanDo` number).
3. Enter the password `cbsh@CS1`.
4. Keep the connection even if your phone says **"No internet"**.
5. Open **http://opencansat.local/**, or **http://192.168.4.1/** if the name does not resolve.

### 4. View and Export Data

The webpage shows live sensor readings and GPS status. Keep the webpage open during the experiment. Tap **Download CSV** to save the browser-collected data to your device.

**Important:** CSV export is **not continuous background storage**. Samples in the browser remain in memory until downloaded. The ESP32-S3 also keeps only a rolling **256-sample (~51-second) RAM buffer** to recover short interruptions; it does **not** write measurements permanently to SD card or flash. Lost or uncollected measurements cannot always be recovered.

## Hardware & Mechanical Design

| Component | Model | Purpose |
|---|---|---|
| Microcontroller | ESP32-S3 | Control, Wi-Fi, and web server |
| IMU | MPU6050 | Acceleration and angular velocity |
| Environmental sensor | BME280 | Temperature, humidity, pressure |
| GPS receiver | NEO-6M | Position, altitude, satellites |

![OpenCanSat wiring diagram](hardware/wiring/wiring_diagram.png)

[**Explore Wiring and CAD Files**](hardware/README.md) · [**View 3D Demonstration**](hardware/mechanical/OpenCanSat.mp4)

## Repository Structure

```text
OpenCanSat/
├── README.md
├── LICENSE
├── .gitignore
├── firmware/
│   ├── README.md
│   ├── OpenCanSat.ino/
│   │   └── OpenCanSat.ino
│   └── tests/
│       ├── BME280/
│       ├── MPU6050/
│       └── NEO-6M/
└── hardware/
    ├── README.md
    ├── LICENSE
    ├── BOM.csv
    ├── pinout.md
    ├── THIRD_PARTY_NOTICES.md
    ├── wiring/
    └── mechanical/
```

The **web ground station is embedded in the ESP32-S3 firmware**. The former desktop Python `ground-station/` directory is not required for this version.

## Development Status & Flight Limitations

**Implemented in firmware:** integrated sensor reading, Wi-Fi access point, onboard HTTP dashboard, short-term RAM history, browser CSV download, and USB serial logs.

**Still to validate:** successful compilation on the target Arduino ESP32 core, physical sensor readings, GPS fix, battery-powered operation, signal strength at the expected drop distance, and mechanical durability.

**Wi-Fi is not guaranteed to work reliably throughout a drop.** Range and packet reception depend on altitude, distance, antenna placement, rotation, enclosure material, interference, and the receiving phone. The current web solution does not provide reliable long-range telemetry or permanent onboard backup storage. Perform controlled range tests before any descent experiment.

## Contributing

Issues, documentation fixes, testing reports, and pull requests are welcome. Please include your board model, wiring, Arduino core version, and relevant serial output when reporting a problem.

## License

| Material | License |
|---|---|
| Software and firmware | [MIT License](LICENSE) |
| Original hardware designs | [CERN-OHL-P-2.0](hardware/LICENSE) |
| Third-party parts and graphics | Their respective original licenses; see [Third-Party Notices](hardware/THIRD_PARTY_NOTICES.md) |

---

<div align="center"><strong>OpenCanSat — Open Hardware. Open Software. Open Exploration.</strong></div>
