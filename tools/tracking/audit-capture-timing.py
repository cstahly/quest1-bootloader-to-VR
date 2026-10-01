#!/usr/bin/env python3
"""Read-only QVRREC01 timing audit. Transport bounds are NOT exposure calibration.

Reports first-second versus full-recording clock-offset estimates and a
lower-envelope affine fit. No images, calibration or absolute clock values leave
this report; output is aggregate timing only. Requires NumPy.
"""
import argparse
import csv
import io
import json
import struct
try:
    import numpy as np
except ImportError as error:
    raise SystemExit('Timing audit requires a working NumPy installation for this Python interpreter.') from error
if not all(hasattr(np, name) for name in ('array', 'quantile', 'polyfit', 'int64')):
    raise SystemExit(
        'Timing audit requires a working NumPy installation; the imported numpy is incomplete. '
        'Check the selected Python interpreter and PYTHONPATH. '
        'See docs/basalt-positional-tracking.md#archived-timing-audit for the verified command.')


def percentiles(values):
    return dict(zip(('min', 'median', 'p95', 'max'),
                    map(float, np.quantile(values, [0, .5, .95, 1]))))


def audit(path):
    camera = []
    rows = []
    with open(path, 'rb') as source:
        if source.read(8) != b'QVRREC01':
            raise ValueError('not a QVRREC01 recording')
        while header := source.read(5):
            if len(header) != 5:
                raise ValueError('truncated record header')
            kind, size = struct.unpack('<cI', header)
            payload = source.read(size)
            if len(payload) != size:
                raise ValueError('truncated record')
            if kind == b'I':
                rows.extend(csv.DictReader(io.StringIO(payload.decode())))
            elif kind == b'C':
                if size != 307336:
                    raise ValueError('unexpected camera ABI size')
                if struct.unpack_from('<5I', payload, 8) != (0x5143414d, 1, 320, 240, 4):
                    raise ValueError('invalid camera ABI header')
                times = struct.unpack_from('<4Q', payload, 32)
                arrival = struct.unpack_from('<Q', payload)[0]
                published = struct.unpack_from('<Q', payload, 128)[0]
                exposure = struct.unpack_from('<4I', payload, 80)
                camera.append((sum(times)//4, max(times)-min(times),
                               published-sum(times)//4, arrival-published, *exposure))
            else:
                raise ValueError(f'unknown record type: {kind!r}')
    if len(rows) < 3 or len(camera) < 2:
        raise ValueError('insufficient camera/IMU data')
    tick = np.array([int(row['device_timestamp'])*1000 for row in rows], dtype=np.int64)
    host = np.array([int(row['host_monotonic_ns']) for row in rows], dtype=np.int64)
    if np.any(np.diff(tick) <= 0) or np.any(np.diff(host) < 0):
        raise ValueError('nonmonotonic clock')
    delta = host-tick
    first = delta[host-host[0] <= 1_000_000_000].min()
    x = (tick-tick[0]).astype(float)/1e9
    y = (host-host[0]).astype(float)/1e9
    slope, intercept = np.polyfit(x, y, 1)
    residual = y-(slope*x+intercept)
    low = residual <= np.quantile(residual, .1)
    if np.count_nonzero(low) < 2 or np.unique(x[low]).size < 2:
        raise ValueError('insufficient distinct points for lower-envelope clock fit')
    slope, intercept = np.polyfit(x[low], y[low], 1)
    c = np.array(camera, dtype=np.int64)
    return dict(imu_samples=len(rows), camera_groups=len(camera), duration_s=float(x[-1]),
                imu_interval_ms=percentiles(np.diff(tick)/1e6),
                imu_arrival_above_global_min_ms=percentiles((delta-delta.min())/1e6),
                first_second_offset_minus_global_min_ms=float(first-delta.min())/1e6,
                first_second_mapped_future_samples=int(np.count_nonzero(delta < first)),
                transport_fit_rate_ppm=float((slope-1)*1e6),
                transport_fit_drift_over_recording_ms=float((slope-1)*x[-1]*1000),
                camera_interval_ms=percentiles(np.diff(c[:, 0])/1e6),
                camera_skew_ms=percentiles(c[:, 1]/1e6),
                driver_to_publication_ms=percentiles(c[:, 2]/1e6),
                publication_to_recorder_ms=percentiles(c[:, 3]/1e6),
                exposure_us_per_camera=[percentiles(c[:, 4+i]) for i in range(4)],
                caveat='Host transport clock fit cannot identify exposure epoch or constant IMU transport latency.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('recording')
    print(json.dumps(audit(parser.parse_args().recording), indent=2))
