
#include <Wire.h>

#define SDA_PIN 8
#define SCL_PIN 9
#define MPU_ADDR 0x68

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);

    // MPU6050 연결 확인
    Wire.beginTransmission(MPU_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("MPU6050 Failed!");
        while (1) delay(1000);
    }

    // 절전 모드 해제
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B);
    Wire.write(0x00);
    Wire.endTransmission();

    Serial.println("MPU6050 Ready!");
}

void loop() {
    // 가속도 및 자이로 데이터 요청
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);

    if (Wire.requestFrom(MPU_ADDR, 14) != 14) {
        Serial.println("Read Failed!");
        delay(500);
        return;
    }

    int16_t ax = (Wire.read() << 8) | Wire.read();
    int16_t ay = (Wire.read() << 8) | Wire.read();
    int16_t az = (Wire.read() << 8) | Wire.read();

    // 온도 데이터 건너뛰기
    Wire.read();
    Wire.read();

    int16_t gx = (Wire.read() << 8) | Wire.read();
    int16_t gy = (Wire.read() << 8) | Wire.read();
    int16_t gz = (Wire.read() << 8) | Wire.read();

    // 기본 측정 범위 기준 단위 변환
    float accX = ax / 16384.0;
    float accY = ay / 16384.0;
    float accZ = az / 16384.0;

    float gyroX = gx / 131.0;
    float gyroY = gy / 131.0;
    float gyroZ = gz / 131.0;

    Serial.printf("ACC  X: %.2f Y: %.2f Z: %.2f g\n",
                  accX, accY, accZ);

    Serial.printf("GYRO X: %.2f Y: %.2f Z: %.2f deg/s\n\n",
                  gyroX, gyroY, gyroZ);

    delay(500);
}