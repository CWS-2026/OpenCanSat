/*
  OpenCanSat - ESP32-S3 Wi-Fi Web Telemetry

  Phone: connect to Wi-Fi "ICanDo<device-number>" / "cbsh@CS1", then open:
         http://opencansat.local/
  Fallback (if .local is unsupported): http://192.168.4.1/

  Hardware:
    MPU6050 + BME280: SDA GPIO8, SCL GPIO9
    NEO-6M: GPS TX -> ESP GPIO44 (RX), GPS RX <- ESP GPIO43 (TX)

  Arduino libraries:
    ESP32 Arduino core (includes WiFi, WebServer, Wire, HardwareSerial)
    Adafruit BME280 Library + dependencies
    TinyGPSPlus

  USB CDC On Boot: Enabled for native USB Serial on ESP32-S3.
  No internet, router, external ground-station Python, or onboard SD required.
  Browser CSV capture lives on the phone until downloaded; not persistent
  inside the satellite. Data missed outside RAM buffer cannot be recovered.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <Adafruit_BME280.h>
#include <TinyGPSPlus.h>
#include <math.h>

// Change these credentials before using the device outside a controlled test.
// Each ESP32-S3 gets a numeric SSID suffix derived from its chip MAC.
// Set AP_NUMBER_OVERRIDE to 1 for "ICanDo1", 2 for "ICanDo2", etc.
// Leave it at 0 to generate a device-specific number automatically.
constexpr uint32_t AP_NUMBER_OVERRIDE = 0;
constexpr char AP_PASSWORD[] = "cbsh@CS1";  // 8 characters (WPA2 minimum)
String apSsid;
constexpr char MDNS_HOSTNAME[] = "opencansat"; // Accessible as opencansat.local if mDNS supported
constexpr uint8_t AP_CHANNEL = 6;

constexpr int SDA_PIN = 8;
constexpr int SCL_PIN = 9;
constexpr int GPS_RX_PIN = 44; // ESP RX <- GPS TX
constexpr int GPS_TX_PIN = 43; // ESP TX -> GPS RX
constexpr uint8_t MPU_ADDR = 0x68;

constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t GPS_BAUD = 9600;
constexpr uint32_t SAMPLE_MS = 200;          // 5 samples per second
constexpr uint32_t GPS_VALID_AGE_MS = 5000;
constexpr uint16_t HISTORY_CAPACITY = 256; // ~51 seconds at 5 Hz (RAM only)
constexpr uint16_t API_BATCH_SIZE = 24;

WebServer server(80);
Adafruit_BME280 bme;
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

bool mpuFound = false;
bool bmeFound = false;
bool apReady = false;

struct Sample {
  uint32_t seq = 0;
  uint32_t ms = 0;
  bool mpuOk = false;
  bool bmeOk = false;
  bool gpsFix = false;
  float ax = NAN, ay = NAN, az = NAN;   // g
  float gx = NAN, gy = NAN, gz = NAN;   // deg/s
  float temp = NAN, humidity = NAN, pressure = NAN; // C, %, hPa
  double lat = NAN, lon = NAN;
  float gpsAlt = NAN;                    // m
  uint32_t satellites = 0;
};

Sample history[HISTORY_CAPACITY];
uint32_t currentSeq = 0;
uint32_t lastSampleMs = 0;

bool writeMpuRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool initMpu() {
  Wire.beginTransmission(MPU_ADDR);
  if (Wire.endTransmission() != 0) return false;
  return writeMpuRegister(0x6B, 0x00) && // wake up
         writeMpuRegister(0x1B, 0x00) && // gyro +/-250 deg/s
         writeMpuRegister(0x1C, 0x00);   // accel +/-2 g
}

void readMpu(Sample &s) {
  if (!mpuFound) return;
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return;
  if (Wire.requestFrom(MPU_ADDR, (uint8_t)14) != 14) return;

  const int16_t ax = (int16_t)((Wire.read() << 8) | Wire.read());
  const int16_t ay = (int16_t)((Wire.read() << 8) | Wire.read());
  const int16_t az = (int16_t)((Wire.read() << 8) | Wire.read());
  Wire.read(); Wire.read(); // skip MPU temperature
  const int16_t gx = (int16_t)((Wire.read() << 8) | Wire.read());
  const int16_t gy = (int16_t)((Wire.read() << 8) | Wire.read());
  const int16_t gz = (int16_t)((Wire.read() << 8) | Wire.read());

  s.ax = ax / 16384.0f;
  s.ay = ay / 16384.0f;
  s.az = az / 16384.0f;
  s.gx = gx / 131.0f;
  s.gy = gy / 131.0f;
  s.gz = gz / 131.0f;
  s.mpuOk = true;
}

void printSerialData(const Sample &s) {
  // DATA,device_ms,mpu_ok,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,
  // bme_ok,temp_c,humidity_pct,pressure_hpa,gps_fix,latitude,longitude,
  // gps_altitude_m,satellites
  Serial.printf(
    "DATA,%lu,%u,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%.2f,%.2f,%.2f,%u,%.6f,%.6f,%.2f,%lu\n",
    (unsigned long)s.ms, (unsigned)s.mpuOk,
    s.ax, s.ay, s.az, s.gx, s.gy, s.gz,
    (unsigned)s.bmeOk, s.temp, s.humidity, s.pressure,
    (unsigned)s.gpsFix, s.lat, s.lon, s.gpsAlt,
    (unsigned long)s.satellites
  );
}

void takeSample() {
  Sample s;
  s.seq = ++currentSeq;
  s.ms = millis();
  readMpu(s);

  if (bmeFound) {
    s.temp = bme.readTemperature();
    s.humidity = bme.readHumidity();
    s.pressure = bme.readPressure() / 100.0f;
    s.bmeOk = isfinite(s.temp) && isfinite(s.humidity) && isfinite(s.pressure);
    if (!s.bmeOk) s.temp = s.humidity = s.pressure = NAN;
  }

  s.gpsFix = gps.location.isValid() && gps.location.age() < GPS_VALID_AGE_MS;
  if (s.gpsFix) {
    s.lat = gps.location.lat();
    s.lon = gps.location.lng();
    if (gps.altitude.isValid() && gps.altitude.age() < GPS_VALID_AGE_MS) {
      s.gpsAlt = gps.altitude.meters();
    }
  }
  if (gps.satellites.isValid() && gps.satellites.age() < GPS_VALID_AGE_MS) {
    s.satellites = gps.satellites.value();
  }

  history[s.seq % HISTORY_CAPACITY] = s;
  printSerialData(s);
}

void appendFloat(String &out, double value, unsigned decimals) {
  if (isfinite(value)) out += String(value, decimals);
  else out += F("null");
}

void appendJsonSample(String &json, const Sample &s) {
  json += F("{\"seq\":"); json += s.seq;
  json += F(",\"device_ms\":"); json += s.ms;
  json += F(",\"mpu_ok\":"); json += (s.mpuOk ? '1' : '0');
  json += F(",\"ax_g\":"); appendFloat(json, s.ax, 3);
  json += F(",\"ay_g\":"); appendFloat(json, s.ay, 3);
  json += F(",\"az_g\":"); appendFloat(json, s.az, 3);
  json += F(",\"gx_dps\":"); appendFloat(json, s.gx, 3);
  json += F(",\"gy_dps\":"); appendFloat(json, s.gy, 3);
  json += F(",\"gz_dps\":"); appendFloat(json, s.gz, 3);
  json += F(",\"bme_ok\":"); json += (s.bmeOk ? '1' : '0');
  json += F(",\"temp_c\":"); appendFloat(json, s.temp, 2);
  json += F(",\"humidity_pct\":"); appendFloat(json, s.humidity, 2);
  json += F(",\"pressure_hpa\":"); appendFloat(json, s.pressure, 2);
  json += F(",\"gps_fix\":"); json += (s.gpsFix ? '1' : '0');
  json += F(",\"latitude\":"); appendFloat(json, s.lat, 6);
  json += F(",\"longitude\":"); appendFloat(json, s.lon, 6);
  json += F(",\"gps_altitude_m\":"); appendFloat(json, s.gpsAlt, 2);
  json += F(",\"satellites\":"); json += s.satellites;
  json += '}';
}

// A minimal onboard site. The eventual full phone ground station can use
// GET /api/telemetry?after=<sequence> without changing the sensor code.
const char PHONE_PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>OpenCanSat | Ground Station</title>
<style>
:root{color-scheme:dark;font-family:system-ui,-apple-system,sans-serif;background:#0d1320;color:#f0f4ff}
*{box-sizing:border-box}body{margin:0 auto;max-width:760px;padding:22px 16px}
header{display:flex;justify-content:space-between;align-items:center;gap:12px}
h1{margin:0;font-size:25px}.sub{color:#94a7c3;font-size:13px;margin:7px 0 24px}
#link{border:1px solid #52647e;border-radius:30px;padding:7px 11px;font-size:12px;white-space:nowrap}
.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}
.card{border:1px solid #26354b;background:#151f30;border-radius:13px;padding:15px}
.card label{display:block;color:#9fb1ca;font-size:12px;margin-bottom:6px}
.card strong{font-size:24px;font-variant-numeric:tabular-nums;overflow-wrap:anywhere}
.full{grid-column:1/-1}h2{font-size:15px;margin:24px 0 12px}
button{background:#dbe7ff;color:#0c192e;border:0;border-radius:9px;font-weight:700;padding:12px 14px;margin:6px 8px 0 0}
button.secondary{background:#26354b;color:white}
#log{background:#08101d;border:1px solid #26354b;border-radius:10px;padding:12px;height:160px;overflow:auto;font:12px/1.6 ui-monospace,monospace;white-space:pre-wrap}
.note{font-size:12px;color:#91a5c3;line-height:1.5}
</style>
</head>
<body>
<header><h1>OpenCanSat</h1><span id="link">CONNECTING...</span></header>
<p class="sub">opencansat.local · 5 Hz · no internet required</p>
<div class="grid">
<div class="card"><label>Temperature</label><strong id="temp">--</strong></div>
<div class="card"><label>Humidity</label><strong id="hum">--</strong></div>
<div class="card"><label>Pressure</label><strong id="pres">--</strong></div>
<div class="card"><label>GPS altitude</label><strong id="alt">--</strong></div>
<div class="card"><label>Acceleration XYZ (g)</label><strong id="acc" style="font-size:17px">--</strong></div>
<div class="card"><label>Gyro XYZ (deg/s)</label><strong id="gyro" style="font-size:17px">--</strong></div>
<div class="card full"><label>GPS</label><strong id="gps" style="font-size:17px">Waiting for fix</strong></div>
<div class="card"><label>Sensor status</label><strong id="sensors" style="font-size:16px">--</strong></div>
<div class="card"><label>Samples collected by phone</label><strong id="count">0</strong></div>
</div>
<h2>Phone-side CSV recording</h2>
<button id="download">Download CSV</button><button class="secondary" id="clear">Clear phone data</button>
<p class="note">Samples are held in this browser tab and are saved to your phone only when you tap Download CSV. Keep this page open during the test. Up to ~51 seconds of recent samples are buffered in satellite RAM to bridge short disconnections; this is not permanent storage.</p>
<h2>Connection log</h2><div id="log"></div>
<script>
const keys=['device_ms','mpu_ok','ax_g','ay_g','az_g','gx_dps','gy_dps','gz_dps','bme_ok','temp_c','humidity_pct','pressure_hpa','gps_fix','latitude','longitude','gps_altitude_m','satellites'];
const rows=[];let lastSeq=0,busy=false,lastConnected=0;
const el=id=>document.getElementById(id);
function log(message){const box=el('log');box.textContent='['+new Date().toLocaleTimeString()+'] '+message+'\n'+box.textContent.slice(0,4000)}
function fmt(v,d=2){return v===null||v===undefined?'--':Number(v).toFixed(d)}
function display(s){
 el('temp').textContent=fmt(s.temp_c)+' °C';el('hum').textContent=fmt(s.humidity_pct)+' %';
 el('pres').textContent=fmt(s.pressure_hpa)+' hPa';el('alt').textContent=fmt(s.gps_altitude_m)+' m';
 el('acc').textContent=[s.ax_g,s.ay_g,s.az_g].map(v=>fmt(v,3)).join(' / ');
 el('gyro').textContent=[s.gx_dps,s.gy_dps,s.gz_dps].map(v=>fmt(v,2)).join(' / ');
 el('gps').textContent=s.gps_fix?fmt(s.latitude,6)+', '+fmt(s.longitude,6)+' · '+s.satellites+' sats':'Waiting for GPS fix · '+s.satellites+' sats';
 el('sensors').textContent='MPU '+(s.mpu_ok?'OK':'ERR')+' · BME '+(s.bme_ok?'OK':'ERR');
 el('count').textContent=rows.length;
}
async function poll(){
 if(busy)return;busy=true;
 try{
  const response=await fetch('/api/telemetry?after='+lastSeq,{cache:'no-store'});
  if(!response.ok)throw Error('HTTP '+response.status);
  const packet=await response.json();
  if(lastSeq&&packet.oldest_seq>lastSeq+1){log('WARNING: '+(packet.oldest_seq-lastSeq-1)+' older samples lost from buffer');lastSeq=packet.oldest_seq-1}
  for(const s of packet.samples){
   if(s.seq<=lastSeq)continue;
   const stamp=new Date().toISOString();rows.push([stamp,...keys.map(k=>s[k]??'')]);
   lastSeq=s.seq;display(s);
  }
  if(!lastConnected)log('Connected to satellite web telemetry');
  lastConnected=Date.now();el('link').textContent='LIVE · '+lastSeq;
 }catch(e){el('link').textContent='SIGNAL LOST';if(lastConnected&&Date.now()-lastConnected>2000){log('Connection interrupted');lastConnected=0}}
 finally{busy=false}
}
function cell(v){let s=String(v??'');return /[",\n\r]/.test(s)?'"'+s.replace(/"/g,'""')+'"':s}
el('download').onclick=()=>{
 const csv=[['received_at_utc',...keys],...rows].map(r=>r.map(cell).join(',')).join('\r\n')+'\r\n';
 const blob=new Blob([csv],{type:'text/csv;charset=utf-8'});
 const url=URL.createObjectURL(blob);const a=document.createElement('a');a.href=url;
 a.download='OpenCanSat_'+new Date().toISOString().replace(/[:.]/g,'-')+'.csv';
 document.body.appendChild(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),1000);
 log('CSV download requested ('+rows.length+' rows)');
};
el('clear').onclick=()=>{if(confirm('Discard phone-side samples?')){rows.length=0;el('count').textContent='0';log('Phone-side CSV cleared')}};
setInterval(poll,250);poll();
</script>
</body>
</html>
)HTML";

void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/html; charset=utf-8", FPSTR(PHONE_PAGE));
}

void handleTelemetry() {
  const String afterStr = server.arg("after");
  const uint32_t requestedAfter = afterStr.length() ? (uint32_t)strtoul(afterStr.c_str(), nullptr, 10) : 0;
  const uint32_t oldest = currentSeq >= HISTORY_CAPACITY
                         ? currentSeq - HISTORY_CAPACITY + 1 : (currentSeq ? 1 : 0);
  uint32_t next = requestedAfter + 1;
  if (next < oldest || requestedAfter > currentSeq) next = oldest;

  String json;
  json.reserve(6500);
  json += F("{\"latest_seq\":"); json += currentSeq;
  json += F(",\"oldest_seq\":"); json += oldest;
  json += F(",\"samples\":[");
  uint16_t count = 0;
  while (next <= currentSeq && count < API_BATCH_SIZE) {
    if (count) json += ',';
    appendJsonSample(json, history[next % HISTORY_CAPACITY]);
    ++next;
    ++count;
  }
  json += F("]}");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(200);
  Serial.println("INFO,OpenCanSat boot");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  Wire.setTimeOut(50);
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  mpuFound = initMpu();
  bmeFound = bme.begin(0x76, &Wire);
  if (!bmeFound) bmeFound = bme.begin(0x77, &Wire);
  Serial.printf("INFO,MPU6050,%s\n", mpuFound ? "OK" : "NOT_FOUND");
  Serial.printf("INFO,BME280,%s\n", bmeFound ? "OK" : "NOT_FOUND");
  Serial.println("INFO,GPS,WAITING_FOR_FIX");

  // Build a numeric network name such as ICanDo123456.
  const uint32_t number = AP_NUMBER_OVERRIDE != 0
                            ? AP_NUMBER_OVERRIDE
                            : (uint32_t)(ESP.getEfuseMac() % 1000000ULL);
  apSsid = "ICanDo" + String(number);
  WiFi.mode(WIFI_AP);
  const IPAddress ip(192, 168, 4, 1);
  const IPAddress mask(255, 255, 255, 0);
  if (!WiFi.softAPConfig(ip, ip, mask)) {
    Serial.println("ERROR,WiFi_AP_IP_CONFIG_FAILED");
  }
  apReady = WiFi.softAP(apSsid.c_str(), AP_PASSWORD, AP_CHANNEL, 0, 4);
  if (apReady) {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/telemetry", HTTP_GET, handleTelemetry);
    server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
    server.begin();
    // mDNS advertises opencansat.local on the local Wi-Fi network.
    // Some phones (especially Android variants) may not resolve .local.
    const bool mdnsReady = MDNS.begin(MDNS_HOSTNAME);
    if (mdnsReady) {
      MDNS.addService("http", "tcp", 80);
      Serial.println("INFO,MDNS_URL,http://opencansat.local/");
    } else {
      Serial.println("WARN,MDNS_START_FAILED_USE_IP");
    }
    Serial.printf("INFO,WIFI_AP,%s\n", apSsid.c_str());
    Serial.print("INFO,WEB_IP_URL,http://"); Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("ERROR,WIFI_AP_START_FAILED");
  }
  Serial.println("INFO,TELEMETRY_RATE,5Hz");
  takeSample();
  lastSampleMs = millis();
}

void loop() {
  // Keep consuming GPS NMEA data even when no phone is connected.
  while (gpsSerial.available() > 0) {
    gps.encode((char)gpsSerial.read());
  }
  if (apReady) server.handleClient();

  const uint32_t now = millis();
  if (now - lastSampleMs >= SAMPLE_MS) {
    lastSampleMs = now;
    takeSample();
  }
  delay(1);
}
