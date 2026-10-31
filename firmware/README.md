# OpenCanSat Firmware

ESP32-S3 Arduino firmware for sensor acquisition, **self-hosted Wi-Fi web telemetry**, browser-based CSV export, and USB serial debugging.

This version does **not** require a separate PC/Python ground-station program. The website is embedded in `OpenCanSat.ino` and served directly by the CanSat.

## Files

| Path | Description |
|---|---|
| [`OpenCanSat.ino/OpenCanSat.ino`](OpenCanSat.ino/OpenCanSat.ino) | Integrated Wi-Fi web ground-station firmware |
| [`tests/MPU6050/MPU6050.ino`](tests/MPU6050/MPU6050.ino) | Standalone MPU6050 test |
| [`tests/BME280/BME280.ino`](tests/BME280/BME280.ino) | Standalone BME280 test |
| [`tests/NEO-6M/NEO-6M.ino`](tests/NEO-6M/NEO-6M.ino) | Standalone GPS test |

> The repository currently uses the directory name `OpenCanSat.ino/`. Arduino projects are conventionally placed in a directory called `OpenCanSat/` containing `OpenCanSat.ino`. You may rename the directory for clarity; if you do, update the Markdown links in the README files.

## Requirements

- ESP32-S3 development board and a USB **data** cable
- Arduino IDE 2.x with the **ESP32 by Espressif Systems** board package
- **Adafruit BME280 Library**, plus its required **Adafruit Unified Sensor** and **Adafruit BusIO** dependencies
- **TinyGPSPlus** library
- ESP32 board-package libraries: `WiFi`, `WebServer`, `ESPmDNS`, `Wire`, and `HardwareSerial` (no separate installation normally needed)

## Pin Configuration

| Module / Signal | ESP32-S3 GPIO | Notes |
|---|---|---|
| MPU6050 SDA + BME280 SDA | GPIO8 | Shared I2C bus |
| MPU6050 SCL + BME280 SCL | GPIO9 | Shared I2C bus |
| NEO-6M TX → ESP32 RX | GPIO44 | GPS UART receive |
| NEO-6M RX ← ESP32 TX | GPIO43 | GPS UART transmit |

- MPU6050 default I2C address: `0x68`.
- BME280 I2C addresses checked: `0x76` and `0x77`.
- GPS UART baud rate: **9600**.
- USB serial baud rate: **115200**.

**GPIO43 and GPIO44 can overlap with ESP32-S3 UART0 functions.** Use the appropriate **native USB** port and enable **USB CDC On Boot** where supported by the board configuration. Disconnect conflicting peripherals temporarily if uploading fails. Check the [hardware pinout](../hardware/pinout.md) before wiring.

## Wi-Fi & Website Configuration

The ESP32-S3 uses **Wi-Fi SoftAP mode**: it creates a local wireless network without a router or internet service.

| Setting | Default |
|---|---|
| Network name | `ICanDo<NUMBER>` |
| Wi-Fi password | `cbsh@CS1` |
| Web address (mDNS) | `http://opencansat.local/` |
| Fallback IP address | `http://192.168.4.1/` |
| Wi-Fi channel | `6` |
| Sampling interval | `200 ms` (nominal 5 Hz) |
| RAM history | `256` samples (~51 seconds at 5 Hz) |

### Change the Wi-Fi Name

Open `OpenCanSat.ino` and find:

```cpp
constexpr uint32_t AP_NUMBER_OVERRIDE = 0;
```

Use `1` for **`ICanDo1`**, `2` for **`ICanDo2`**, etc. A value of `0` automatically generates a number from the ESP32-S3 chip identifier (for example, `ICanDo893764`).

```cpp
constexpr uint32_t AP_NUMBER_OVERRIDE = 1;
constexpr char AP_PASSWORD[] = "cbsh@CS1";
```

