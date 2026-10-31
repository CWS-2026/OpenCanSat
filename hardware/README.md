# OpenCanSat Hardware

Hardware documentation for the **ESP32-S3 OpenCanSat** prototype: sensor wiring, component selection, mechanical design, and power/flight considerations.

## Hardware Overview

| Component | Model | Function |
|---|---|---|
| Microcontroller | ESP32-S3 | Sensor acquisition, onboard Wi-Fi access point, HTTP web server |
| IMU | MPU6050 | 3-axis acceleration and angular velocity |
| Environmental sensor | BME280 | Temperature, humidity, and atmospheric pressure |
| GPS receiver | NEO-6M | Latitude, longitude, altitude, and satellite count |

See the [Bill of Materials](BOM.csv) and [Pin Configuration](pinout.md) for the documented prototype components and pin assignments.

## Wiring Diagram

![OpenCanSat wiring diagram](wiring/wiring_diagram.png)

- [Editable Fritzing project](wiring/OpenCanSat.fzz)
- [Wiring diagram image](wiring/wiring_diagram.png)
- [Pin configuration](pinout.md)

### Signal Connections

| Module | Signal | ESP32-S3 |
|---|---|---|
| MPU6050 | SDA | GPIO8 |
| MPU6050 | SCL | GPIO9 |
| BME280 | SDA | GPIO8 |
| BME280 | SCL | GPIO9 |
| NEO-6M | TX → RX | GPIO44 |
| NEO-6M | RX ← TX | GPIO43 |

The MPU6050 and BME280 share an **I2C** bus. GPS communication uses **UART at 9600 baud**. Native USB serial debugging operates at **115200 baud**.

**Electrical caution:** GPIO43/44 can be associated with UART0; confirm the specific ESP32-S3 board's upload and USB-CDC configuration. Use the correct supply voltage for **each exact sensor breakout board** and check common ground connections before applying power. The Fritzing drawing is a wiring reference, **not a validated PCB manufacturing design**.

## Wireless Web Ground Station

The ESP32-S3 uses its **built-in 2.4 GHz Wi-Fi** to create a local access point. A phone connects to `ICanDo<NUMBER>` (password `cbsh@CS1`) and opens:

- **http://opencansat.local/** (if mDNS is supported)
- **http://192.168.4.1/** (fallback)

The dashboard runs **on the ESP32-S3**, so there is **no external Python ground-station application** in the current design. The web interface shows live measurements and allows browser-side CSV download. The firmware also prints debug logs over native USB serial.

### Radio and flight limitations

Wi-Fi operation is implemented in software, but **drop-test range and reliability are unverified**. Range depends on the board's antenna, enclosure, CanSat orientation, surrounding obstacles, interference, and separation from the phone. Place the Wi-Fi antenna away from batteries and metal parts as practical, and validate reception with the *actual* enclosure and power source before flight.

The system has no long-range radio, guaranteed retransmission, or permanent onboard storage. If the link is lost longer than the short RAM history, data cannot be recovered from the CanSat. For demanding flight experiments, consider independent onboard storage and a purpose-built long-range telemetry link.

## Power Supply (Under Development)

The present published [BOM](BOM.csv) primarily lists the controller and sensing modules; **the battery, charger/protection circuitry, and DC-DC regulator should not be treated as finalized or verified components**.

A proposed architecture is:

```text
Protected 1S LiPo (3.7 V nominal)
            │
       Power switch
            │
   Regulated 5 V DC-DC output
            │
      ESP32-S3 5V input
            │
  Sensors powered according to
  their individual module ratings
```

Use a suitable **protected 1S LiPo** with adequate discharge current, and a regulator capable of handling ESP32-S3 Wi-Fi current peaks. Previously considered Pololu 5 V boost regulators are candidates, **not yet validated in the final power system**. Verify the board's power-input design, regulator output voltage, wiring polarity, connector polarity, battery temperature, and behavior during radio transmission before untethered use.

**Do not connect an unregulated LiPo directly to a 5 V input.** A boost converter is not a LiPo charger or a substitute for battery protection. Do not charge or operate damaged or swollen cells; charging should use an appropriate dedicated LiPo charger.

## Mechanical Design

The enclosure is designed in **Autodesk Fusion 360**. Original CAD and manufacturing formats are available for community inspection and modification.

### 3D Model Preview

![OpenCanSat 3D model](mechanical/OpenCanSat.png)

### 3D Model Video

**[▶ Watch the OpenCanSat 3D Demonstration](mechanical/OpenCanSat.mp4)**

### Downloadable Design Files

| File | Description |
|---|---|
| [OpenCanSat.f3d](mechanical/OpenCanSat.f3d) | Editable Fusion 360 project |
| [OpenCanSat.stl](mechanical/OpenCanSat.stl) | 3D printing/slicing mesh |
| [OpenCanSat.3mf](mechanical/OpenCanSat.3mf) | 3D manufacturing format |
| [OpenCanSat.png](mechanical/OpenCanSat.png) | Model preview image |
| [OpenCanSat.mp4](mechanical/OpenCanSat.mp4) | Design demonstration video |

**Note:** a `.step` file is **not currently listed** in the repository. Add it later if cross-platform CAD interchange is needed.

This is a prototype enclosure. Printed-part tolerances, fit, impact resistance, parachute/attachment hardware, and durability during descent have not yet been validated.

## Verification Checklist

- [ ] Verify all sensor power rails and I2C/UART connections.
- [ ] Compile and upload the latest web firmware to the actual ESP32-S3 board.
- [ ] Confirm the `ICanDo<NUMBER>` AP and web page work with the intended phone.
- [ ] Verify GPS reception and realistic sensor readings.
- [ ] Test independent power under sustained Wi-Fi activity.
- [ ] Measure wireless reliability at the maximum planned separation and different orientations.
- [ ] Check enclosure fit and mechanical integrity before any drop.
- [ ] Confirm that browser CSV downloads contain complete, useful test data.

## License & Attribution

Original OpenCanSat hardware designs are made available under the [CERN Open Hardware Licence Version 2 – Permissive (CERN-OHL-P-2.0)](LICENSE), to the extent that the contributors own the necessary rights.

Third-party Fritzing components, images, and other assets retain their original license terms. See [Third-Party Notices](THIRD_PARTY_NOTICES.md) for sources and licensing caveats.

The software and firmware are covered by the repository's [MIT License](../LICENSE).
