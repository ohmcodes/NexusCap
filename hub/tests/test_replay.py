"""
Tests for nexuscap_replay.py packet iteration.
"""

import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from nexuscap_hub   import MAGIC, PACKET_FMT, PACKET_SIZE, PROTOCOL_VER
from nexuscap_replay import iter_packets


def make_raw(sensor_id=0, timestamp=0):
    return struct.pack(
        PACKET_FMT,
        MAGIC, PROTOCOL_VER, sensor_id,
        1.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0,
        timestamp,
    )


def test_iter_packets_empty_file():
    with tempfile.NamedTemporaryFile(delete=False, suffix=".bin") as f:
        path = Path(f.name)
    path.write_bytes(b"")
    assert list(iter_packets(path)) == []


def test_iter_packets_single():
    raw = make_raw(sensor_id=1, timestamp=500)
    with tempfile.NamedTemporaryFile(delete=False, suffix=".bin") as f:
        f.write(raw)
        path = Path(f.name)
    packets = list(iter_packets(path))
    assert len(packets) == 1
    data, ts = packets[0]
    assert data == raw
    assert ts == 500


def test_iter_packets_multiple():
    raws = [make_raw(sensor_id=i, timestamp=i * 100) for i in range(5)]
    with tempfile.NamedTemporaryFile(delete=False, suffix=".bin") as f:
        for r in raws:
            f.write(r)
        path = Path(f.name)
    packets = list(iter_packets(path))
    assert len(packets) == 5
    for i, (data, ts) in enumerate(packets):
        assert ts == i * 100


def test_iter_packets_skips_bad_magic():
    good = make_raw(sensor_id=0, timestamp=10)
    bad  = b"\x00" * PACKET_SIZE   # wrong magic
    with tempfile.NamedTemporaryFile(delete=False, suffix=".bin") as f:
        f.write(good)
        f.write(bad)
        f.write(good)
        path = Path(f.name)
    packets = list(iter_packets(path))
    # Only the two good packets should be yielded
    assert len(packets) == 2


def test_iter_packets_ignores_trailing_incomplete():
    raw = make_raw(sensor_id=0, timestamp=42)
    with tempfile.NamedTemporaryFile(delete=False, suffix=".bin") as f:
        f.write(raw)
        f.write(b"\xAB\xCD")  # 2 trailing garbage bytes
        path = Path(f.name)
    packets = list(iter_packets(path))
    assert len(packets) == 1