After modifying these values, **upload the sketch again**. Wi-Fi access-point passwords must contain at least 8 characters. Avoid publishing unique or sensitive Wi-Fi passwords in public repositories.

The firmware advertises `opencansat.local` using mDNS. Some mobile devices, especially certain Android configurations, may not resolve `.local` on this connection. **`192.168.4.1` remains the fallback**. When multiple CanSats are powered on, identical `.local` names may conflict; use the IP address or connect to only one at a time.

## Upload Instructions (Arduino IDE)

1. Connect the ESP32-S3 through its USB **data** port.
2. Open the main `.ino` sketch in Arduino IDE.
3. Select **ESP32S3 Dev Module** or your board's exact model under **Tools → Board**.
4. Select the detected USB/COM port under **Tools → Port**.
5. Where supported, set **USB CDC On Boot → Enabled** and use the native USB connection for serial messages.
6. Set `AP_NUMBER_OVERRIDE` to your team's assigned number, if needed.
7. Click **Verify**, then **Upload**.
8. Open **Serial Monitor** at **115200 baud** to confirm the reported Wi-Fi name, status, and address.

If upload fails with `Failed to connect to ESP32-S3`, close other serial applications, verify the cable and port, try a slower upload speed, and—if necessary—enter bootloader mode by holding **BOOT**, pressing and releasing **RST/EN**, then releasing **BOOT** and uploading. Temporarily disconnect components using conflicting UART pins while debugging.

## Connect a Phone or Laptop

1. Power the CanSat independently or by USB for bench testing.
2. Join its `ICanDo<NUMBER>` Wi-Fi network using `cbsh@CS1`.
3. Remain connected even if the phone reports **no internet**.
4. Open **http://opencansat.local/** or **http://192.168.4.1/** in a browser.
5. Keep the page open to collect readings, and tap **Download CSV** before closing it.

## Firmware Functions & Data Interfaces

### Sensor acquisition

The firmware reads the **MPU6050**, **BME280**, and **NEO-6M** approximately every 200 ms and continuously parses GPS serial input. A GPS fix may take time, especially indoors. `null` values in the web API indicate unavailable numeric readings.

### Embedded web interface

| Route | Purpose |
|---|---|
| `GET /` | Mobile-friendly ground-station webpage |
| `GET /api/telemetry?after=0` | JSON telemetry samples after the supplied sequence number |

The telemetry API returns `latest_seq`, `oldest_seq`, and a `samples` array. Requests are served in batches (up to 24 samples per response). The built-in webpage polls the API and displays current readings.

### USB serial output

The same sensor readings are also printed as comma-separated serial records at **115200 baud**:

```text
DATA,device_ms,mpu_ok,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,bme_ok,temp_c,humidity_pct,pressure_hpa,gps_fix,latitude,longitude,gps_altitude_m,satellites
```

The example above shows the **field order**, not a header transmitted by the firmware. Each record begins with `DATA` followed by 17 values. Startup and status messages begin with `INFO,` or `ERROR,`.

### CSV recording: important distinction

The webpage collects samples **in the browser's memory** and creates a CSV download when the user taps **Download CSV**. Each saved row includes a browser-generated UTC timestamp and the sensor fields.

The ESP32-S3 **does not save data permanently** in this version. It retains only a 256-sample (~51-second) rolling buffer in RAM, so data may be lost if the phone is disconnected for too long, the page is closed without exporting, or the board loses power. This is **not** a flight-safe data logger.

## Development & Safety Notes

- The site and CSV download are designed for local experiments; no internet or cloud account is needed.
- The Wi-Fi link's usable range during descent has **not been validated**. Do not assume reliable communication at the planned drop altitude.
- Battery-powered flight requires a properly regulated supply, suitable protection, and bench testing under Wi-Fi load.
- Physical compilation, integrated sensor operation, and flight performance need validation on the target board.

See [Hardware Documentation](../hardware/README.md). Firmware source code is provided under the repository's [MIT License](../LICENSE).
