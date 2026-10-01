#!/usr/bin/env python3
"""Validate private QRAW001 snapshots and optionally extract owner-only PGM files."""
import argparse
import json
import os
from pathlib import Path
import struct

HEADER, FRAME, SIZE = 256, 307840, 256 + 4*307840

def parse(data):
    if len(data) != SIZE or data[:8] != b'QRAW001\0':
        raise ValueError('invalid capture size/magic')
    dimensions = struct.unpack_from('<7I', data, 8)
    if dimensions != (256, 4, 640, 480, 640, 640, FRAME):
        raise ValueError('unsupported dimensions/layout')
    flags, captured, requested = struct.unpack_from('<IQQ', data, 36)
    if flags & ~7 or not requested or captured < requested or any(data[56:64]):
        raise ValueError('invalid flags/capture/request clock/reserved header')
    cameras = []
    for c in range(4):
        record = struct.unpack_from('<IIQIIQIIII', data, 64+c*48)
        index, sequence, timestamp, exposure, gain, offset, size, request_us, request_gain, reserved = record
        if index != c or not sequence or timestamp <= requested or timestamp > captured:
            raise ValueError('invalid camera identity/sequence/timestamp')
        if offset != HEADER+c*FRAME or size != FRAME or reserved:
            raise ValueError('invalid camera payload layout/reserved')
        metadata = data[offset:offset+640]
        if exposure < 1000 or exposure != ((metadata[6]<<8)+metadata[7])*19 or gain != metadata[3]:
            raise ValueError('scene classification or applied metadata mismatch')
        cameras.append(dict(camera=c, sequence=sequence, timestamp_ns=timestamp,
                            exposure_us=exposure, gain_q4=gain, requested_us=request_us,
                            requested_gain_q4=request_gain, offset=offset))
    times = [c['timestamp_ns'] for c in cameras]
    if max(times)-min(times) > 1_000_000:
        raise ValueError('unsynchronized cohort')
    return dict(version=1, width=640, height=480, captured_ns=captured,
                requested_ns=requested, auto_exposure=bool(flags&1),
                startup_exposure_verified=bool(flags&2), feed_denoise=bool(flags&4),
                skew_ns=max(times)-min(times), cameras=cameras)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('--extract', type=Path, help='new private output directory (must not exist)')
    args = parser.parse_args()
    data = args.capture.read_bytes()
    result = parse(data)
    if args.extract:
        args.extract.mkdir(mode=0o700)
        for camera in result['cameras']:
            path = args.extract/f"camera{camera['camera']}.pgm"
            fd = os.open(path, os.O_CREAT|os.O_EXCL|os.O_WRONLY, 0o600)
            with os.fdopen(fd, 'wb') as out:
                offset = camera['offset']+640
                out.write(b'P5\n640 480\n255\n'+data[offset:offset+640*480])
    print(json.dumps(result, indent=2))

if __name__ == '__main__':
    main()
