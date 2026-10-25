// OpenCanSat - integrated sensor firmware for ESP32-S3
// Sensors: MPU6050 (I2C), BME280 (I2C), NEO-6M (UART)
// IMPORTANT: GPIO 43/44 are UART0 pins. Use native USB CDC for Serial
// if the GPS is connected to these pins.

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <math.h>

// Current OpenCanSat prototype pinout
constexpr uint8_t I2C_SDA_PIN = 8;
constexpr uint8_t I2C_SCL_PIN = 9;
constexpr uint8_t GPS_RX_PIN = 44;  // ESP RX <- GPS TX
constexpr uint8_t GPS_TX_PIN = 43;  // ESP TX -> GPS RX

constexpr uint8_t MPU_ADDR = 0x68;
constexpr unsigned long USB_BAUD = 115200;
constexpr unsigned long GPS_BAUD = 9600;
constexpr unsigned long OUTPUT_INTERVAL_MS = 1000;
constexpr unsigned long GPS_MAX_AGE_MS = 5000;

Adafruit_BME280 bme;
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

bool mpuFound = false;
bool bmeFound = false;
unsigned long lastOutput = 0;

struct ImuSample {
  float ax = NAN, ay = NAN, az = NAN; // g
  float gx = NAN, gy = NAN, gz = NAN; // degrees/second
  bool ok = false;
};

bool writeMpuRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool initializeMpu() {
  Wire.beginTransmission(MPU_ADDR);
  if (Wire.endTransmission() != 0) return false;

  // Wake up, and explicitly select default full-scale ranges:
  // accelerometer +/-2 g, gyroscope +/-250 deg/s.
  return writeMpuRegister(0x6B, 0x00) &&
         writeMpuRegister(0x1B, 0x00) &&
         writeMpuRegister(0x1C, 0x00);
}

ImuSample readMpu() {
  ImuSample s;
  if (!mpuFound) return s;

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return s;
  if (Wire.requestFrom(MPU_ADDR, (uint8_t)14) != 14) return s;

  int16_t ax = (int16_t)((Wire.read() << 8) | Wire.read());
  int16_t ay = (int16_t)((Wire.read() << 8) | Wire.read());
  int16_t az = (int16_t)((Wire.read() << 8) | Wire.read());

  // Skip the two MPU6050 temperature-register bytes.
  Wire.read();
  Wire.read();

  int16_t gx = (int16_t)((Wire.read() << 8) | Wire.read());
  int16_t gy = (int16_t)((Wire.read() << 8) | Wire.read());
  int16_t gz = (int16_t)((Wire.read() << 8) | Wire.read());

  s.ax = ax / 16384.0f;
  s.ay = ay / 16384.0f;
  s.az = az / 16384.0f;
  s.gx = gx / 131.0f;
  s.gy = gy / 131.0f;
  s.gz = gz / 131.0f;
  s.ok = true;
  return s;
}

void printTelemetry() {
  const ImuSample imu = readMpu();

  float tempC = NAN, humidity = NAN, pressureHpa = NAN;
  bool bmeOk = false;
  if (bmeFound) {
    tempC = bme.readTemperature();
    humidity = bme.readHumidity();
    pressureHpa = bme.readPressure() / 100.0f;
    bmeOk = isfinite(tempC) && isfinite(humidity) && isfinite(pressureHpa);
    if (!bmeOk) tempC = humidity = pressureHpa = NAN;
  }

  const bool gpsFix = gps.location.isValid() &&
                      gps.location.age() < GPS_MAX_AGE_MS;
  const float latitude = gpsFix ? gps.location.lat() : NAN;
  const float longitude = gpsFix ? gps.location.lng() : NAN;
  const float altitudeM = gpsFix && gps.altitude.isValid() &&
                          gps.altitude.age() < GPS_MAX_AGE_MS
                          ? gps.altitude.meters() : NAN;
  const unsigned long satellites = gps.satellites.isValid() &&
                                   gps.satellites.age() < GPS_MAX_AGE_MS
                                   ? gps.satellites.value() : 0UL;

  // Machine-readable CSV: one complete sample per line.
  // DATA,ms,mpu_ok,ax,ay,az,gx,gy,gz,bme_ok,temp,humidity,pressure,
  //      gps_fix,latitude,longitude,gps_altitude,satellites
  Serial.printf(
    "DATA,%lu,%u,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%.2f,%.2f,%.2f,%u,%.6f,%.6f,%.2f,%lu\n",
    millis(), imu.ok ? 1 : 0,
    imu.ax, imu.ay, imu.az, imu.gx, imu.gy, imu.gz,
    bmeOk ? 1 : 0, tempC, humidity, pressureHpa,
    gpsFix ? 1 : 0, latitude, longitude, altitudeM, satellites
  );
}

void setup() {
  Serial.begin(USB_BAUD);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  mpuFound = initializeMpu();
  bmeFound = bme.begin(0x76, &Wire);
  if (!bmeFound) bmeFound = bme.begin(0x77, &Wire);

  Serial.println("INFO,OpenCanSat boot");
  Serial.printf("INFO,MPU6050,%s\n", mpuFound ? "OK" : "NOT_FOUND");
  Serial.printf("INFO,BME280,%s\n", bmeFound ? "OK" : "NOT_FOUND");
  Serial.println("INFO,GPS,WAITING_FOR_FIX");
  Serial.println("INFO,Telemetry interval,1000 ms");
}

void loop() {
  // Parse UART continuously to avoid losing NMEA sentences.
  while (gpsSerial.available() > 0) {
    gps.encode((char)gpsSerial.read());
  }

  const unsigned long now = millis();
  if (now - lastOutput >= OUTPUT_INTERVAL_MS) {
    lastOutput = now;
    printTelemetry();
  }
}
