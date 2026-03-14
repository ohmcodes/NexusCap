/**
 * NexusCap - Hub Node Firmware
 *
 * Receives IMUPacket frames from all sensor nodes via ESP-NOW,
 * then forwards each frame as a UDP datagram to the PC running
 * nexuscap_hub.py (or the UE5 plugin's built-in receiver).
 *
 * Wire-protocol (UDP datagram layout):
 *   Offset  Size  Description
 *   0       4     Magic "NCAP"
 *   4       1     Protocol version (1)
 *   5       1     sensor_id
 *   6       4     qw  (IEEE-754 float, little-endian)
 *   10      4     qx
 *   14      4     qy
 *   18      4     qz
 *   22      4     ax  (g)
 *   26      4     ay  (g)
 *   30      4     az  (g)
 *   34      4     timestamp (ms, uint32)
 *   Total = 38 bytes
 *
 * Hardware:
 *   - ESP32 dev-board with 2.4 GHz antenna (standard)
 *   - Connected to 2.4 GHz Wi-Fi network (same subnet as PC)
 *
 * Build: PlatformIO  (see platformio.ini)
 * Config: edit include/config.h before flashing
 */

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "config.h"

// ---------------------------------------------------------------------------
// Wire-protocol structs (must match sensor_node/src/main.cpp)
// ---------------------------------------------------------------------------
struct __attribute__((packed)) IMUPacket {
    uint8_t  sensor_id;
    float    qw, qx, qy, qz;
    float    ax, ay, az;
    uint32_t timestamp;
};

struct __attribute__((packed)) UDPPacket {
    uint8_t  magic[4];      // 'N','C','A','P'
    uint8_t  version;       // 1
    uint8_t  sensor_id;
    float    qw, qx, qy, qz;
    float    ax, ay, az;
    uint32_t timestamp;
};

static_assert(sizeof(IMUPacket) == 33, "IMUPacket size mismatch");
static_assert(sizeof(UDPPacket) == 38, "UDPPacket size mismatch");

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
static WiFiUDP  s_udp;
static bool     s_udpReady = false;

// Per-sensor packet counters for basic diagnostics
static uint32_t s_packetCount[20] = {};   // indexed by sensor_id (0-19)
static uint32_t s_lastStatMs      = 0;

// ---------------------------------------------------------------------------
// ESP-NOW receive callback  (called from Wi-Fi task – keep it short)
// ---------------------------------------------------------------------------
static void IRAM_ATTR onDataRecv(const uint8_t* mac,
                                  const uint8_t* data,
                                  int             len)
{
    if (!s_udpReady)              return;
    if (len != sizeof(IMUPacket)) return;

    const IMUPacket* imu = reinterpret_cast<const IMUPacket*>(data);

    UDPPacket pkt;
    pkt.magic[0] = 'N';
    pkt.magic[1] = 'C';
    pkt.magic[2] = 'A';
    pkt.magic[3] = 'P';
    pkt.version   = 1;
    pkt.sensor_id = imu->sensor_id;
    pkt.qw        = imu->qw;
    pkt.qx        = imu->qx;
    pkt.qy        = imu->qy;
    pkt.qz        = imu->qz;
    pkt.ax        = imu->ax;
    pkt.ay        = imu->ay;
    pkt.az        = imu->az;
    pkt.timestamp = imu->timestamp;

    s_udp.beginPacket(PC_IP, UDP_OUT_PORT);
    s_udp.write(reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt));
    s_udp.endPacket();

    // Update counter (no bounds check inside ISR – validated above)
    if (imu->sensor_id < 20) {
        s_packetCount[imu->sensor_id]++;
    }
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(SERIAL_BAUD);
    Serial.println("\n[NexusCap] Hub node starting ...");

    // --- Wi-Fi: station + AP modes so ESP-NOW works while also connected ---
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - t0 > 20000) {
            Serial.println("\n[WiFi] Timeout! Check SSID/password in config.h");
            while (true) { delay(2000); }
        }
        delay(500);
        Serial.print('.');
    }
    Serial.printf("\n[WiFi] Connected  IP=%s\n",
                  WiFi.localIP().toString().c_str());
    Serial.printf("[WiFi] Hub MAC (for sensor_node config.h): %s\n",
                  WiFi.macAddress().c_str());

    // --- UDP output socket ---
    s_udp.begin(UDP_LOCAL_PORT);
    s_udpReady = true;
    Serial.printf("[UDP]  Forwarding to %s:%u\n", PC_IP, (unsigned)UDP_OUT_PORT);

    // --- ESP-NOW ---
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] init FAILED");
        while (true) { delay(1000); }
    }
    esp_now_register_recv_cb(onDataRecv);
    Serial.println("[ESP-NOW] Listening for sensor nodes");

    Serial.println("[NexusCap] Hub node ready");
}

// ---------------------------------------------------------------------------
// loop() – print per-sensor statistics every 5 s
// ---------------------------------------------------------------------------
void loop() {
    uint32_t nowMs = millis();
    if (nowMs - s_lastStatMs >= 5000) {
        s_lastStatMs = nowMs;
        Serial.print("[Stats] packets/5s by sensor id: ");
        bool anyActive = false;
        for (int i = 0; i < 20; i++) {
            if (s_packetCount[i] > 0) {
                Serial.printf(" [%d]=%u", i, s_packetCount[i]);
                s_packetCount[i] = 0;
                anyActive = true;
            }
        }
        if (!anyActive) Serial.print(" (none)");
        Serial.println();
    }
    delay(10);
}
