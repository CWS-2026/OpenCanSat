# OpenCanSat Firmware

ESP32-S3 Arduino firmware for acquiring data from an MPU6050, a BME280, and a NEO-6M GPS receiver, then transmitting telemetry to a computer over USB serial.

## Files

| Path | Description |
|---|---|
| [`OpenCanSat.ino/OpenCanSat.ino`](OpenCanSat.ino/OpenCanSat.ino) | Integrated main firmware |
| [`tests/MPU6050/MPU6050.ino`](tests/MPU6050/MPU6050.ino) | Standalone MPU6050 test |
| [`tests/BME280/BME280.ino`](tests/BME280/BME280.ino) | Standalone BME280 test |
| [`tests/NEO-6M/NEO-6M.ino`](tests/NEO-6M/NEO-6M.ino) | Standalone GPS test |

> The current repository uses `firmware/OpenCanSat.ino/OpenCanSat.ino`. For compatibility with Arduino tooling, **rename the containing folder from `OpenCanSat.ino` to `OpenCanSat`** when organizing the project. The expected conventional path is `firmware/OpenCanSat/OpenCanSat.ino`.

## Requirements

- An ESP32-S3 development board and a USB data cable.
- Arduino IDE with Espressif's **esp32** board package installed.
- Arduino libraries: **Adafruit BME280 Library** (and its required dependencies) and **TinyGPSPlus**.
- The `Wire` and `HardwareSerial` headers are provided by the Arduino ESP32 platform.

## Pin Configuration

| Connection | ESP32-S3 GPIO | Interface |
|---|---|---|
| MPU6050 SDA and BME280 SDA | 8 | Shared I2C SDA |
| MPU6050 SCL and BME280 SCL | 9 | Shared I2C SCL |
| NEO-6M TX → ESP32 RX | 44 | GPS UART receive |
| NEO-6M RX ← ESP32 TX | 43 | GPS UART transmit |

GPS runs at **9600 baud**. USB telemetry uses **115200 baud**. The BME280 firmware checks I2C addresses `0x76` and `0x77`; the MPU6050 uses `0x68`.

**ESP32-S3 USB note:** GPIO43 and GPIO44 can be associated with UART0 on ESP32-S3 boards. The integrated sketch allocates those pins to the GPS receiver. Select an appropriate **native USB** connection and enable **USB CDC On Boot** in the Arduino IDE when supported by your board configuration. Exact Arduino settings vary by board and core version. Verify that USB serial output works without conflicting with the GPS wiring.

For full connections, power requirements, and drawings, see the [hardware documentation](../hardware/README.md) and [pin mapping](../hardware/pinout.md).

## Uploading the Main Firmware

1. Install the ESP32 board package and required libraries in Arduino IDE.
2. Open [`OpenCanSat.ino/OpenCanSat.ino`](OpenCanSat.ino/OpenCanSat.ino).
3. If Arduino IDE reports a sketch-folder naming issue, rename the sketch directory to `OpenCanSat` (without `.ino`). Then select the correct ESP32-S3 board model and USB port.
4. Enable native USB CDC serial output where applicable.
5. Compile and upload the sketch.
6. Close Arduino Serial Monitor before launching the Python ground station, because the two programs cannot normally share the same serial port.

## Telemetry Format

The sketch sends one measurement line approximately every **1,000 ms**. GPS characters are parsed continuously between transmissions. The record format is:

```text
DATA,device_ms,mpu_ok,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,bme_ok,temp_c,humidity_pct,pressure_hpa,gps_fix,latitude,longitude,gps_altitude_m,satellites
```

The header above is explanatory; the firmware sends data rows without a header. The `DATA` prefix is followed by **17 values**.

| Field(s) | Description |
|---|---|
| `device_ms` | Milliseconds since ESP32 startup |
| `mpu_ok`, `bme_ok`, `gps_fix` | `1` when the corresponding reading/fix is valid; otherwise `0` |
| `ax_g`, `ay_g`, `az_g` | Acceleration in g |
| `gx_dps`, `gy_dps`, `gz_dps` | Angular velocity in degrees per second |
| `temp_c`, `humidity_pct`, `pressure_hpa` | Temperature (°C), relative humidity (%), pressure (hPa) |
| `latitude`, `longitude`, `gps_altitude_m` | GPS coordinates (decimal degrees) and altitude (m) |
| `satellites` | Number of satellites reported by the GPS parser; `0` if unavailable or stale |

Unavailable measurements are output as `nan`. A GPS receiver may output valid NMEA sentences before it has acquired a location fix; a valid fix often requires testing outdoors.

Startup messages begin with `INFO,`, for example:

```text
INFO,OpenCanSat boot
INFO,MPU6050,OK
INFO,BME280,OK
INFO,GPS,WAITING_FOR_FIX
```

The exact messages depend on which sensors respond at startup.

## Data Recording

**The firmware does not write measurements to onboard storage.** It streams them over USB serial. The companion [ground station](../ground-station/README.md) displays incoming data and saves valid `DATA` records to timestamped CSV files on the connected computer.

## Current Limitations

- USB cable required; there is no wireless transmitter or receiver in this version.
- Sensor initialization and basic read-error handling are present, but automatic sensor reconnection and flight-state detection are not implemented.
- The nominal output rate is 1 Hz; actual timing depends on sensor reads and USB communication.
- Compilation and physical operation on the target ESP32-S3 hardware still require verification.

For the hardware license and wiring details, see [`../hardware/`](../hardware/). Firmware is covered by the repository's [MIT License](../LICENSE).
