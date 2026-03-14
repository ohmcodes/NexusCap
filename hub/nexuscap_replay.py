#!/usr/bin/env python3
"""
NexusCap Replay Tool
====================
Reads a binary file recorded by nexuscap_hub.py --record
and replays it to a UDP destination at the original capture rate.

Usage:
    python nexuscap_replay.py <recording.bin> [options]

Options:
    --dest-ip   IP    Destination IP    (default: 127.0.0.1)
    --dest-port PORT  Destination port  (default: 9001)
    --speed     N     Playback speed multiplier (default: 1.0)
    --loop            Loop the recording indefinitely
"""

import argparse
import socket
import struct
import sys
import time
from pathlib import Path

PACKET_SIZE = 38
PACKET_FMT  = "<4sBBfffffffI"
MAGIC       = b"NCAP"


def iter_packets(path: Path):
    """Yield (raw_bytes, timestamp_ms) tuples from a recording file."""
    data = path.read_bytes()
    offset = 0
    while offset + PACKET_SIZE <= len(data):
        raw = data[offset: offset + PACKET_SIZE]
        offset += PACKET_SIZE
        fields = struct.unpack(PACKET_FMT, raw)
        ts_ms = fields[-1]  # last field is timestamp
        if fields[0] == MAGIC:
            yield raw, ts_ms


def replay(path: Path, dest_ip: str, dest_port: int, speed: float, loop: bool):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    dest = (dest_ip, dest_port)

    runs = 0
    while True:
        packets = list(iter_packets(path))
        if not packets:
            print("No valid packets in recording", file=sys.stderr)
            return

        runs += 1
        print(f"Replaying {len(packets)} packets (run #{runs}) → {dest_ip}:{dest_port}")

        first_ts    = packets[0][1]
        wall_start  = time.monotonic()

        for raw, ts_ms in packets:
            # Compute when this packet should be sent
            elapsed_target = (ts_ms - first_ts) / 1000.0 / speed
            wall_elapsed   = time.monotonic() - wall_start
            sleep_dur      = elapsed_target - wall_elapsed
            if sleep_dur > 0:
                time.sleep(sleep_dur)
            sock.sendto(raw, dest)

        if not loop:
            break

    sock.close()
    print("Replay complete")


def main():
    parser = argparse.ArgumentParser(
        description="NexusCap Replay – replays a recorded session",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("file", help="Binary recording created by nexuscap_hub.py")
    parser.add_argument("--dest-ip",   default="127.0.0.1")
    parser.add_argument("--dest-port", type=int, default=9001)
    parser.add_argument("--speed",     type=float, default=1.0)
    parser.add_argument("--loop",      action="store_true")
    args = parser.parse_args()

    replay(
        path=Path(args.file),
        dest_ip=args.dest_ip,
        dest_port=args.dest_port,
        speed=args.speed,
        loop=args.loop,
    )


if __name__ == "__main__":
    main()
