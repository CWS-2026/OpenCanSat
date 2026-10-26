# OpenCanSat Firmware

Arduino firmware for the ESP32-S3. It reads **MPU6050**, **BME280**, and **NEO-6M** measurements and streams `DATA` packets to the [Python ground station](../ground-station/README.md) over **Wi-Fi UDP**. Native USB serial remains available for debugging. No onboard data logging is implemented.

## Source Files

| File | Purpose |
|---|---|
| [`OpenCanSat.ino/OpenCanSat.ino`](OpenCanSat.ino/OpenCanSat.ino) | Integrated Wi-Fi telemetry firmware |
| [`tests/MPU6050/MPU6050.ino`](tests/MPU6050/MPU6050.ino) | MPU6050 sensor test |
| [`tests/BME280/BME280.ino`](tests/BME280/BME280.ino) | BME280 sensor test |
| [`tests/NEO-6M/NEO-6M.ino`](tests/NEO-6M/NEO-6M.ino) | GPS test |

The current GitHub sketch directory is named `OpenCanSat.ino/`; Arduino IDE conventionally expects the sketch folder to be `OpenCanSat/` containing `OpenCanSat.ino`. Rename the folder if Arduino IDE requires it, and update links if the repository path changes.

## Prerequisites

- Arduino IDE with the Espressif **esp32** boards package and a suitable ESP32-S3 board selection.
- **Adafruit BME280 Library** and dependencies (Adafruit Unified Sensor / BusIO when required).
- **TinyGPSPlus** library.
- Native Arduino ESP32 `Wire`, `HardwareSerial`, `WiFi`, and `WiFiUdp` libraries (no separate UDP dependency).
- USB data cable for firmware upload and debugging; **independent regulated power is required for untethered flight**.

## Pinout

| Peripheral | Connection | ESP32-S3 |
|---|---|---|
| MPU6050 and BME280 | Shared SDA | GPIO8 |
| MPU6050 and BME280 | Shared SCL | GPIO9 |
| NEO-6M TX | UART receiver | GPIO44 |
| NEO-6M RX | UART transmitter | GPIO43 |

- MPU6050 address: `0x68`.
- BME280 addresses attempted: `0x76`, then `0x77`.
- GPS UART: **9600 baud**.
- Optional USB serial: **115200 baud**.

**USB warning:** GPIO43 and GPIO44 can be UART0 pins. The sketch uses them for GPS, so use the correct **native USB / USB CDC On Boot** setup for your specific board rather than relying on UART0 for logs. Check the [wiring documentation](../hardware/pinout.md).

## Wireless Telemetry Configuration

Edit these constants near the top of the `.ino` file if needed:

| Constant | Default | Meaning |
|---|---|---|
| `WIFI_SSID` | `OpenCanSat` | ESP32 access point name |
| `WIFI_PASSWORD` | `OpenCanSat1234` | Development password; **change before field use** |
| `WIFI_CHANNEL` | `6` | Fixed 2.4 GHz channel; choose based on local congestion |
| `UDP_PORT` | `4210` | ESP32 UDP listening port |
| `TELEMETRY_INTERVAL_MS` | `200` | Nominal **5 Hz** data rate |
| `PEER_TIMEOUT_MS` | `10000` | Ground-station `HELLO` timeout |

SoftAP address: `192.168.4.1/24`. The ESP32 runs its **own Wi-Fi access point**; internet/router access is unnecessary. The laptop connects to the AP, and the ground station sends `HELLO` every 2 s to `192.168.4.1:4210`. The ESP32 replies to the source IP and UDP port observed in that handshake.

This is a best-effort UDP telemetry stream. Lost datagrams are **not retransmitted**. The ESP32 does not save data that the ground station fails to receive.

## Upload and Run

1. Wire sensors and supply power according to the [hardware documentation](../hardware/README.md).
2. Open the [main sketch](OpenCanSat.ino/OpenCanSat.ino), select your ESP32-S3 board, and configure native USB CDC where applicable.
3. Compile and upload the sketch from Arduino IDE.
4. Connect your computer to `OpenCanSat` Wi-Fi. A warning that the network has **no internet** is expected.
5. Run the [ground station](../ground-station/README.md): `python3 ground_station.py` on macOS/Linux or `py ground_station.py` on Windows.
6. Watch incoming `INFO` and `DATA` messages. CSV records are saved on the PC to `ground-station/logs/`.

Optional USB debugging: `ground_station.py --transport serial` (requires pyserial). Close Arduino Serial Monitor before opening the same USB serial port in Python.

## Telemetry Protocol

One UDP datagram contains one line. The data protocol is **unchanged** from the earlier serial-only firmware:

```text
DATA,device_ms,mpu_ok,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,bme_ok,temp_c,humidity_pct,pressure_hpa,gps_fix,latitude,longitude,gps_altitude_m,satellites
```

The literal transmitted `DATA,` line contains **17 values** after `DATA`, without a column header. The Python ground station adds `received_at_utc` when writing CSV (18 columns in total).

| Field group | Units / meaning |
|---|---|
| `device_ms` | ESP32 uptime in ms |
| `mpu_ok`, `bme_ok`, `gps_fix` | Validity flags, 0 or 1 |
| `ax_g`, `ay_g`, `az_g` | Acceleration in g; default MPU6050 full-scale ±2 g |
| `gx_dps`, `gy_dps`, `gz_dps` | Angular velocity in °/s; default full-scale ±250 °/s |
| `temp_c`, `humidity_pct`, `pressure_hpa` | °C, %RH, hPa |
| `latitude`, `longitude`, `gps_altitude_m` | Decimal degrees and GPS altitude (m) |
| `satellites` | Reported visible satellites, or 0 if stale/unavailable |

Unavailable readings are represented by `nan`; sensor access failures do not intentionally halt the main loop. Failed sensor initialization is retried approximately every 5 s.

**Sensor-rate note:** the firmware sends measurement records nominally every 200 ms, but the NEO-6M's GPS output may update more slowly. Several consecutive records can therefore repeat a valid GPS position. The MPU6050 ±2 g range can saturate under strong impacts; change sensor full-scale registers and scale factors together if higher acceleration range is required.

## Flight Reliability Limits

- The Wi-Fi connection depends on 2.4 GHz radio visibility and receiving-device antenna characteristics; no guaranteed range is claimed.
- The case, battery, and antenna placement can significantly affect reception. A USB-only power supply does not work after untethering.
- Loss of Wi-Fi means **irrecoverable data gaps** with this no-storage design.
- Do not rely on a successful short-range test as evidence that the link will work at the maximum drop distance or through rotation.
- Link-gap estimates displayed by the ground station are derived from `device_ms`, not RF measurements.

For higher-assurance flight data, use an appropriate long-range radio link and/or add onboard storage. These are **not included** in this firmware.

## Development Status

Integrated Wi-Fi UDP and USB debug code is provided; **compilation, flashing, outdoor reception range, battery operation, and end-to-end flight tests on the physical target have not been verified here**.

Software license: [MIT](../LICENSE). Hardware license and third-party notices: [hardware/](../hardware/README.md).
