#!/usr/bin/env python3
"""Cross-platform USB serial terminal ground station for OpenCanSat.

Requires pyserial. Supports Windows, macOS, and Linux.
"""

import argparse
import sys
from datetime import datetime

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    sys.exit("Missing pyserial. Install it with: python -m pip install -r requirements.txt")


def available_ports():
    """Return device ports in a predictable, natural order."""
    return list(list_ports.comports())


def print_ports(ports):
    print("[GROUND] Available serial ports:")
    for number, port in enumerate(ports, start=1):
        detail = port.description or "Unknown device"
        print(f"  {number}. {port.device}  —  {detail}")


def select_port(ports):
    """Auto-connect with one candidate; otherwise let the user choose."""
    if not ports:
        sys.exit(
            "[GROUND] No serial ports detected. Check USB data cable, "
            "board connection, and drivers."
        )

    if len(ports) == 1:
        port = ports[0].device
        print(f"[GROUND] Auto-selected the only detected port: {port}")
        return port

    print_ports(ports)
    if not sys.stdin.isatty():
        sys.exit("[GROUND] Multiple ports detected. Specify one with --port PORT.")

    while True:
        answer = input(f"[GROUND] Choose port [1-{len(ports)}], or q to quit: ").strip()
        if answer.lower() in ("q", "quit"):
            return None
        if answer.isdigit() and 1 <= int(answer) <= len(ports):
            return ports[int(answer) - 1].device
        print("[GROUND] Invalid selection. Try again.")


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="OpenCanSat serial log monitor (Windows / macOS / Linux)"
    )
    parser.add_argument(
        "--port",
        help="Explicit port, e.g. COM3 (Windows), /dev/cu.usbmodem101 (macOS), "
             "or /dev/ttyACM0 (Linux)",
    )
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--list-ports", action="store_true", help="List detected serial ports and exit")
    args = parser.parse_args(argv)

    if args.baud <= 0:
        parser.error("--baud must be a positive integer")

    if args.list_ports:
        ports = available_ports()
        if ports:
            print_ports(ports)
        else:
            print("[GROUND] No serial ports detected.")
        return 0

    port = args.port or select_port(available_ports())
    if port is None:
        print("[GROUND] Cancelled.")
        return 0

    try:
        with serial.Serial(port, args.baud, timeout=1) as device:
            print(f"[GROUND] Connected: {port} @ {args.baud} baud")
            print("[GROUND] Listening (Ctrl+C to stop)...")
            while True:
                raw = device.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="replace").strip()
                if line:
                    timestamp = datetime.now().strftime("%H:%M:%S")
                    print(f"[{timestamp}] {line}", flush=True)
    except KeyboardInterrupt:
        print("\n[GROUND] Stopped.")
        return 0
    except (serial.SerialException, OSError) as exc:
        print(f"[GROUND] Failed to open/read {port}: {exc}", file=sys.stderr)
        print(
            "[GROUND] Check the port, USB cable, driver, and whether Arduino Serial Monitor "
            "or another program is using this port.",
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
