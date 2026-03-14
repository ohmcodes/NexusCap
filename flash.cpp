#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>

// MAC Address of Master Hub (Get via Serial.println(WiFi.macAddress()))
uint8_t masterAddr[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 

struct __attribute__((packed)) MocapPacket {
    uint8_t id = 1;      // Unique ID for each limb
    int16_t w, x, y, z;  // Compressed Quaternions (* 32767)
    uint8_t cmd = 0;     // 1=Rec, 2=Calibrate
};

MocapPacket data;
unsigned long lastPress = 0;
int pressCount = 0;

void setup() {
    pinMode(2, INPUT_PULLUP);
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) return;

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, masterAddr, 6);
    esp_now_add_peer(&peerInfo);
}

void loop() {
    // Input Logic: Single Click = Rec, Double Click = Calibrate
    if (digitalRead(2) == LOW) {
        if (millis() - lastPress > 200) { pressCount++; lastPress = millis(); }
        delay(100); 
    }

    if (pressCount > 0 && (millis() - lastPress > 400)) {
        data.cmd = (pressCount == 1) ? 1 : 2;
        pressCount = 0;
    } else { data.cmd = 0; }

    // Replace with real IMU data fetch
    data.w = 32767; // Placeholder for Quat W
    esp_now_send(masterAddr, (uint8_t *) &data, sizeof(data));
    delay(10);
}
