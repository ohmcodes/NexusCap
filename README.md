# NexusCap


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
