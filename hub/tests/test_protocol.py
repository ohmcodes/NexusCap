"""
Tests for NexusCap hub wire-protocol codec.

Run with:
    python -m pytest hub/tests/
"""

import struct
import sys
from pathlib import Path

# Make hub module importable when running from repo root
sys.path.insert(0, str(Path(__file__).parent.parent))

from nexuscap_hub import (
    MAGIC,
    PACKET_FMT,
    PACKET_SIZE,
    PROTOCOL_VER,
    SENSOR_NAMES,
    IMUFrame,
    decode_packet,
    encode_packet,
)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def make_raw_packet(
    magic=MAGIC,
    version=PROTOCOL_VER,
    sensor_id=0,
    qw=1.0, qx=0.0, qy=0.0, qz=0.0,
    ax=0.0, ay=0.0, az=0.0,
    timestamp=1000,
) -> bytes:
    """Construct a raw UDP datagram using the wire format."""
    return struct.pack(
        PACKET_FMT,
        magic, version, sensor_id,
        qw, qx, qy, qz,
        ax, ay, az,
        timestamp,
    )


# ---------------------------------------------------------------------------
# PACKET_SIZE constant
# ---------------------------------------------------------------------------

def test_packet_size_constant():
    raw = make_raw_packet()
    assert len(raw) == PACKET_SIZE == 38


# ---------------------------------------------------------------------------
# decode_packet – happy path
# ---------------------------------------------------------------------------

def test_decode_identity_quaternion():
    raw = make_raw_packet(qw=1.0, qx=0.0, qy=0.0, qz=0.0)
    frame = decode_packet(raw)
    assert frame is not None
    assert frame.sensor_id == 0
    assert abs(frame.qw - 1.0) < 1e-6
    assert abs(frame.qx) < 1e-6
    assert abs(frame.qy) < 1e-6
    assert abs(frame.qz) < 1e-6


def test_decode_sensor_id_range():
    for sid in range(20):
        raw = make_raw_packet(sensor_id=sid)
        frame = decode_packet(raw)
        assert frame is not None
        assert frame.sensor_id == sid


def test_decode_acceleration_values():
    raw = make_raw_packet(ax=0.5, ay=-0.3, az=9.81)
    frame = decode_packet(raw)
    assert frame is not None
    assert abs(frame.ax - 0.5)  < 1e-5
    assert abs(frame.ay - (-0.3)) < 1e-5
    assert abs(frame.az - 9.81) < 1e-5


def test_decode_timestamp():
    raw = make_raw_packet(timestamp=123456)
    frame = decode_packet(raw)
    assert frame is not None
    assert frame.timestamp_ms == 123456


def test_decode_bone_name_property():
    for sid, name in SENSOR_NAMES.items():
        raw = make_raw_packet(sensor_id=sid)
        frame = decode_packet(raw)
        assert frame is not None
        assert frame.bone_name == name


def test_decode_unknown_sensor_id_returns_generic_name():
    raw = make_raw_packet(sensor_id=99)
    frame = decode_packet(raw)
    assert frame is not None
    assert frame.bone_name == "sensor_99"


# ---------------------------------------------------------------------------
# decode_packet – invalid inputs
# ---------------------------------------------------------------------------

def test_decode_wrong_length_returns_none():
    assert decode_packet(b"") is None
    assert decode_packet(b"\x00" * 10) is None
    assert decode_packet(b"\x00" * 37) is None
    assert decode_packet(b"\x00" * 39) is None


def test_decode_bad_magic_returns_none():
    raw = make_raw_packet(magic=b"XXXX")
    assert decode_packet(raw) is None


def test_decode_wrong_version_returns_none():
    raw = make_raw_packet(version=99)
    assert decode_packet(raw) is None


def test_decode_version_zero_returns_none():
    raw = make_raw_packet(version=0)
    assert decode_packet(raw) is None


# ---------------------------------------------------------------------------
# encode_packet – round-trip
# ---------------------------------------------------------------------------

def test_encode_decode_roundtrip():
    original = make_raw_packet(
        sensor_id=5,
        qw=0.707, qx=0.0, qy=0.707, qz=0.0,
        ax=0.1, ay=0.2, az=0.3,
        timestamp=9999,
    )
    frame = decode_packet(original)
    assert frame is not None

    re_encoded = encode_packet(frame)
    assert re_encoded == original


def test_encode_produces_correct_size():
    raw    = make_raw_packet()
    frame  = decode_packet(raw)
    assert frame is not None
    encoded = encode_packet(frame)
    assert len(encoded) == PACKET_SIZE


def test_encode_preserves_magic_and_version():
    raw   = make_raw_packet(sensor_id=3)
    frame = decode_packet(raw)
    assert frame is not None
    encoded = encode_packet(frame)
    assert encoded[:4] == MAGIC
    assert encoded[4] == PROTOCOL_VER


# ---------------------------------------------------------------------------
# IMUFrame dataclass
# ---------------------------------------------------------------------------

def test_imuframe_defaults():
    frame = IMUFrame(sensor_id=0, qw=1, qx=0, qy=0, qz=0,
                     ax=0, ay=0, az=0, timestamp_ms=0)
    assert frame.sensor_id == 0
    assert frame.received_at > 0  # auto-set by field(default_factory=time.monotonic)


def test_imuframe_bone_name_pelvis():
    frame = IMUFrame(sensor_id=0, qw=1, qx=0, qy=0, qz=0,
                     ax=0, ay=0, az=0, timestamp_ms=0)
    assert frame.bone_name == "pelvis"
