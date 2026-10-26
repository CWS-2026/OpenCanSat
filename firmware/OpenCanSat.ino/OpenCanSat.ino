/*
 * OpenCanSat | ESP32-S3 integrated Wi-Fi UDP telemetry
 *
 * MPU6050 + BME280: I2C SDA=GPIO8, SCL=GPIO9
 * NEO-6M: GPS TX -> ESP32 RX(GPIO44), GPS RX <- ESP32 TX(GPIO43)
 *
 * Connect the PC to the "OpenCanSat" Wi-Fi network and run ground_station.py.
 * The PC sends "HELLO" to 192.168.4.1:4210 every two seconds; the ESP32
 * returns INFO/DATA UDP packets to that PC and prints to native USB Serial.
 * No wireless reception = no onboard copy of the lost data.
 *
 * Arduino libraries: Adafruit BME280 Library, TinyGPSPlus (plus BME deps).
 * For the GPS on GPIO43/44, select native USB CDC for serial debugging.
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <math.h>
#include <string.h>

// --- Board wiring (matches hardware/pinout.md) ---
constexpr uint8_t I2C_SDA_PIN = 8;
constexpr uint8_t I2C_SCL_PIN = 9;
constexpr uint8_t GPS_RX_PIN = 44;  // ESP RX <- GPS TX
constexpr uint8_t GPS_TX_PIN = 43;  // ESP TX -> GPS RX
constexpr uint8_t MPU_ADDR = 0x68;

// --- Telemetry and timeouts ---
constexpr uint32_t USB_BAUD = 115200;
constexpr uint32_t GPS_BAUD = 9600;
constexpr uint32_t TELEMETRY_INTERVAL_MS = 200;   // 5 Hz: more samples in a short drop
constexpr uint32_t GPS_MAX_AGE_MS = 5000;
constexpr uint32_t PEER_TIMEOUT_MS = 10000;       // HELLO refreshed by PC every 2 s
constexpr uint32_t SENSOR_RETRY_MS = 5000;

// --- ESP32-S3 built-in 2.4 GHz Wi-Fi access point ---
constexpr uint16_t UDP_PORT = 4210;
constexpr uint8_t WIFI_CHANNEL = 6;  // Keep a fixed channel; change if locally congested
const char *WIFI_SSID = "OpenCanSat";
const char *WIFI_PASSWORD = "OpenCanSat1234"; // Demo only! Change for field use.
IPAddress apIP(192, 168, 4, 1);
IPAddress apMask(255, 255, 255, 0);

Adafruit_BME280 bme;
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
WiFiUDP udp;

bool mpuAvailable = false;
bool bmeAvailable = false;
bool wifiReady = false;
bool peerKnown = false;
IPAddress peerIP;
uint16_t peerPort = 0;
uint32_t lastHelloMs = 0;
uint32_t lastTelemetryMs = 0;
uint32_t lastSensorRetryMs = 0;

struct ImuData {
  float ax = NAN, ay = NAN, az = NAN;  // g
  float gx = NAN, gy = NAN, gz = NAN;  // degrees/s
  bool ok = false;
};

void usbLog(const char *line) {
  // Native USB serial is optional in flight; do not depend on the cable.
  if (Serial) Serial.println(line);
}

bool peerActive() {
  return wifiReady && peerKnown && WiFi.softAPgetStationNum() > 0 &&
         (uint32_t)(millis() - lastHelloMs) < PEER_TIMEOUT_MS;
}

void sendUdp(const char *line) {
  if (!peerActive()) return;
  if (udp.beginPacket(peerIP, peerPort)) {
    udp.write((const uint8_t *)line, strlen(line));
    udp.endPacket();
  }
}

void emitInfo(const char *line) {
  usbLog(line);
  sendUdp(line);
}

bool writeMpuRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool beginMpu() {
  Wire.beginTransmission(MPU_ADDR);
  if (Wire.endTransmission() != 0) return false;
  return writeMpuRegister(0x6B, 0x00) &&  // Wake up
         writeMpuRegister(0x1B, 0x00) &&  // Gyro +/-250 dps
         writeMpuRegister(0x1C, 0x00);    // Accel +/-2 g
}

ImuData readMpu() {
  ImuData imu;
  if (!mpuAvailable) return imu;

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(MPU_ADDR, (uint8_t)14) != 14) {
    mpuAvailable = false;
    return imu;
  }
  const int16_t ax = (int16_t)((Wire.read() << 8) | Wire.read());
  const int16_t ay = (int16_t)((Wire.read() << 8) | Wire.read());
  const int16_t az = (int16_t)((Wire.read() << 8) | Wire.read());
  Wire.read(); Wire.read();  // Skip temperature register
  const int16_t gx = (int16_t)((Wire.read() << 8) | Wire.read());
  const int16_t gy = (int16_t)((Wire.read() << 8) | Wire.read());
  const int16_t gz = (int16_t)((Wire.read() << 8) | Wire.read());

  imu.ax = ax / 16384.0f; imu.ay = ay / 16384.0f; imu.az = az / 16384.0f;
  imu.gx = gx / 131.0f; imu.gy = gy / 131.0f; imu.gz = gz / 131.0f;
  imu.ok = true;
  return imu;
}

void retryUnavailableSensors() {
  const uint32_t now = millis();
  if ((uint32_t)(now - lastSensorRetryMs) < SENSOR_RETRY_MS) return;
  lastSensorRetryMs = now;
  if (!mpuAvailable && beginMpu()) {
    mpuAvailable = true;
    emitInfo("INFO,MPU6050,RECOVERED");
  }
  if (!bmeAvailable && (bme.begin(0x76, &Wire) || bme.begin(0x77, &Wire))) {
    bmeAvailable = true;
    emitInfo("INFO,BME280,RECOVERED");
  }
}

void receiveHello() {
  if (!wifiReady) return;
  const int packetSize = udp.parsePacket();
  if (packetSize <= 0) return;

  const IPAddress sender = udp.remoteIP();
  const uint16_t port = udp.remotePort();
  char command[16];
  int size = udp.read(command, sizeof(command) - 1);
  if (size < 0) size = 0;
  command[size] = '\0';
  while (udp.available() > 0) udp.read();
  if (strcmp(command, "HELLO") != 0 || port == 0) return;

  const bool newPeer = !peerActive() || !peerKnown || sender != peerIP || port != peerPort;
  peerIP = sender;
  peerPort = port;
  lastHelloMs = millis();
  peerKnown = true;
  if (newPeer) {
    emitInfo("INFO,GROUND_STATION_CONNECTED");
    emitInfo(mpuAvailable ? "INFO,MPU6050,OK" : "INFO,MPU6050,NOT_FOUND");
    emitInfo(bmeAvailable ? "INFO,BME280,OK" : "INFO,BME280,NOT_FOUND");
    emitInfo("INFO,LINK,WIFI_UDP_4210");
    emitInfo("INFO,TELEMETRY_INTERVAL_MS,200");
  }
}

void transmitTelemetry() {
  const ImuData imu = readMpu();
  float temperature = NAN, humidity = NAN, pressure = NAN;
  bool bmeOK = false;
  if (bmeAvailable) {
    temperature = bme.readTemperature();
    humidity = bme.readHumidity();
    pressure = bme.readPressure() / 100.0f;
    bmeOK = isfinite(temperature) && isfinite(humidity) && isfinite(pressure);
    if (!bmeOK) {
      bmeAvailable = false;
      temperature = humidity = pressure = NAN;
    }
  }

  const bool gpsFix = gps.location.isValid() && gps.location.age() < GPS_MAX_AGE_MS;
  const double latitude = gpsFix ? gps.location.lat() : NAN;
  const double longitude = gpsFix ? gps.location.lng() : NAN;
  const double altitudeM = gpsFix && gps.altitude.isValid() && gps.altitude.age() < GPS_MAX_AGE_MS
                         ? gps.altitude.meters() : NAN;
  const unsigned long satellites = gps.satellites.isValid() && gps.satellites.age() < GPS_MAX_AGE_MS
                         ? gps.satellites.value() : 0UL;

  // Protocol unchanged: DATA + exactly 17 fields, accepted by current CSV logger.
  // DATA,device_ms,mpu_ok,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,bme_ok,
  //      temp_c,humidity_pct,pressure_hpa,gps_fix,latitude,longitude,gps_altitude_m,satellites
  char line[320];
  const int n = snprintf(line, sizeof(line),
      "DATA,%lu,%u,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%.2f,%.2f,%.2f,%u,%.6f,%.6f,%.2f,%lu",
      (unsigned long)millis(), (unsigned)imu.ok,
      imu.ax, imu.ay, imu.az, imu.gx, imu.gy, imu.gz,
      (unsigned)bmeOK, temperature, humidity, pressure,
      (unsigned)gpsFix, latitude, longitude, altitudeM, satellites);
  if (n <= 0 || (size_t)n >= sizeof(line)) {
    emitInfo("INFO,ERROR,DATA_TRUNCATED");
    return;
  }
  usbLog(line);
  sendUdp(line);
}

void setup() {
  Serial.begin(USB_BAUD);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  mpuAvailable = beginMpu();
  bmeAvailable = bme.begin(0x76, &Wire);
  if (!bmeAvailable) bmeAvailable = bme.begin(0x77, &Wire);

  usbLog("INFO,OpenCanSat boot");
  usbLog(mpuAvailable ? "INFO,MPU6050,OK" : "INFO,MPU6050,NOT_FOUND");
  usbLog(bmeAvailable ? "INFO,BME280,OK" : "INFO,BME280,NOT_FOUND");
  usbLog("INFO,GPS,WAITING_FOR_FIX");

  WiFi.mode(WIFI_AP);
  const bool configured = WiFi.softAPConfig(apIP, apIP, apMask);
  // Default platform TX power is preserved; do not override local legal limits.
  wifiReady = configured && WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL, 0, 1);
  if (wifiReady) wifiReady = udp.begin(UDP_PORT) != 0;

  usbLog(wifiReady ? "INFO,WIFI,READY" : "INFO,WIFI,FAILED");
  if (wifiReady && Serial) {
    Serial.print("INFO,WIFI_SSID,"); Serial.println(WIFI_SSID);
    Serial.print("INFO,WIFI_IP,"); Serial.println(WiFi.softAPIP());
    Serial.printf("INFO,UDP_PORT,%u\n", (unsigned)UDP_PORT);
  }
}

void loop() {
  // Drain NMEA UART data as often as possible.
  while (gpsSerial.available() > 0) {
    gps.encode((char)gpsSerial.read());
  }
  receiveHello();
  retryUnavailableSensors();
  const uint32_t now = millis();
  if ((uint32_t)(now - lastTelemetryMs) >= TELEMETRY_INTERVAL_MS) {
    lastTelemetryMs = now;
    transmitTelemetry();
  }
}
