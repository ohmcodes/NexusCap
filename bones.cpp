#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h> // Common, cheap IMU

Adafruit_MPU6050 mpu;
uint8_t hubAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Replace with Hub MAC

struct BoneData {
    int id; // 0 for Hips, 1 for Leg, etc.
    float qW, qX, qY, qZ; // Quaternions for rotation
};

void setup() {
    WiFi.mode(WIFI_STA);
    if (!mpu.begin()) while (1);
    if (esp_now_init() != ESP_OK) return;
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, hubAddress, 6);
    esp_now_add_peer(&peerInfo);
}

void loop() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    
    BoneData data = {1, 1.0, g.gyro.x, g.gyro.y, g.gyro.z}; // Simplified orientation
    esp_now_send(hubAddress, (uint8_t *) &data, sizeof(data));
    delay(10); // 100Hz
}
