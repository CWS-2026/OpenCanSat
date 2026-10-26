# OpenCanSat Hardware

Hardware documentation for the ESP32-S3 OpenCanSat prototype: sensors, wiring, 3D enclosure design, and current limitations affecting **Wi-Fi telemetry during descent**.

## Components

| Component | Model | Purpose |
|---|---|---|
| Microcontroller | ESP32-S3 | Sensor control and built-in **2.4 GHz Wi-Fi** |
| IMU | MPU6050 | Acceleration and angular velocity |
| Environmental sensor | BME280 | Temperature, humidity, pressure |
| GPS | NEO-6M | Latitude, longitude, GPS altitude |

[Bill of Materials](BOM.csv) · [Pin Configuration](pinout.md)

**Important:** the current BOM lists the controller and sensors but **does not specify a flight battery, power regulator, parachute, or long-range radio**. These must be planned and validated before untethered use.

## Wiring Diagram

![OpenCanSat wiring diagram](wiring/wiring_diagram.png)

- [Editable Fritzing project](wiring/OpenCanSat.fzz)
- [Wiring diagram image](wiring/wiring_diagram.png)
- [Detailed pin assignments](pinout.md)

### Signal Pins

| Signal | ESP32-S3 GPIO |
|---|---|
| MPU6050 + BME280 I2C SDA | GPIO8 |
| MPU6050 + BME280 I2C SCL | GPIO9 |
| GPS TX → ESP32 UART RX | GPIO44 |
| GPS RX ← ESP32 UART TX | GPIO43 |

GPS baud: **9600**. USB serial debugging: **115200**. Note that GPIO43/44 may conflict with UART0 on some ESP32-S3 boards; configure native USB CDC if supported and check the exact module's power and logic voltage requirements.

## Wireless Telemetry Hardware

The present firmware uses the **ESP32-S3's built-in Wi-Fi radio**, with no separate LoRa module. The ESP32-S3 hosts a Wi-Fi access point; a laptop connects to it and receives UDP packets through the [ground station](../ground-station/README.md). This does **not** require internet access.

**The wireless range is unverified.** Actual reception depends on radio path, antenna type and placement, enclosure material, device orientation during rotation, receiving laptop, interference, and flight distance. Avoid surrounding the antenna with metal, batteries, or other obstructing materials; do not assume that a CAD enclosure is radio-transparent without testing. Use permitted transmit-power settings for the test location.

For descent testing, verify radio reception at the **maximum expected separation**, including multiple CanSat orientations, while using the actual enclosure and independent power supply. Continuous long-range telemetry may require an additional radio (for example, an appropriately configured LoRa solution) and a matching ground receiver; neither is included in the current BOM.

The current firmware **does not write to onboard storage**, so lost UDP packets will not be available after landing unless an independent logger is added.

## Mechanical Design

The enclosure was designed in Autodesk Fusion 360. Editable CAD, 3D printing files, preview image, and demonstration video are provided below.

### 3D Model Preview

![OpenCanSat 3D enclosure](mechanical/OpenCanSat.png)

### Video Demonstration

**[▶ Watch the OpenCanSat 3D Model Video](mechanical/OpenCanSat.mp4)**

### Design Files

| File | Purpose |
|---|---|
| [OpenCanSat.f3d](mechanical/OpenCanSat.f3d) | Editable native Fusion 360 CAD |
| [OpenCanSat.3mf](mechanical/OpenCanSat.3mf) | 3D manufacturing model |
| [OpenCanSat.stl](mechanical/OpenCanSat.stl) | 3D printing mesh |
| [OpenCanSat.png](mechanical/OpenCanSat.png) | Design preview image |
| [OpenCanSat.mp4](mechanical/OpenCanSat.mp4) | 3D model demonstration video |

The CAD is a **prototype**, not a validated flight-ready enclosure. Print tolerance, structural integrity, component mounting, antenna clearance, thermal management, and recovery-system compatibility require physical checks. The current repository does **not** include a `.step` file; if a STEP export is uploaded later, it can be added to this table.

## Firmware and Ground Station

- [ESP32-S3 firmware](../firmware/README.md): integrated sensors, 5 Hz Wi-Fi UDP telemetry, optional USB serial.
- [Ground station](../ground-station/README.md): cross-platform UDP/USB receiver, live terminal output, **CSV logging**, and estimated packet-gap diagnostics.

## Development Status

**Available:** Fritzing project/image, pin map, current component list, Fusion 360 CAD, 3MF/STL, PNG preview, and MP4 demonstration.

**Not yet flight-validated:** actual assembly, independent power, antenna performance during descent, Wi-Fi range, case robustness, GPS fix in flight, and data completeness. Wireless code availability is not proof of a reliable RF link.

## Licensing and Attribution

Original hardware designs owned by the OpenCanSat contributors are licensed under [CERN Open Hardware Licence Version 2 – Permissive (CERN-OHL-P-2.0)](LICENSE).

Third-party Fritzing symbols, community components, and other graphical materials retain their respective rights and licensing terms; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Software and firmware are covered by the repository's [MIT License](../LICENSE).
