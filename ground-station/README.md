# OpenCanSat Ground Station

**Terminal-only wireless telemetry receiver + automatic CSV logger** for Windows, macOS, and Linux. The default connection is **Wi-Fi UDP**; optional USB serial mode is also available for bench testing. No GUI, cloud service, or internet connection is required.

## What It Does

- Receives `INFO` status and `DATA` telemetry from ESP32-S3 over Wi-Fi UDP.
- Prints all received messages with the **PC's local time**.
- Automatically saves valid `DATA` packets to **CSV** using a separate **UTC reception timestamp**.
- Warns when no Wi-Fi telemetry is received and estimates packet gaps from ESP32 `device_ms` values.
- Continues supporting USB serial input with optional `pyserial`.

## Setup: Wi-Fi UDP (Default)

1. Upload the [wireless OpenCanSat firmware](../firmware/README.md) to the ESP32-S3 and power it.
2. Connect the laptop to the ESP32's **`OpenCanSat`** Wi-Fi network (development password: `OpenCanSat1234`). You should stay connected even though this network does not provide internet.
3. In a terminal, change into `ground-station/` and run:

**Windows PowerShell:**

```powershell
py ground_station.py
```

**macOS / Linux:**

```bash
python3 ground_station.py
```

The Python standard library is sufficient for UDP mode. The PC sends a `HELLO` datagram to `192.168.4.1:4210` every 2 s. The ESP32 sends telemetry back to the UDP source port chosen by the operating system. If the OS firewall prompts to allow incoming UDP for Python, allow it on the appropriate **private/trusted network**.

### Wireless Configuration

| Setting | Default |
|---|---|
| ESP32 Wi-Fi SSID | `OpenCanSat` |
| ESP32 AP IP | `192.168.4.1` |
| ESP32 UDP port | `4210` |
| Expected telemetry interval | `200 ms` (5 Hz nominal) |
| Link warning threshold | 3 s without a received UDP packet |
| CSV folder | `ground-station/logs/` |

## CSV Logging

The logger **always writes the CSV header** and then immediately writes and flushes each valid `DATA` record. Files are stored on the laptop, **not** on the ESP32:

```text
ground-station/logs/flight_YYYYMMDD_HHMMSS_microseconds.csv
```

CSV columns:

```csv
received_at_utc,device_ms,mpu_ok,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,bme_ok,temp_c,humidity_pct,pressure_hpa,gps_fix,latitude,longitude,gps_altitude_m,satellites
```

The CSV has **18 columns** (UTC receipt time plus **17 original firmware fields**). Status `INFO` packets are printed, but they do not become CSV measurement rows. Malformed `DATA` rows are ignored and reported. Some values may be `nan` when a sensor is unavailable or the GPS has no fix.

A CSV with only its header means no valid `DATA` packets were saved. If wireless packets disappear mid-flight, the missing readings **cannot be reconstructed** from this CSV.

## Link Gap Diagnostics

The program compares consecutive ESP32 `device_ms` values with the nominal interval (200 ms). It reports:

- `LINK GAP`: roughly how many expected telemetry samples are missing between successive received records.
- `LINK WARNING`: no incoming Wi-Fi UDP packets for the configured timeout.
- `LINK STATS`: total received valid samples and an **estimated**, not exact, loss percentage.

Delays in firmware, reboot, or out-of-order datagrams can distort these estimates. **This is not a received-signal-strength (RSSI) measurement and cannot prove a flight link is reliable.**

## Command-Line Options

```text
--transport wifi|serial      Wi-Fi UDP (default) or USB serial
--esp-ip ADDRESS             ESP32 IP (default 192.168.4.1)
--udp-port PORT              ESP32 UDP port (default 4210)
--expected-interval-ms N     Expected DATA interval, default 200
--warn-after SECONDS         Warn when packets stop, default 3
--output-dir PATH            CSV output directory
--port PORT                  USB serial device (serial mode)
--baud RATE                  USB baud, default 115200
--list-ports                 List available USB serial devices (needs pyserial)
```

**Windows USB debugging:**

```powershell
py -m pip install -r requirements.txt
py ground_station.py --transport serial --port COM3
```

**macOS / Linux USB debugging:**

```bash
python3 -m pip install -r requirements.txt
python3 ground_station.py --transport serial
```

The software uses `pyserial>=3.5,<4` for serial mode. The port name depends on the device and operating system. Close Arduino Serial Monitor before using the same USB port in the ground station.

## Example Output

```text
[GROUND] CSV log: .../logs/flight_20261026_090000_123456.csv
[GROUND] Wi-Fi UDP listening on local port 51234
[GROUND] ESP32-S3: 192.168.4.1:4210
[GROUND] Waiting for telemetry (Ctrl+C to stop)...
[09:00:01] INFO,GROUND_STATION_CONNECTED
[09:00:01] INFO,TELEMETRY_INTERVAL_MS,200
[09:00:02] DATA,1120,1,0.001,-0.002,0.999,0.01,0.01,-0.02,1,24.50,55.00,1013.20,0,nan,nan,nan,0
[GROUND] LINK STATS (estimate): received=50, estimated_missing=0, estimated_loss=0.0%
```

*Example only; no actual flight measurements are implied.*

## Troubleshooting

| Symptom | Check |
|---|---|
| ESP32 network not visible | Confirm standalone power, successful firmware boot, board antenna, and Wi-Fi AP initialization |
| Connected to Wi-Fi but no `DATA` | Join **OpenCanSat**, check local firewall, `192.168.4.1`, and UDP port `4210`; ensure Python is allowed to receive UDP replies |
| Laptop switches to another network | Disable automatic switching/reconnecting during the experiment; the AP has no internet |
| Frequent LINK GAP messages | Increase separation gradually, rotate the enclosure during testing, inspect antenna obstruction/interference and power stability |
| CSV only contains a header | Check whether valid `DATA` records actually arrive |
| Serial port busy | Close Arduino Serial Monitor or other terminal |
| GPS shows `nan` | Test NEO-6M with good sky visibility and allow time for a position fix |

## Field-Test Advice

Perform bench tests, a controlled separation test, and tests with the actual enclosure and flight power supply **before** a permitted, safely supervised drop. Distances and reception reliability must be measured on the specific build; this code makes no guaranteed range claim. A Wi-Fi link is **not** a substitute for a flight-ready long-range radio or an onboard recorder when reliable data recovery matters.

See the [main project README](../README.md), [firmware guide](../firmware/README.md), and [hardware guide](../hardware/README.md).

Software license: [MIT](../LICENSE).
