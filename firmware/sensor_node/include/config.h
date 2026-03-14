/**
 * NexusCap - Sensor Node Configuration
 *
 * Edit these values to match your hardware setup before flashing.
 * Each sensor node must have a unique SENSOR_ID.
 */

#pragma once

// ---------------------------------------------------------------------------
// Sensor identity (0-19, see docs/sensor_map.md for body-part assignments)
// ---------------------------------------------------------------------------
#define SENSOR_ID 0

// ---------------------------------------------------------------------------
// I2C pin assignment
// ---------------------------------------------------------------------------
#define IMU_SDA_PIN 21
#define IMU_SCL_PIN 22

// ---------------------------------------------------------------------------
// MPU6050 interrupt pin (connect INT of MPU6050 to this GPIO)
// ---------------------------------------------------------------------------
#define MPU_INT_PIN 2

// ---------------------------------------------------------------------------
// Hub MAC address
// Set to the MAC address printed by the hub_node on first boot.
// 0xFF bytes = broadcast (works for testing, not recommended in production).
// ---------------------------------------------------------------------------
#define HUB_MAC { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }

// ---------------------------------------------------------------------------
// Target send rate (frames per second)
// ESP-NOW practical maximum is around 100 fps per node.
// ---------------------------------------------------------------------------
#define TARGET_FPS 60

// ---------------------------------------------------------------------------
// ESP-NOW channel (must match hub_node ESPNOW_CHANNEL)
// ---------------------------------------------------------------------------
#define ESPNOW_CHANNEL 1
