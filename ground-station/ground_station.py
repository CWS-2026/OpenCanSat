#!/usr/bin/env python3
"""OpenCanSat ground station: Wi-Fi UDP by default; optional USB serial.

Wi-Fi mode requires only Python's standard library. Serial mode needs pyserial.
DATA schema (17 values) and automatic CSV logging match the original station.
"""

import argparse
import csv
import socket
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

TELEMETRY_FIELDS = (
    "device_ms", "mpu_ok", "ax_g", "ay_g", "az_g",
    "gx_dps", "gy_dps", "gz_dps", "bme_ok", "temp_c",
    "humidity_pct", "pressure_hpa", "gps_fix", "latitude",
    "longitude", "gps_altitude_m", "satellites",
)
CSV_HEADER = ("received_at_utc",) + TELEMETRY_FIELDS


def parse_data_line(line):
    """Return 17 data fields, or None if this is not a complete DATA record."""
    try:
        fields = next(csv.reader([line]))
    except (csv.Error, StopIteration):
        return None
    if len(fields) != 18 or fields[0] != "DATA":
        return None
    return fields[1:]


def process_line(line, writer, output):
    """Print telemetry and store valid DATA packets; return 1 for a saved row."""
    if not line:
        return 0
    received_at = datetime.now(timezone.utc).isoformat(timespec="milliseconds")
    print(f"[{datetime.now().strftime('%H:%M:%S')}] {line}", flush=True)
    if not line.startswith("DATA,"):
        return 0
    values = parse_data_line(line)
    if values is None:
        print("[GROUND] Warning: malformed DATA packet (not saved).", file=sys.stderr)
        return 0
    writer.writerow((received_at, *values))
    output.flush()
    return 1


def receive_wifi(args, writer, output):
    """UDP handshake (HELLO), then listen for INFO/DATA datagrams."""
    destination = (args.esp_ip, args.udp_port)
    count = 0
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.bind(("0.0.0.0", 0))
        sock.settimeout(0.7)
        print(f"[GROUND] Wi-Fi UDP local port: {sock.getsockname()[1]} (automatically assigned)")
        print(f"[GROUND] ESP32-S3 target: {args.esp_ip}:{args.udp_port}")
        print("[GROUND] Waiting for telemetry (Ctrl+C to stop)...")
        next_hello = 0.0
        last_data = time.monotonic()
        next_warning = last_data + 10.0
        while True:
            now = time.monotonic()
            if now >= next_hello:
                sock.sendto(b"HELLO", destination)
                next_hello = now + 2.0  # Refresh peer address and keepalive
            try:
                packet, sender = sock.recvfrom(2048)
            except socket.timeout:
                if time.monotonic() >= next_warning:
                    print("[GROUND] No telemetry received for 10+ s: check OpenCanSat Wi-Fi / firewall.")
                    next_warning = time.monotonic() + 10.0
                continue
            if sender[0] != args.esp_ip:
                continue  # Avoid mixing unrelated datagrams in the CSV
            line = packet.decode("utf-8", errors="replace").strip()
            if not line:
                continue
            last_data = time.monotonic()
            next_warning = last_data + 10.0
            # Each datagram contains exactly one INFO or DATA record.
            count += process_line(line, writer, output)
    return count


def print_ports(ports):
    print("[GROUND] Available serial ports:")
    for number, port in enumerate(ports, start=1):
        print(f"  {number}. {port.device} — {port.description or 'Unknown device'}")


def receive_serial(args, writer, output):
    try:
        import serial
        from serial.tools import list_ports
    except ImportError:
        raise RuntimeError("Serial mode needs pyserial: python -m pip install -r requirements.txt")
    ports = list(list_ports.comports())
    if args.list_ports:
        print_ports(ports)
        return 0
    port = args.port
    if not port:
        if not ports:
            raise RuntimeError("No serial ports detected. Check cable and drivers.")
        if len(ports) == 1:
            port = ports[0].device
            print(f"[GROUND] Auto-selected: {port}")
        else:
            print_ports(ports)
            if not sys.stdin.isatty():
                raise RuntimeError("Multiple ports found: supply --port PORT.")
            while True:
                answer = input(f"[GROUND] Choose port [1-{len(ports)}], or q: ").strip()
                if answer.lower() in ("q", "quit"):
                    return 0
                if answer.isdigit() and 1 <= int(answer) <= len(ports):
                    port = ports[int(answer) - 1].device
                    break
                print("[GROUND] Invalid selection.")
    with serial.Serial(port, args.baud, timeout=1) as device:
        print(f"[GROUND] USB serial connected: {port} @ {args.baud} baud")
        print("[GROUND] Listening (Ctrl+C to stop)...")
        while True:
            raw = device.readline()
            if raw:
                process_line(raw.decode("utf-8", errors="replace").strip(), writer, output)


def main(argv=None):
    parser = argparse.ArgumentParser(description="OpenCanSat Wi-Fi UDP / USB serial ground station with CSV logging")
    parser.add_argument("--transport", choices=("wifi", "serial"), default="wifi",
                        help="Wireless UDP (default) or legacy USB serial")
    parser.add_argument("--esp-ip", default="192.168.4.1", help="ESP32 AP IP (default: 192.168.4.1)")
    parser.add_argument("--udp-port", type=int, default=4210, help="UDP port (default: 4210)")
    parser.add_argument("--port", help="USB serial device for --transport serial, e.g. COM3")
    parser.add_argument("--baud", type=int, default=115200, help="USB baud rate (default: 115200)")
    parser.add_argument("--list-ports", action="store_true", help="List USB serial ports")
    parser.add_argument("--output-dir", type=Path, default=Path(__file__).resolve().parent / "logs",
                        help="CSV output directory")
    args = parser.parse_args(argv)
    if not 1 <= args.udp_port <= 65535:
        parser.error("--udp-port must be in the range 1..65535")
    if args.baud <= 0:
        parser.error("--baud must be positive")
    if args.list_ports:
        try:
            from serial.tools import list_ports
        except ImportError:
            parser.error("--list-ports requires pyserial (see requirements.txt)")
        print_ports(list(list_ports.comports()))
        return 0

    try:
        args.output_dir.mkdir(parents=True, exist_ok=True)
        csv_path = args.output_dir / f"flight_{datetime.now().strftime('%Y%m%d_%H%M%S_%f')}.csv"
        with csv_path.open("w", newline="", encoding="utf-8") as output:
            writer = csv.writer(output)
            writer.writerow(CSV_HEADER)
            output.flush()
            print(f"[GROUND] CSV log: {csv_path}")
            if args.transport == "wifi":
                receive_wifi(args, writer, output)
            else:
                receive_serial(args, writer, output)
    except KeyboardInterrupt:
        print("\n[GROUND] Stopped. CSV log saved.")
        return 0
    except (OSError, RuntimeError) as exc:
        print(f"[GROUND] Error: {exc}", file=sys.stderr)
        if args.transport == "wifi":
            print("[GROUND] Join the OpenCanSat Wi-Fi, allow Python UDP port 4210 through the firewall, and retry.",
                  file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
