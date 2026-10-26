#!/usr/bin/env python3
"""OpenCanSat Wi-Fi UDP receiver + optional USB serial receiver with CSV logging.

Compatible with Windows, macOS, and Linux. Wireless UDP is the default.
Requires pyserial only in USB serial mode or when listing serial ports.
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
    """Return exactly 17 telemetry values; ignore status or malformed records."""
    try:
        fields = next(csv.reader([line]))
    except (csv.Error, StopIteration):
        return None
    if len(fields) != len(TELEMETRY_FIELDS) + 1 or fields[0] != "DATA":
        return None
    return fields[1:]


class TelemetryLogger:
    def __init__(self, writer, output, expected_interval_ms):
        self.writer = writer
        self.output = output
        self.expected_interval_ms = expected_interval_ms
        self.received_packets = 0
        self.estimated_missing = 0
        self.last_device_ms = None

    def process(self, line):
        if not line:
            return False
        received = datetime.now(timezone.utc).isoformat(timespec="milliseconds")
        print(f"[{datetime.now().strftime('%H:%M:%S')}] {line}", flush=True)
        if not line.startswith("DATA,"):
            return False
        values = parse_data_line(line)
        if values is None:
            print("[GROUND] Malformed DATA packet ignored.", file=sys.stderr)
            return False
        try:
            device_ms = int(values[0])
        except ValueError:
            print("[GROUND] Invalid device_ms ignored.", file=sys.stderr)
            return False

        if self.last_device_ms is not None:
            dt = device_ms - self.last_device_ms
            if dt < 0:
                print("[GROUND] Device time reset or packets reordered; resetting loss estimate.")
            elif dt > self.expected_interval_ms * 1.5:
                # Estimate only. Firmware can skip intervals or be delayed;
                # UDP datagrams can also arrive out of order.
                missing = max(0, round(dt / self.expected_interval_ms) - 1)
                if missing:
                    self.estimated_missing += missing
                    print(f"[GROUND] LINK GAP: ~{missing} expected packets missing ({dt} ms uptime gap).")
        self.last_device_ms = device_ms
        self.writer.writerow((received, *values))
        self.output.flush()
        self.received_packets += 1
        return True

    def summary(self):
        expected = self.received_packets + self.estimated_missing
        percent = (100.0 * self.estimated_missing / expected) if expected else 0.0
        return f"received={self.received_packets}, estimated_missing={self.estimated_missing}, estimated_loss={percent:.1f}%"


def receive_wifi(args, logger):
    destination = (args.esp_ip, args.udp_port)
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.bind(("0.0.0.0", 0))  # Listen on the same UDP socket used for HELLO.
        sock.settimeout(0.5)
        print(f"[GROUND] Wi-Fi UDP listening on local port {sock.getsockname()[1]}")
        print(f"[GROUND] ESP32-S3: {args.esp_ip}:{args.udp_port}")
        print("[GROUND] Waiting for telemetry (Ctrl+C to stop)...")
        next_hello = 0.0
        last_packet = time.monotonic()
        next_warning = last_packet + args.warn_after
        next_summary = last_packet + 10.0
        while True:
            now = time.monotonic()
            if now >= next_hello:
                sock.sendto(b"HELLO", destination)
                next_hello = now + 2.0
            try:
                packet, sender = sock.recvfrom(2048)
            except socket.timeout:
                packet = b""
                sender = None
            if packet and sender and sender[0] == args.esp_ip:
                line = packet.decode("utf-8", errors="replace").strip()
                if line:
                    last_packet = time.monotonic()
                    next_warning = last_packet + args.warn_after
                    logger.process(line)
            now = time.monotonic()
            if now >= next_warning:
                print(f"[GROUND] LINK WARNING: no packets for {now - last_packet:.1f}s.")
                next_warning = now + args.warn_after
            if now >= next_summary:
                print(f"[GROUND] LINK STATS (estimate): {logger.summary()}")
                next_summary = now + 10.0


def print_ports(ports):
    print("[GROUND] Available serial ports:")
    for n, port in enumerate(ports, 1):
        print(f"  {n}. {port.device} — {port.description or 'Unknown device'}")


def receive_serial(args, logger):
    try:
        import serial
        from serial.tools import list_ports
    except ImportError as exc:
        raise RuntimeError("Serial mode requires pyserial: python -m pip install -r requirements.txt") from exc

    ports = list(list_ports.comports())
    port = args.port
    if port is None:
        if not ports:
            raise RuntimeError("No serial ports detected. Check USB cable/driver.")
        if len(ports) == 1:
            port = ports[0].device
        else:
            print_ports(ports)
            if not sys.stdin.isatty():
                raise RuntimeError("Multiple ports found; provide --port PORT.")
            while True:
                answer = input("[GROUND] Select port number (or q): ").strip().lower()
                if answer in ("q", "quit"):
                    return
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
                logger.process(raw.decode("utf-8", errors="replace").strip())


def main(argv=None):
    parser = argparse.ArgumentParser(description="OpenCanSat wireless/USB ground station + automatic CSV logger")
    parser.add_argument("--transport", choices=("wifi", "serial"), default="wifi")
    parser.add_argument("--esp-ip", default="192.168.4.1")
    parser.add_argument("--udp-port", type=int, default=4210)
    parser.add_argument("--port", help="USB serial port, e.g. COM3 (serial mode only)")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--list-ports", action="store_true")
    parser.add_argument("--expected-interval-ms", type=int, default=200,
                        help="Expected firmware DATA interval in milliseconds (default 200)")
    parser.add_argument("--warn-after", type=float, default=3.0,
                        help="Warn after this many seconds without UDP packets (default 3)")
    parser.add_argument("--output-dir", type=Path,
                        default=Path(__file__).resolve().parent / "logs")
    args = parser.parse_args(argv)

    if not (1 <= args.udp_port <= 65535):
        parser.error("--udp-port must be 1..65535")
    if args.baud <= 0 or args.expected_interval_ms <= 0 or args.warn_after <= 0:
        parser.error("baud, expected interval and warning delay must be positive")

    if args.list_ports:
        try:
            from serial.tools import list_ports
        except ImportError:
            parser.error("--list-ports requires pyserial")
        print_ports(list(list_ports.comports()))
        return 0

    csv_path = None
    logger = None
    try:
        args.output_dir.mkdir(parents=True, exist_ok=True)
        csv_path = args.output_dir / f"flight_{datetime.now().strftime('%Y%m%d_%H%M%S_%f')}.csv"
        with csv_path.open("w", newline="", encoding="utf-8") as out:
            writer = csv.writer(out)
            writer.writerow(CSV_HEADER)
            out.flush()
            logger = TelemetryLogger(writer, out, args.expected_interval_ms)
            print(f"[GROUND] CSV log: {csv_path}")
            if args.transport == "wifi":
                receive_wifi(args, logger)
            else:
                receive_serial(args, logger)
    except KeyboardInterrupt:
        print("\n[GROUND] Stopped.")
        return 0
    except (OSError, RuntimeError) as exc:
        print(f"[GROUND] Error: {exc}", file=sys.stderr)
        return 1
    finally:
        if logger is not None:
            print(f"[GROUND] Final link stats (estimate): {logger.summary()}")
        if csv_path is not None:
            print(f"[GROUND] CSV saved: {csv_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
