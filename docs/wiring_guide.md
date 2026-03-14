# Wiring Guide

## Sensor Node (ESP32 + MPU6050)

```
ESP32            MPU6050
GPIO 21 (SDA) -> SDA
GPIO 22 (SCL) -> SCL
GPIO  2       -> INT
3V3           -> VCC
GND           -> GND
```

### Notes
- Use short wires (< 10 cm) between ESP32 and MPU6050 to minimise I2C noise.
- The MPU6050 AD0 pin should be pulled LOW (to GND) for the default I²C address `0x68`.
- If you need two MPU6050 on the same I²C bus, pull AD0 HIGH on one of them (address becomes `0x69`), and adjust the `MPU6050` constructor call accordingly – but a dedicated ESP32 per sensor is the recommended approach.

## Hub Node (ESP32 only)

The hub node does not need an IMU.  It only needs a Wi-Fi connection to forward UDP packets to the PC.

```
ESP32 hub  ->  Wi-Fi router  ->  PC running nexuscap_hub.py (or UE5 directly)
```

## Power

- Sensor nodes can be powered by a small LiPo battery via a TP4056-based charger board.
- Recommended battery: 3.7 V 500 mAh LiPo per sensor node.
- Alternatively, use a USB power bank if mobility is not a concern.

## Recommended Hardware BOM (per sensor node)

| Component              | Approximate Cost |
|------------------------|-----------------|
| ESP32 dev-board        | $3–5            |
| MPU6050 breakout       | $1–2            |
| 3.7 V 500 mAh LiPo     | $3–5            |
| TP4056 charger module  | $0.50           |
| Mini breadboard + wire | $1              |
| **Total per sensor**   | **~$9–14**      |

A full 15-sensor suit therefore costs roughly **$135–210** in hardware, compared to thousands for a commercial solution.
