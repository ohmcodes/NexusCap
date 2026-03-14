#!/usr/bin/env python3
"""
NexusCap Hub Server
====================
Receives UDP datagrams from the ESP32 hub node, decodes IMU frames,
and relays them to Unreal Engine 5's Live Link plugin (or any other
consumer on the LAN).

Wire protocol (38 bytes per datagram):
  [0:4]   magic "NCAP"
  [4]     protocol version (1)
  [5]     sensor_id (0-19)
  [6:10]  qw  (float32 LE)
  [10:14] qx
  [14:18] qy
  [18:22] qz
  [22:26] ax  (float32 LE, g)
  [26:30] ay
  [30:34] az
  [34:38] timestamp (uint32 LE, ms)

Usage:
    python nexuscap_hub.py [options]

Options:
    --listen-port PORT   UDP port to listen on         (default: 9000)
    --ue5-port    PORT   Port to relay data to UE5     (default: 9001)
    --ue5-ip      IP     IP of machine running UE5     (default: 127.0.0.1)
    --record      FILE   Record raw UDP stream to file (default: off)
    --verbose            Print every received packet
"""

import argparse
import logging
import socket
import struct
import sys
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, Optional

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
MAGIC           = b"NCAP"
PROTOCOL_VER    = 1
PACKET_SIZE     = 38
PACKET_FMT      = "<4sBBfffffffI"   # little-endian: magic(4s) ver(B) id(B) qw qx qy qz ax ay az(7f) ts(I)

# Sensor-ID → human-readable bone name (UE5 Mannequin / MetaHuman naming)
SENSOR_NAMES: Dict[int, str] = {
    0:  "pelvis",
    1:  "spine_01",
    2:  "spine_02",
    3:  "spine_03",
    4:  "neck_01",
    5:  "head",
    6:  "clavicle_l",
    7:  "upperarm_l",
    8:  "lowerarm_l",
    9:  "hand_l",
    10: "clavicle_r",
    11: "upperarm_r",
    12: "lowerarm_r",
    13: "hand_r",
    14: "thigh_l",
    15: "calf_l",
    16: "foot_l",
    17: "thigh_r",
    18: "calf_r",
    19: "foot_r",
}


# ---------------------------------------------------------------------------
# Data structures
# ---------------------------------------------------------------------------
@dataclass
class IMUFrame:
    sensor_id: int
    qw: float
    qx: float
    qy: float
    qz: float
    ax: float
    ay: float
    az: float
    timestamp_ms: int
    received_at: float = field(default_factory=time.monotonic)

    @property
    def bone_name(self) -> str:
        return SENSOR_NAMES.get(self.sensor_id, f"sensor_{self.sensor_id}")


# ---------------------------------------------------------------------------
# Packet codec
# ---------------------------------------------------------------------------
def decode_packet(data: bytes) -> Optional[IMUFrame]:
    """Parse a raw UDP datagram into an IMUFrame, or return None on error."""
    if len(data) != PACKET_SIZE:
        return None

    try:
        magic, version, sensor_id, qw, qx, qy, qz, ax, ay, az, ts = \
            struct.unpack(PACKET_FMT, data)
    except struct.error:
        return None

    if magic != MAGIC or version != PROTOCOL_VER:
        return None

    return IMUFrame(
        sensor_id=sensor_id,
        qw=qw, qx=qx, qy=qy, qz=qz,
        ax=ax, ay=ay, az=az,
        timestamp_ms=ts,
    )


def encode_packet(frame: IMUFrame) -> bytes:
    """Re-encode an IMUFrame back to wire format (for relay)."""
    return struct.pack(
        PACKET_FMT,
        MAGIC,
        PROTOCOL_VER,
        frame.sensor_id,
        frame.qw, frame.qx, frame.qy, frame.qz,
        frame.ax, frame.ay, frame.az,
        frame.timestamp_ms,
    )


# ---------------------------------------------------------------------------
# Statistics tracker
# ---------------------------------------------------------------------------
class Stats:
    def __init__(self) -> None:
        self._lock   = threading.Lock()
        self._counts: Dict[int, int] = {}
        self._start  = time.monotonic()

    def record(self, sensor_id: int) -> None:
        with self._lock:
            self._counts[sensor_id] = self._counts.get(sensor_id, 0) + 1

    def report(self) -> str:
        with self._lock:
            elapsed = time.monotonic() - self._start
            if elapsed < 0.001:
                return "No data yet"
            lines = []
            for sid in sorted(self._counts):
                fps  = self._counts[sid] / elapsed
                name = SENSOR_NAMES.get(sid, f"sensor_{sid}")
                lines.append(f"  [{sid:2d}] {name:<14s}  {fps:6.1f} fps")
            self._counts.clear()
            self._start = time.monotonic()
            return "\n".join(lines) if lines else "  (no sensors active)"


