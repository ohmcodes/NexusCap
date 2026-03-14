/**
 * NexusCap - Sensor Node Firmware
 *
 * Reads orientation quaternion + linear acceleration from an MPU6050 via DMP,
 * then broadcasts the result to the hub node over ESP-NOW at TARGET_FPS.
 *
 * Hardware:
 *   - ESP32 dev-board (any 38-pin variant)
 *   - MPU6050 breakout  SDA->GPIO21  SCL->GPIO22  INT->GPIO2  VCC->3V3  GND->GND
 *
 * Build: PlatformIO  (see platformio.ini)
 * Config: edit include/config.h before flashing
 */

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include "MPU6050_6Axis_MotionApps20.h"
#include "config.h"

// ---------------------------------------------------------------------------
// Wire-protocol packet (shared with hub_node and hub server)
// Must be kept in sync with:
//   firmware/hub_node/src/main.cpp
//   hub/nexuscap_hub.py
//   ue5_plugin/.../NexusCapTypes.h
// ---------------------------------------------------------------------------
struct __attribute__((packed)) IMUPacket {
    uint8_t  sensor_id;   // 0-18  (see docs/sensor_map.md)
    float    qw;          // quaternion  w
    float    qx;          //             x
    float    qy;          //             y
    float    qz;          //             z
    float    ax;          // linear acceleration  x  (g)
    float    ay;          //                      y  (g)
    float    az;          //                      z  (g)
    uint32_t timestamp;   // millis() on sensor node
};

static_assert(sizeof(IMUPacket) == 33, "IMUPacket size mismatch");

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
static uint8_t    s_hubMac[]  = HUB_MAC;
static MPU6050    s_mpu;
static bool       s_dmpReady  = false;
static uint16_t   s_packetSize;
static uint8_t    s_fifoBuffer[64];

// Interval between sends (microseconds)
static const uint32_t SEND_INTERVAL_US = 1000000UL / TARGET_FPS;
static uint32_t   s_lastSendUs = 0;

// ISR flag set by MPU6050 INT pin
static volatile bool s_mpuInterrupt = false;
void IRAM_ATTR dmpDataReady() { s_mpuInterrupt = true; }

// ---------------------------------------------------------------------------
// ESP-NOW send callback (optional: log failures)
// ---------------------------------------------------------------------------
static void onDataSent(const uint8_t* /*mac*/, esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
        Serial.println("[ESP-NOW] send failed");
    }
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    Serial.printf("\n[NexusCap] Sensor node  id=%u  target=%u fps\n",
                  (unsigned)SENSOR_ID, (unsigned)TARGET_FPS);

    // --- I2C + MPU6050 ---
    Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN);
    Wire.setClock(400000);   // 400 kHz fast-mode

    s_mpu.initialize();
    if (!s_mpu.testConnection()) {
        Serial.println("[MPU6050] connection FAILED – check wiring");
        while (true) { delay(1000); }
    }
    Serial.println("[MPU6050] connection OK");

    uint8_t devStatus = s_mpu.dmpInitialize();
    if (devStatus != 0) {
        Serial.printf("[MPU6050] DMP init failed (code %u)\n", devStatus);
        while (true) { delay(1000); }
    }

    // Gyro / accel calibration (6-step, ~3 seconds each)
    Serial.println("[MPU6050] Calibrating – keep sensor still ...");
    s_mpu.CalibrateAccel(6);
    s_mpu.CalibrateGyro(6);
    s_mpu.PrintActiveOffsets();

    s_mpu.setDMPEnabled(true);
    attachInterrupt(digitalPinToInterrupt(MPU_INT_PIN), dmpDataReady, RISING);
    s_dmpReady  = true;
    s_packetSize = s_mpu.dmpGetFIFOPacketSize();
    Serial.printf("[MPU6050] DMP ready  packet=%u bytes\n", s_packetSize);

    // --- Wi-Fi (STA, no AP needed) ---
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();  // ensure we don't associate

    // --- ESP-NOW ---
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] init FAILED");
        while (true) { delay(1000); }
    }
    esp_now_register_send_cb(onDataSent);

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, s_hubMac, 6);
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;
    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println("[ESP-NOW] add peer FAILED");
        while (true) { delay(1000); }
    }

    Serial.println("[NexusCap] Sensor node running");
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------
void loop() {
    if (!s_dmpReady) return;

    // Throttle to TARGET_FPS even if DMP fires faster
    uint32_t nowUs = micros();
    if ((nowUs - s_lastSendUs) < SEND_INTERVAL_US) return;

    // Wait for DMP interrupt or for FIFO to have data
    if (!s_mpuInterrupt && s_mpu.getFIFOCount() < s_packetSize) return;
    s_mpuInterrupt = false;

    uint8_t  mpuIntStatus = s_mpu.getIntStatus();
    uint16_t fifoCount    = s_mpu.getFIFOCount();

    // FIFO overflow – reset and continue
    if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
        s_mpu.resetFIFO();
        Serial.println("[MPU6050] FIFO overflow – reset");
        return;
    }

    if (!(mpuIntStatus & 0x02)) return;   // no DMP data int

    // Read one packet from FIFO
    while (fifoCount < s_packetSize) fifoCount = s_mpu.getFIFOCount();
    s_mpu.getFIFOBytes(s_fifoBuffer, s_packetSize);

    // Parse quaternion + accelerometer
    Quaternion     q;
    VectorInt16    aa;
    VectorInt16    aaReal;
    VectorFloat    gravity;

    s_mpu.dmpGetQuaternion(&q, s_fifoBuffer);
    s_mpu.dmpGetAccel(&aa, s_fifoBuffer);
    s_mpu.dmpGetGravity(&gravity, &q);
    s_mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);

    // Pack and send
    IMUPacket pkt;
    pkt.sensor_id = SENSOR_ID;
    pkt.qw        = q.w;
    pkt.qx        = q.x;
    pkt.qy        = q.y;
    pkt.qz        = q.z;
    // Convert raw 16-bit accel to g  (±2g range → 16384 LSB/g)
    pkt.ax        = aaReal.x / 16384.0f;
    pkt.ay        = aaReal.y / 16384.0f;
    pkt.az        = aaReal.z / 16384.0f;
    pkt.timestamp = millis();

    esp_now_send(s_hubMac, reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt));
    s_lastSendUs = nowUs;
}
