#include <esp_now.h>
#include <WiFi.h>
#include <WiFiUdp.h>

WiFiUDP udp;
const char* host = "192.168.1.100"; // PC IP
const int port = 7000;

struct BoneData { int id; float qW, qX, qY, qZ; };
BoneData receivedData;

void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    memcpy(&receivedData, incomingData, sizeof(receivedData));
    
    // Forward to Unreal over UDP
    udp.beginPacket(host, port);
    udp.write((uint8_t*)&receivedData, sizeof(receivedData));
    udp.endPacket();
}

void setup() {
    WiFi.mode(WIFI_AP_STA);
    // Add WiFi connection logic for UDP...
    esp_now_init();
    esp_now_register_recv_cb(onDataRecv);
}
void loop() {}
