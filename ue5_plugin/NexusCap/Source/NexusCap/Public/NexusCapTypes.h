// Copyright NexusCap Contributors. All Rights Reserved.
/**
 * NexusCapTypes.h
 *
 * Shared wire-protocol definitions.  These MUST match:
 *   firmware/hub_node/src/main.cpp  (UDPPacket struct)
 *   hub/nexuscap_hub.py             (PACKET_FMT, PACKET_SIZE)
 */

#pragma once

#include "CoreMinimal.h"

// ---------------------------------------------------------------------------
// Protocol constants
// ---------------------------------------------------------------------------

/** Magic bytes that identify a NexusCap UDP datagram. */
static constexpr uint8 NEXUSCAP_MAGIC[4] = { 'N', 'C', 'A', 'P' };

/** Current wire-protocol version. */
static constexpr uint8 NEXUSCAP_PROTOCOL_VERSION = 1;

/** Total size of one UDP datagram in bytes. */
static constexpr int32 NEXUSCAP_PACKET_SIZE = 38;

/** Default UDP port the plugin listens on. */
static constexpr int32 NEXUSCAP_DEFAULT_PORT = 9001;

/** Maximum number of sensor IDs (0 … NEXUSCAP_MAX_SENSORS-1). */
static constexpr int32 NEXUSCAP_MAX_SENSORS = 20;

// ---------------------------------------------------------------------------
// Wire-protocol packet layout (little-endian, packed, 38 bytes total)
//
//   Offset  Size  Field
//   0       4     magic   "NCAP"
//   4       1     version (1)
//   5       1     sensor_id
//   6       4     qw      float32
//   10      4     qx      float32
//   14      4     qy      float32
//   18      4     qz      float32
//   22      4     ax      float32  (g)
//   26      4     ay      float32  (g)
//   30      4     az      float32  (g)
//   34      4     timestamp  uint32 (ms, from sensor node millis())
// ---------------------------------------------------------------------------

#pragma pack(push, 1)
struct FNexusCapWirePacket
{
    uint8  Magic[4];
    uint8  Version;
    uint8  SensorId;
    float  Qw;
    float  Qx;
    float  Qy;
    float  Qz;
    float  Ax;
    float  Ay;
    float  Az;
    uint32 Timestamp;
};
#pragma pack(pop)

static_assert(sizeof(FNexusCapWirePacket) == NEXUSCAP_PACKET_SIZE,
              "FNexusCapWirePacket size mismatch – check struct packing");

// ---------------------------------------------------------------------------
// Decoded frame (host-friendly)
// ---------------------------------------------------------------------------
struct FNexusCapFrame
{
    int32  SensorId    = 0;
    FQuat  Orientation = FQuat::Identity;
    FVector LinearAccel = FVector::ZeroVector;   // g
    uint32 TimestampMs = 0;
};

// ---------------------------------------------------------------------------
// Sensor-ID to UE5 bone-name mapping  (UE5 Mannequin / MetaHuman skeleton)
// Indices correspond to sensor_id values sent by firmware.
// ---------------------------------------------------------------------------
static const TArray<FName> GNexusCapBoneNames =
{
    /* 0  */ FName("pelvis"),
    /* 1  */ FName("spine_01"),
    /* 2  */ FName("spine_02"),
    /* 3  */ FName("spine_03"),
    /* 4  */ FName("neck_01"),
    /* 5  */ FName("head"),
    /* 6  */ FName("clavicle_l"),
    /* 7  */ FName("upperarm_l"),
    /* 8  */ FName("lowerarm_l"),
    /* 9  */ FName("hand_l"),
    /* 10 */ FName("clavicle_r"),
    /* 11 */ FName("upperarm_r"),
    /* 12 */ FName("lowerarm_r"),
    /* 13 */ FName("hand_r"),
    /* 14 */ FName("thigh_l"),
    /* 15 */ FName("calf_l"),
    /* 16 */ FName("foot_l"),
    /* 17 */ FName("thigh_r"),
    /* 18 */ FName("calf_r"),
    /* 19 */ FName("foot_r"),
};

// ---------------------------------------------------------------------------
// Bone parent indices  (matches GNexusCapBoneNames order)
// -1 = root bone
// ---------------------------------------------------------------------------
static const TArray<int32> GNexusCapBoneParents =
{
    /* 0  pelvis     */ -1,
    /* 1  spine_01   */  0,
    /* 2  spine_02   */  1,
    /* 3  spine_03   */  2,
    /* 4  neck_01    */  3,
    /* 5  head       */  4,
    /* 6  clavicle_l */  3,
    /* 7  upperarm_l */  6,
    /* 8  lowerarm_l */  7,
    /* 9  hand_l     */  8,
    /* 10 clavicle_r */  3,
    /* 11 upperarm_r */ 10,
    /* 12 lowerarm_r */ 11,
    /* 13 hand_r     */ 12,
    /* 14 thigh_l    */  0,
    /* 15 calf_l     */ 14,
    /* 16 foot_l     */ 15,
    /* 17 thigh_r    */  0,
    /* 18 calf_r     */ 17,
    /* 19 foot_r     */ 18,
};
