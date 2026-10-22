
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

#define GPS_RX 44
#define GPS_TX 43

TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

void setup() {
    Serial.begin(115200);
    delay(1500);

    gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

    Serial.println("GPS Ready!");
}

void loop() {
    // GPS 데이터 수신
    while (gpsSerial.available()) {
        gps.encode(gpsSerial.read());
    }

    static unsigned long lastTime = 0;

    if (millis() - lastTime >= 1000) {
        lastTime = millis();

        // GPS 수신 상태 확인
        if (gps.charsProcessed() == 0 && millis() > 5000) {
            Serial.println("GPS Communication Failed!");
            return;
        }

        // 위도 및 경도
        if (gps.location.isValid() &&
            gps.location.age() < 5000) {
            Serial.printf("LAT  : %.6f\n", gps.location.lat());
            Serial.printf("LON  : %.6f\n", gps.location.lng());
        } else {
            Serial.println("Waiting for GPS Fix...");
        }

        // 고도
        if (gps.altitude.isValid()) {
            Serial.printf("ALT  : %.2f m\n", gps.altitude.meters());
        }

        // 위성 수
        if (gps.satellites.isValid()) {
            Serial.printf("SAT  : %lu\n", gps.satellites.value());
        }

        Serial.println("--------------------");
    }
}
