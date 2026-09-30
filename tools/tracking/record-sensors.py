#!/usr/bin/env python3
"""Record existing camera feed and a passive IMU reader; never configure sensors.

Stdout: QVRREC01, then <char type, uint32 LE length, payload> records.
C payload: uint64 host read time, followed by the unchanged camera-feed ABI.
I payload: CSV from capture-imu (original device ticks and host arrival times).
Run on the headset and redirect stdout on the host. No room images persist on
the headset. Recording does not imply clocks have been synchronized.
"""
import argparse
import os
import struct
import subprocess
import sys
import tempfile
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--seconds', type=int, default=15, choices=range(1, 61))
parser.add_argument('--feed', default='/run/quest-camera-root/tmp/camera-feed')
parser.add_argument('--imu', default='/run/capture-imu')
args = parser.parse_args()
out = sys.stdout.buffer

def record(kind, payload):
    out.write(struct.pack('<cI', kind, len(payload)))
    out.write(payload)

with tempfile.TemporaryFile(dir='/run') as imu_csv:
    imu = subprocess.Popen([args.imu, str(args.seconds + 2)], stdout=imu_csv)
    count = 0
    try:
        out.write(b'QVRREC01')
        last = None
        deadline = time.monotonic() + args.seconds
        while time.monotonic() < deadline:
            with open(args.feed, 'rb') as source:
                feed = source.read()
            arrival = time.monotonic_ns()
            if len(feed) != 307328 or struct.unpack_from('<5I', feed) != (0x5143414d, 1, 320, 240, 4):
                raise RuntimeError('Invalid camera feed ABI')
            publication = struct.unpack_from('<I', feed, 20)[0]
            published = struct.unpack_from('<Q', feed, 120)[0]
            if 0 <= arrival - published < 500_000_000 and publication != last:
                record(b'C', struct.pack('<Q', arrival) + feed)
                last = publication
                count += 1
            time.sleep(.005)
        if imu.wait(timeout=10) != 0:
            raise RuntimeError('IMU capture failed')
        imu_csv.seek(0)
        record(b'I', imu_csv.read())
        out.flush()
        print(f'Recorded {count} camera publications', file=sys.stderr)
        if not count:
            raise RuntimeError('No fresh cameras')
    finally:
        if imu.poll() is None:
            imu.terminate()
            imu.wait(timeout=5)