# ---------------------------------------------------------------------------
# Hub Server
# ---------------------------------------------------------------------------
class NexusCapHub:
    def __init__(
        self,
        listen_ip: str,
        listen_port: int,
        ue5_ip: str,
        ue5_port: int,
        record_path: Optional[Path],
        verbose: bool,
    ) -> None:
        self.listen_ip    = listen_ip
        self.listen_port  = listen_port
        self.ue5_ip       = ue5_ip
        self.ue5_port     = ue5_port
        self.record_path  = record_path
        self.verbose      = verbose
        self.stats        = Stats()

        self._running     = threading.Event()
        self._recv_sock:  Optional[socket.socket] = None
        self._send_sock:  Optional[socket.socket] = None
        self._record_fh   = None

    # ------------------------------------------------------------------
    def start(self) -> None:
        """Open sockets, optionally open record file, then block."""
        self._recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._recv_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        # Bind to the configured interface so that callers can restrict
        # reception to a specific NIC (e.g. "192.168.1.5") instead of
        # accepting from all interfaces.  Defaults to "0.0.0.0".
        self._recv_sock.bind((self.listen_ip, self.listen_port))
        self._recv_sock.settimeout(1.0)

        self._send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        if self.record_path:
            self._record_fh = open(self.record_path, "wb")
            logging.info("Recording to %s", self.record_path)

        self._running.set()
        logging.info(
            "NexusCap Hub listening on %s:%d  →  relay to %s:%d",
            self.listen_ip, self.listen_port, self.ue5_ip, self.ue5_port,
        )

        # Stats printer thread
        stats_thread = threading.Thread(
            target=self._stats_loop, daemon=True, name="stats"
        )
        stats_thread.start()

        # Main receive loop (blocking)
        try:
            self._receive_loop()
        finally:
            self._cleanup()

    def stop(self) -> None:
        self._running.clear()

    # ------------------------------------------------------------------
    def _receive_loop(self) -> None:
        assert self._recv_sock is not None
        assert self._send_sock is not None

        while self._running.is_set():
            try:
                data, addr = self._recv_sock.recvfrom(512)
            except socket.timeout:
                continue
            except OSError:
                break

            frame = decode_packet(data)
            if frame is None:
                logging.debug("Bad packet from %s (len=%d)", addr, len(data))
                continue

            # Record raw bytes
            if self._record_fh is not None:
                self._record_fh.write(data)

            # Relay to UE5 (same wire format)
            self._send_sock.sendto(data, (self.ue5_ip, self.ue5_port))

            self.stats.record(frame.sensor_id)

            if self.verbose:
                logging.info(
                    "[%s] q=(%.3f %.3f %.3f %.3f)  a=(%.3f %.3f %.3f)  t=%d",
                    frame.bone_name,
                    frame.qw, frame.qx, frame.qy, frame.qz,
                    frame.ax, frame.ay, frame.az,
                    frame.timestamp_ms,
                )

    def _stats_loop(self) -> None:
        while self._running.is_set():
            time.sleep(5.0)
            report = self.stats.report()
            logging.info("--- Sensor FPS (last 5 s) ---\n%s", report)

    def _cleanup(self) -> None:
        if self._recv_sock:
            self._recv_sock.close()
        if self._send_sock:
            self._send_sock.close()
        if self._record_fh:
            self._record_fh.close()
            logging.info("Recording closed: %s", self.record_path)


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------
def main() -> None:
    parser = argparse.ArgumentParser(
        description="NexusCap Hub Server – bridges ESP32 hub to UE5 Live Link",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--listen-ip", default="0.0.0.0",
        help="Local IP to bind to (0.0.0.0 = all interfaces, or a specific NIC IP)",
    )
    parser.add_argument(
        "--listen-port", type=int, default=9000,
        help="UDP port to receive from ESP32 hub",
    )
    parser.add_argument(
        "--ue5-ip", default="127.0.0.1",
        help="IP address of the machine running UE5",
    )
    parser.add_argument(
        "--ue5-port", type=int, default=9001,
        help="UDP port the UE5 NexusCap plugin listens on",
    )
    parser.add_argument(
        "--record", metavar="FILE", default=None,
        help="Write raw UDP stream to a binary file for later replay",
    )
    parser.add_argument(
        "--verbose", action="store_true",
        help="Print every received packet to the console",
    )
    args = parser.parse_args()

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s [%(levelname)s] %(message)s",
        datefmt="%H:%M:%S",
    )

    hub = NexusCapHub(
        listen_ip=args.listen_ip,
        listen_port=args.listen_port,
        ue5_ip=args.ue5_ip,
        ue5_port=args.ue5_port,
        record_path=Path(args.record) if args.record else None,
        verbose=args.verbose,
    )

    try:
        hub.start()
    except KeyboardInterrupt:
        logging.info("Shutting down ...")
        hub.stop()


if __name__ == "__main__":
    main()
