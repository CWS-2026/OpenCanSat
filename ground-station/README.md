# OpenCanSat Ground Station

A simple cross-platform **terminal-only** USB serial log monitor for OpenCanSat. No GUI, charts, or data logging. Works on **Windows, macOS, and Linux**.

## Requirements

- Python 3.9 or later
- A USB data cable connected to the ESP32-S3
- Firmware sending newline-delimited serial text at 115200 baud

Install the Python dependency from this folder:

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

If **exactly one** serial port is detected, it is selected automatically. If several are detected, pick a numbered port in the terminal. This avoids assuming a Mac-specific device name.

## Command-line options

```bash
# Show detected ports
python ground_station.py --list-ports

# Pick a port manually on Windows
python ground_station.py --port COM3

# Pick a port manually on macOS
python3 ground_station.py --port /dev/cu.usbmodem101

# Pick a port manually on Linux
python3 ground_station.py --port /dev/ttyACM0

# Custom baud rate
python ground_station.py --baud 115200
```

On Windows, use `py` instead of `python` if that's how Python is installed. Press **Ctrl+C** to stop.

## Sample output

```text
[GROUND] Connected: COM3 @ 115200 baud
[GROUND] Listening (Ctrl+C to stop)...
[14:32:01] DATA,1000,25.40,1012.50,...
[14:32:02] DATA,2000,25.42,1012.48,...
```

This station prints each **line received from the firmware as-is**, prefixed by local PC time. It does not require a particular CSV schema.

## Troubleshooting

- **No ports:** Check that the USB cable supports data, the ESP32-S3 is connected, and the serial USB driver is installed (if required by the board).
- **Access denied / port busy:** Close Arduino Serial Monitor or any other app using the device.
- **Linux permission denied:** Depending on your distribution, add your user to the serial-port group (often `dialout`) and sign in again.
- **Unreadable output:** Ensure firmware and ground station have matching baud rates.
- **Multiple serial ports:** Run `--list-ports`, then use `--port` to avoid connecting to the wrong device.

This program uses USB serial, **not wireless telemetry**. A separate radio link would require additional hardware/software.
