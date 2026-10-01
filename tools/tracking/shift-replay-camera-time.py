#!/usr/bin/env python3
"""Create a private replay timing hypothesis; preserve every payload and IMU time.

Only C event timestamps change. Sort by new timestamp and original event order.
Output is exclusively created, never overwrites a recording. No device interaction.
"""
import argparse
import hashlib
import json
import struct


def shift(source_path, output_path, offset_ns):
    records = []
    imu_hash = hashlib.sha256()
    last = -1
    counts = {b'I': 0, b'C': 0}
    with open(source_path, 'rb') as source:
        while header := source.read(13):
            if len(header) != 13:
                raise ValueError('truncated event header')
            kind, stamp, size = struct.unpack('<cQI', header)
            if kind not in counts or size != (24 if kind == b'I' else 307200):
                raise ValueError('unsupported event ABI')
            if stamp < last:
                raise ValueError('unsorted input')
            last = stamp
            position = source.tell()
            payload = source.read(size)
            if len(payload) != size:
                raise ValueError('truncated event')
            if kind == b'I':
                imu_hash.update(header+payload)
            new = stamp+(offset_ns if kind == b'C' else 0)
            if not 0 < new < 2**64:
                raise ValueError('timestamp shift out of range')
            records.append((new, len(records), kind, size, position))
            counts[kind] += 1
        if not all(counts.values()):
            raise ValueError('missing camera or IMU events')
        records.sort()
        with open(output_path, 'xb') as out:
            for stamp, _, kind, size, position in records:
                source.seek(position)
                payload = source.read(size)
                if len(payload) != size:
                    raise ValueError('source changed while reading')
                out.write(struct.pack('<cQI', kind, stamp, size)+payload)
    return dict(camera_offset_ns=offset_ns, camera_groups=counts[b'C'], imu_samples=counts[b'I'],
                unchanged_imu_records_sha256=imu_hash.hexdigest(),
                note='All payloads and IMU timestamps unchanged; only camera header timestamps shift.')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('source')
    p.add_argument('output')
    p.add_argument('--offset-ms', type=int, required=True)
    args = p.parse_args()
    print(json.dumps(shift(args.source, args.output, args.offset_ms*1_000_000), indent=2))
