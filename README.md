# OpenCanSat

**An open-source ESP32-S3-based CanSat platform for environmental monitoring and flight data acquisition.**

## Overview

OpenCanSat is an open-source CanSat project built around the ESP32-S3 microcontroller. The project aims to provide a modular and accessible platform for collecting environmental and flight-related data using various sensors.

The project is currently in its early development stage, focusing on individual sensor testing and hardware validation.

## Hardware

The current hardware configuration includes the following components:

| Component | Model | Purpose |
|---|---|---|
| Microcontroller | ESP32-S3 | Main processing unit |
| IMU | MPU6050 | 3-axis acceleration and angular velocity |
| Environmental Sensor | BME280 | Temperature, humidity, and atmospheric pressure |
| GPS Module | NEO-6M | Position, altitude, and satellite information |

### Pin Configuration

| Component | Interface | ESP32-S3 Pins |
|---|---|---|
| MPU6050 | I2C | SDA: GPIO 8, SCL: GPIO 9 |
| BME280 | I2C | SDA: GPIO 8, SCL: GPIO 9 |
| NEO-6M | UART | RX: GPIO 44, TX: GPIO 43 |

**Note:** GPIO 43 and GPIO 44 are also associated with the default UART0 interface on the ESP32-S3. Using these pins for GPS communication may conflict with serial debugging through a USB-to-UART interface. Native USB CDC debugging is recommended when using this configuration.

Pin assignments may change as the hardware design evolves.

## Firmware

The firmware is developed using the Arduino framework for the ESP32-S3.

### Project Structure

The current firmware directory contains individual sensor testing programs.

- `firmware/tests/mpu6050_test/mpu6050_test.ino` — MPU6050 accelerometer and gyroscope testing
- `firmware/tests/bme280_test/bme280_test.ino` — BME280 environmental sensor testing
- `firmware/tests/gps_test/gps_test.ino` — NEO-6M GPS module testing

These programs are intended to verify the basic operation of each sensor independently before integration into the main CanSat firmware.

### Dependencies

The following Arduino libraries are used:

| Library | Purpose |
|---|---|
| Wire | I2C communication |
| Adafruit BME280 Library | BME280 sensor interface |
| Adafruit Unified Sensor | Dependency for the BME280 library |
| TinyGPSPlus | GPS data parsing |

Additional dependencies may be installed automatically through the Arduino Library Manager.

### Getting Started

1. Install Arduino IDE and the ESP32 board support package.
2. Select the appropriate ESP32-S3 board.
3. Install the required libraries using Arduino Library Manager.
4. Connect the sensors according to the hardware configuration.
5. Open the desired `.ino` test file in Arduino IDE.
6. Compile and upload the program to the ESP32-S3.
7. Open the Serial Monitor at **115200 baud** to view the sensor output.

Each test program can be executed independently.

## Development Status

OpenCanSat is currently under active development.

**Current focus**
- Individual sensor testing
- Hardware connection verification
- Basic environmental and motion data acquisition
- GPS communication testing

**Planned Development**
- Integration of multiple sensors
- Unified sensor data processing
- Flight data logging
- Flight-state monitoring

The planned features are subject to change as the project progresses.

## Contributing

Contributions, suggestions, and improvements are welcome.

You can contribute by reporting issues, suggesting new features, improving documentation, or submitting pull requests.

## License

See the [LICENSE](LICENSE) file for the project's current licensing terms.

Hardware-specific licensing information will be provided when the hardware design files are released.
