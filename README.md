# NexusCap
**Low-Latency Open-Source IMU Motion Capture for UE5**

NexusCap bridges the gap between expensive professional motion-capture suites and budget-conscious indie developers by using affordable ESP32 hardware and Unreal Engine 5's Live Link + Control Rig pipeline.  Each body segment gets a tiny ESP32 + MPU6050 sensor node.  The nodes broadcast quaternion data wirelessly over ESP-NOW to a single hub node, which forwards the stream as UDP datagrams to your PC.  The UE5 plugin receives those datagrams and drives a skeletal mesh in real time — ready to record into the Sequencer.

---

## Architecture

```
┌────────────────────────────────────────────────────────────────────────┐
│  Sensor Layer (up to 20 ESP32 + MPU6050 nodes)                         │
│                                                                        │
│  [Head]──┐  [Chest]──┐  [Hips]──┐  [L.Arm]──┐  [R.Arm]──┐  ...       │
│           │            │           │            │            │          │
│           └────────────┴───────────┴────────────┴────────────┘         │
│                               ESP-NOW (~100 fps)                        │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                          ┌─────────▼──────────┐
                          │   Hub Node (ESP32)  │
                          │  ESP-NOW → UDP/Wi-Fi│
                          └─────────┬───────────┘
                                    │ UDP  (38 bytes/frame, port 9000)
                          ┌─────────▼───────────┐
                          │  nexuscap_hub.py     │  (optional relay/recorder)
                          │  Python 3 server     │
                          └─────────┬────────────┘
                                    │ UDP  (port 9001)
                          ┌─────────▼───────────┐
                          │  UE5  NexusCap       │
                          │  Live Link Plugin    │
                          │  + Control Rig       │
                          └──────────────────────┘
```

---

## Repository Layout

```
NexusCap/
├── firmware/
│   ├── sensor_node/          # ESP32 IMU node  (PlatformIO project)
│   │   ├── platformio.ini
│   │   ├── include/config.h  ← set SENSOR_ID and HUB_MAC here
│   │   └── src/main.cpp
│   └── hub_node/             # ESP32 UDP gateway  (PlatformIO project)
│       ├── platformio.ini
│       ├── include/config.h  ← set Wi-Fi credentials and PC_IP here
│       └── src/main.cpp
├── hub/
│   ├── nexuscap_hub.py       # Python relay / recorder / debugger
│   ├── nexuscap_replay.py    # Replay recorded sessions
│   └── requirements.txt      # (stdlib only – no pip install needed)
├── ue5_plugin/
│   └── NexusCap/             # Drop into <YourProject>/Plugins/
│       ├── NexusCap.uplugin
│       └── Source/NexusCap/
│           ├── NexusCap.Build.cs
│           ├── Public/
│           │   ├── NexusCapTypes.h               # Wire protocol + bone map
│           │   ├── NexusCapLiveLinkSource.h
│           │   └── NexusCapLiveLinkSourceFactory.h
│           └── Private/
│               ├── NexusCapModule.cpp
│               ├── NexusCapLiveLinkSource.cpp
│               └── NexusCapLiveLinkSourceFactory.cpp
└── docs/
    ├── sensor_map.md         # Sensor ID → bone name table
    └── wiring_guide.md       # Hardware wiring + BOM
```

---

## Quick Start

### 1 — Build the firmware

