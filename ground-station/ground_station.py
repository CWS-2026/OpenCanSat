#!/usr/bin/env python3
"""OpenCanSat cross-platform USB terminal receiver with automatic CSV logging.

Install: python -m pip install -r requirements.txt
Run:     python ground_station.py [--port COM3] [--output-dir logs]
"""

import argparse
import csv
import sys
from datetime import datetime, timezone
from pathlib import Path

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    sys.exit("Missing pyserial. Install it with: python -m pip install -r requirements.txt")

# Matches the 18 comma-separated fields sent by firmware/OpenCanSat/OpenCanSat.ino.
TELEMETRY_FIELDS = (
    "device_ms", "mpu_ok", "ax_g", "ay_g", "az_g",
    "gx_dps", "gy_dps", "gz_dps", "bme_ok", "temp_c",
    "humidity_pct", "pressure_hpa", "gps_fix", "latitude",
    "longitude", "gps_altitude_m", "satellites",
)
CSV_HEADER = ("received_at_utc",) + TELEMETRY_FIELDS


def available_ports():
    """List USB/serial ports on Windows, macOS and Linux."""
    return list(list_ports.comports())


def print_ports(ports):
    print("[GROUND] Available serial ports:")
    for number, port in enumerate(ports, start=1):
        print(f"  {number}. {port.device}  —  {port.description or 'Unknown device'}")


def select_port(ports):
    if not ports:
        print("[GROUND] No serial ports detected. Check the USB cable and driver.", file=sys.stderr)
        return None
    if len(ports) == 1:
        print(f"[GROUND] Auto-selected: {ports[0].device}")
        return ports[0].device

    print_ports(ports)
    if not sys.stdin.isatty():
        print("[GROUND] Multiple ports found. Use --port PORT.", file=sys.stderr)
        return None

    while True:
        answer = input(f"[GROUND] Choose port [1-{len(ports)}], or q to quit: ").strip()
        if answer.lower() in ("q", "quit"):
            return None
        if answer.isdigit() and 1 <= int(answer) <= len(ports):
            return ports[int(answer) - 1].device
        print("[GROUND] Invalid selection.")


def log_filename():
    # Microseconds prevent overwrites when started twice in the same second.
    return f"flight_{datetime.now().strftime('%Y%m%d_%H%M%S_%f')}.csv"


def parse_data_line(line):
    """Return the firmware's 17 telemetry columns, or None for malformed DATA."""
    fields = next(csv.reader([line]))
    if len(fields) != len(TELEMETRY_FIELDS) + 1 or fields[0] != "DATA":
        return None
    return fields[1:]


def receive(device, writer, output):
    """Print all received lines; write valid DATA packets as CSV, flushing each row."""
    count = 0
    while True:
        raw = device.readline()
        if not raw:
            continue
        line = raw.decode("utf-8", errors="replace").strip()
        if not line:
            continue

        received = datetime.now(timezone.utc).isoformat(timespec="milliseconds")
        print(f"[{datetime.now().strftime('%H:%M:%S')}] {line}", flush=True)

        if line.startswith("DATA,"):
            values = parse_data_line(line)
            if values is None:
                print("[GROUND] Warning: malformed DATA packet (not saved).", file=sys.stderr)
                continue
            writer.writerow((received, *values))
            output.flush()  # Preserve rows even if the receiver is interrupted.
            count += 1
    return count


def main(argv=None):
    parser = argparse.ArgumentParser(description="OpenCanSat terminal ground station + CSV logger")
    parser.add_argument("--port", help="Serial port: COM3, /dev/cu.usbmodem101, /dev/ttyACM0, etc.")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud rate (default: 115200)")
    parser.add_argument("--list-ports", action="store_true", help="Show available ports and exit")
    parser.add_argument("--output-dir", type=Path, default=Path(__file__).resolve().parent / "logs",
                        help="CSV destination folder (default: ground-station/logs)")
    args = parser.parse_args(argv)

    if args.baud <= 0:
        parser.error("--baud must be positive")

    if args.list_ports:
        ports = available_ports()
        print_ports(ports) if ports else print("[GROUND] No serial ports detected.")
        return 0

    port = args.port or select_port(available_ports())
    if port is None:
        return 1

    csv_path = None
    try:
        with serial.Serial(port, args.baud, timeout=1) as device:
            args.output_dir.mkdir(parents=True, exist_ok=True)
            csv_path = args.output_dir / log_filename()
            with csv_path.open("w", encoding="utf-8", newline="") as output:
                writer = csv.writer(output)
                writer.writerow(CSV_HEADER)
                output.flush()
                print(f"[GROUND] Connected: {port} @ {args.baud} baud")
                print(f"[GROUND] CSV log: {csv_path}")
                print("[GROUND] Listening (Ctrl+C to stop)...")
                receive(device, writer, output)
    except KeyboardInterrupt:
        print("\n[GROUND] Stopped. CSV log saved.")
        return 0
    except (serial.SerialException, OSError) as exc:
        print(f"[GROUND] Serial/CSV error: {exc}", file=sys.stderr)
        if csv_path is not None:
            print(f"[GROUND] Received rows (if any) were saved to: {csv_path}", file=sys.stderr)
        print("[GROUND] Check USB cable, driver, available disk space, and other serial monitors.", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
