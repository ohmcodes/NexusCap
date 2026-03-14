/**
 * NexusCap - Hub Node Configuration
 *
 * Edit these values to match your network before flashing.
 */

#pragma once

// ---------------------------------------------------------------------------
// Wi-Fi credentials  (hub connects to your router to reach the PC)
// ---------------------------------------------------------------------------
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ---------------------------------------------------------------------------
// PC / server address
// Set to the static LAN IP of the machine running nexuscap_hub.py (or UE5).
// ---------------------------------------------------------------------------
#define PC_IP   "192.168.1.100"
#define UDP_OUT_PORT 9000   // port the hub sends TO

// ---------------------------------------------------------------------------
// Local port the hub listens on (reserved – not currently used for incoming)
// ---------------------------------------------------------------------------
#define UDP_LOCAL_PORT 9001

// ---------------------------------------------------------------------------
// ESP-NOW channel (must match sensor_node ESPNOW_CHANNEL)
// ---------------------------------------------------------------------------
#define ESPNOW_CHANNEL 1

// ---------------------------------------------------------------------------
// Serial baud rate
// ---------------------------------------------------------------------------
#define SERIAL_BAUD 115200
