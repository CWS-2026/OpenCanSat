# OpenCanSat Ground Station

A lightweight Python **terminal-based USB ground station** for OpenCanSat. It works on **Windows, macOS, and Linux**, prints telemetry as it arrives, and **automatically saves valid `DATA` messages to CSV files** for later analysis.

There is no graphical interface or charting. The current connection uses a **USB cable**, not a radio link.

## Requirements

- Python 3.9 or later (recommended).
- An ESP32-S3 running the [OpenCanSat firmware](../firmware/README.md).
- A USB data cable.
- [`pyserial`](requirements.txt), installed using the provided requirements file.

## Installation and Usage

Open a terminal inside the `ground-station/` directory.

**Windows (PowerShell):**

```powershell
py -m pip install -r requirements.txt
py ground_station.py
```

**macOS / Linux:**

```bash
python3 -m pip install -r requirements.txt
python3 ground_station.py
```

The program lists available serial ports. If only one port is detected, it selects that port automatically; if several exist, choose one by number. Press **Ctrl+C** to stop the receiver.

If Arduino Serial Monitor or another program has the same port open, close it before running the ground station.

## Command-Line Options

```text
--list-ports           List available serial ports and exit
--port PORT            Connect to a specific serial port
--baud RATE            Set serial baud rate (default: 115200)
--output-dir PATH      Choose CSV folder (default: ground-station/logs/)
```

Examples:

```bash
# Windows: connect to COM3
py ground_station.py --port COM3

# macOS: select a USB CDC port
python3 ground_station.py --port /dev/cu.usbmodem101

# Linux: select a typical USB CDC port
python3 ground_station.py --port /dev/ttyACM0

# Save logs to a different folder
python3 ground_station.py --output-dir ./my-logs
```

The port names are examples only; use the port detected on your computer.

## Live Terminal Output

Every nonempty serial line is displayed with the PC's local time. For example:

```text
[GROUND] Connected: COM3 @ 115200 baud
[GROUND] CSV log: .../ground-station/logs/flight_20261026_153000_123456.csv
[GROUND] Listening (Ctrl+C to stop)...
[15:30:01] INFO,OpenCanSat boot
[15:30:02] DATA,1000,1,0.000,0.000,1.000,0.000,0.000,0.000,1,24.50,55.00,1013.20,0,nan,nan,nan,0
```

These values are illustrative, not actual flight measurements.

## Automatic CSV Logging

When a serial connection succeeds, the ground station creates `logs/` automatically if it does not exist, then opens a new file named like:

```text
ground-station/logs/flight_20261026_153000_123456.csv
```

- The filename is based on the **ground-station computer's local start time**; microseconds avoid filename collisions.
- Only properly formatted `DATA,` lines are written; `INFO,` status messages remain visible in the terminal but are not stored as CSV measurement rows.
- Each row is written and flushed as it arrives, helping preserve data if the program is interrupted.
- `received_at_utc` is the **PC reception time in UTC**, while `device_ms` is the ESP32's uptime in milliseconds. These are different clocks.
- Files are saved on the **PC**, not on the ESP32-S3. Nothing is uploaded to a server.
- The `logs/` directory contains experiment data; review it before committing files to GitHub.

### CSV Column Schema

```csv
received_at_utc,device_ms,mpu_ok,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,bme_ok,temp_c,humidity_pct,pressure_hpa,gps_fix,latitude,longitude,gps_altitude_m,satellites
```

| Column group | Meaning |
|---|---|
| `received_at_utc` | UTC timestamp created by the PC at receipt |
| `device_ms` | ESP32 uptime (ms) |
| `mpu_ok`, `bme_ok`, `gps_fix` | Validity flags (`1` or `0`) |
| `ax_g` to `az_g` | Acceleration (g) |
| `gx_dps` to `gz_dps` | Angular velocity (deg/s) |
| `temp_c`, `humidity_pct`, `pressure_hpa` | BME280 readings |
| `latitude`, `longitude`, `gps_altitude_m`, `satellites` | GPS data |

Unavailable numeric readings may be represented by `nan`. If no valid `DATA` line arrives, the CSV file may contain only the header.

## Troubleshooting

| Symptom | Check |
|---|---|
| No serial ports | USB **data** cable, device connection, board driver |
| Access denied / busy port | Close Arduino Serial Monitor and other serial applications |
| Wrong port selected | Run `--list-ports`, then specify `--port` |
| No `DATA` packets | Check that the integrated firmware is running and USB CDC serial is configured correctly |
| Garbled text | Use the same baud rate as the firmware (default 115200) |
| Linux permission error | Check serial-port group permissions (often `dialout`) |
| CSV has only a header | Verify that firmware is actually sending complete `DATA` records |

The receiver has been implemented, but end-to-end operation with the actual ESP32-S3 and sensors still needs to be tested.

## Related Documentation

- [Main project README](../README.md)
- [Firmware setup and telemetry format](../firmware/README.md)
- [Hardware and wiring](../hardware/README.md)

This software is distributed under the project's [MIT License](../LICENSE).
