# Sensor ID to Body Part Mapping

Each ESP32 sensor node must be assigned a unique `SENSOR_ID` in `firmware/sensor_node/include/config.h`.

| Sensor ID | Bone Name     | Body Part          |
|-----------|---------------|--------------------|
| 0         | pelvis        | Hips / Pelvis      |
| 1         | spine_01      | Lower Spine        |
| 2         | spine_02      | Mid Spine          |
| 3         | spine_03      | Upper Spine / Chest|
| 4         | neck_01       | Neck               |
| 5         | head          | Head               |
| 6         | clavicle_l    | Left Shoulder      |
| 7         | upperarm_l    | Left Upper Arm     |
| 8         | lowerarm_l    | Left Forearm       |
| 9         | hand_l        | Left Hand          |
| 10        | clavicle_r    | Right Shoulder     |
| 11        | upperarm_r    | Right Upper Arm    |
| 12        | lowerarm_r    | Right Forearm      |
| 13        | hand_r        | Right Hand         |
| 14        | thigh_l       | Left Upper Leg     |
| 15        | calf_l        | Left Shin          |
| 16        | foot_l        | Left Foot          |
| 17        | thigh_r       | Right Upper Leg    |
| 18        | calf_r        | Right Shin         |
| 19        | foot_r        | Right Foot         |

## Minimal Viable Setup (5 Sensors)

For a basic upper-body capture, use:

| Sensor ID | Body Part   |
|-----------|-------------|
| 0         | Hips        |
| 3         | Chest       |
| 5         | Head        |
| 7         | Left Arm    |
| 11        | Right Arm   |

## Bone Names Compatibility

The bone names in this table match the default **UE5 Mannequin** and **MetaHuman** skeleton naming convention.  If you are using a custom skeleton, update `GNexusCapBoneNames` and `GNexusCapBoneParents` in `ue5_plugin/NexusCap/Source/NexusCap/Public/NexusCapTypes.h`.
