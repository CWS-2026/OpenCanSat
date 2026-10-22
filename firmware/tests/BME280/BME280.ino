
#include <Wire.h>
#include <Adafruit_BME280.h>

#define SDA_PIN 8
#define SCL_PIN 9

Adafruit_BME280 bme;

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);

    // BME280 연결 확인
    if (!bme.begin(0x76, &Wire) &&
        !bme.begin(0x77, &Wire)) {
        Serial.println("BME280 Failed!");
        while (1) delay(1000);
    }

    Serial.println("BME280 Ready!");
}

void loop() {
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0;

    Serial.printf("TEMP : %.2f C\n", temperature);
    Serial.printf("HUM  : %.2f %%\n", humidity);
    Serial.printf("PRES : %.2f hPa\n\n", pressure);

    delay(1000);
}