Install [PlatformIO](https://platformio.org/) (VS Code extension or CLI).

**Hub node** (flash once to your gateway ESP32):
```bash
cd firmware/hub_node
# Edit include/config.h: set WIFI_SSID, WIFI_PASSWORD, PC_IP
pio run --target upload
# Open serial monitor and note the printed Hub MAC address
pio device monitor
```

**Sensor nodes** (flash once per IMU board, each with a different SENSOR_ID):
```bash
cd firmware/sensor_node
# Edit include/config.h: set SENSOR_ID (0–19) and HUB_MAC
pio run --target upload
```

See `docs/sensor_map.md` for the sensor-ID → body-part table and `docs/wiring_guide.md` for the ESP32 ↔ MPU6050 pin connections.

---

### 2 — Run the Python hub (optional)

The Python hub is useful for:
- Debugging (see live FPS per sensor)
- Recording sessions to a file for offline replay
- Relaying data when UE5 is on a different machine

```bash
cd hub
# Receive on port 9000, relay to UE5 on port 9001 (same machine)
python nexuscap_hub.py

# Record a session
python nexuscap_hub.py --record my_take.bin

# Replay a recording
python nexuscap_replay.py my_take.bin --loop
```

Run `python nexuscap_hub.py --help` for all options.

> **Tip:** If UE5 is running on the same PC as the hub, the hub listens on port 9000 (from ESP32) and relays to port 9001 (to UE5).  
> If you want UE5 to receive directly from the ESP32 hub, point the ESP32's `PC_IP` / `UDP_OUT_PORT` at UE5 and configure the plugin port to `9000`.

---

### 3 — Install the UE5 plugin

1. Copy `ue5_plugin/NexusCap/` into your project's `Plugins/` folder:
   ```
   <YourProject>/Plugins/NexusCap/
   ```
2. Re-generate project files (right-click `.uproject` → *Generate Visual Studio project files*) and rebuild.
3. Enable the plugin in **Edit → Plugins → Animation → NexusCap IMU**.

---

### 4 — Connect Live Link

1. Open **Window → Live Link**.
2. Click **+ Source → NexusCap IMU**.
3. Set the **UDP Listen Port** to `9001` (or whichever port your hub relays to).
4. Click **Connect**.

The subject **NexusCap_Body** will appear.  Its bones match the UE5 Mannequin / MetaHuman naming convention.

---

### 5 — Apply to a skeletal mesh (Control Rig)

1. In your Character Blueprint, add a **Live Link Pose** node to the AnimGraph.
2. Set **Subject Name** to `NexusCap_Body`.
3. Set **Role** to **Animation**.
4. For full retargeting, add a **Control Rig** node and connect the Live Link output.

The [UE5 Control Rig documentation](https://docs.unrealengine.com/5.0/en-US/control-rig-in-unreal-engine/) covers retargeting and Sequencer recording in depth.

---

### 6 — Record into Sequencer

1. Open the **Sequencer**.
2. Click **+ Track → Live Link Track** and select `NexusCap_Body`.
3. Press **Record** in Sequencer and perform your motion.
4. Press **Stop** — the animation is baked into the Level Sequence.

---

## Wire Protocol

Every UDP datagram is exactly **38 bytes**, little-endian:

| Offset | Size | Type     | Field       | Description                    |
|--------|------|----------|-------------|--------------------------------|
| 0      | 4    | `char[4]`| `magic`     | `"NCAP"`                       |
| 4      | 1    | `uint8`  | `version`   | Protocol version (`1`)         |
| 5      | 1    | `uint8`  | `sensor_id` | 0–19 (see `docs/sensor_map.md`)|
| 6      | 4    | `float32`| `qw`        | Quaternion W                   |
| 10     | 4    | `float32`| `qx`        | Quaternion X                   |
| 14     | 4    | `float32`| `qy`        | Quaternion Y                   |
| 18     | 4    | `float32`| `qz`        | Quaternion Z                   |
| 22     | 4    | `float32`| `ax`        | Linear accel X (g)             |
| 26     | 4    | `float32`| `ay`        | Linear accel Y (g)             |
| 30     | 4    | `float32`| `az`        | Linear accel Z (g)             |
| 34     | 4    | `uint32` | `timestamp` | Sensor `millis()` value (ms)   |

---

## Performance

| Metric                      | Value              |
|-----------------------------|--------------------|
| ESP-NOW data rate per node  | up to 100 fps      |
| Target capture rate         | 60 fps (default)   |
| UDP datagram size           | 38 bytes           |
| End-to-end latency (LAN)    | < 5 ms typical     |
| Supported sensor count      | up to 20           |
| Hardware cost per sensor    | ~$9–14             |

---

## Supported Hardware

| Component     | Recommended                  | Notes                           |
|---------------|------------------------------|---------------------------------|
| MCU           | ESP32 (any 38-pin dev-board) | ESP32-S3 also works             |
| IMU           | MPU6050                      | Very cheap; DMP provides quaternion output |
| Hub           | ESP32 (no IMU needed)        | Only needs Wi-Fi                |
| Power (sensor)| 3.7 V LiPo + TP4056          | ~4 h runtime on 500 mAh cell   |

---

## License

This project is released under the **MIT License**.  
See [LICENSE](LICENSE) for details.

---

## Contributing

Pull requests are welcome!  Please open an issue first for significant changes.


# NexusCap: Open-Source ESP32 Motion Capture for UE5
Low-Latency Open-Source IMU Motion Capture for UE5

NexusCap is a high-performance, budget-friendly motion capture system designed for indie developers. It uses 7+ ESP32 nodes to track body rotation and streams data directly into Unreal Engine 5 using ESP-NOW and UDP.

## ✨ Key Features
- **Low Latency:** Uses ESP-NOW protocol to bypass Wi-Fi congestion.
- **UE5 Native:** Deep integration with Take Recorder, Sequencer, and Control Rig.
- **Physical Controls:** Main hub features buttons for "One-Touch Record" and "Instant Calibrate."
- **Small Form Factor:** Designed for portability and ease of use.

## 🛠 Technology Stack
- **Hardware:** ESP32 (Nodes), MPU6050/BNO055 (IMUs), ESP32 (Central Hub).
- **Communication:** ESP-NOW (Node to Hub), UDP (Hub to PC).
- **Unreal Engine Plugins:** 
    - [UDP-Unreal](https://github.com/getnamo/udp-unreal) by Getnamo.
    - Take Recorder / Takes Core.
    - Sequencer Scripting.

## 🚀 Getting Started

### Hardware Setup
1. Flash the `Node_Firmware` to your 7 bone-point ESP32s.
2. Flash the `Hub_Firmware` to your main central ESP32.
3. Connect the Hub to your PC via USB or local Wi-Fi.

### Software Setup
1. Clone this repo into your Unreal Project's `Plugins` folder.
2. Open the **NexusCap Control Rig** and assign your UDP port (default: 7000).
3. Open the **Take Recorder** and ensure "NexusCap_Source" is active.

## 🎮 Usage
- **Single Press (Hub Button):** Start/Stop Take Recorder.
- **Double Press/Hold:** Calibrate sensors to T-Pose.

## 📄 License
Distributed under the MIT License. See `LICENSE` for more information.

## 🙌 Acknowledgments
- Thanks to **Getnamo** for the UDP-Unreal plugin.
- Inspired by the indie dev community's need for accessible MoCap.
