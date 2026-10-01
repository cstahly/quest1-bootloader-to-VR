#!/usr/bin/env python3
"""Diagnose translation prediction corrections from recorded VIT output.

Models the r6 100 ms/15 cm cap at fixed latency, without head lever arm, quality
gating, or frame alignment. This is a discontinuity diagnostic, not ground truth
and not an exact reconstruction of a wearer trial.
"""
import argparse
import csv
import json
import math
import statistics


def predicted(position, velocity, seconds):
    speed = math.sqrt(sum(v*v for v in velocity))
    if not math.isfinite(speed) or speed > 3:
        return position
    horizon = min(max(seconds, 0), .1, .15/speed if speed else .1)
    return [p + v*horizon for p, v in zip(position, velocity)]


def summary(values):
    ordered = sorted(values)
    return dict(median=statistics.median(ordered),
                p95=ordered[int(.95*(len(ordered)-1))], maximum=ordered[-1])


def analyze(rows, latency):
    samples = [(int(r['timestamp_ns']), [float(r[k]) for k in ('x', 'y', 'z')],
                [float(r[k]) for k in ('vx', 'vy', 'vz')]) for r in rows]
    if len(samples) < 2 or not all(math.isfinite(v) for _, p, vel in samples for v in p+vel):
        raise ValueError('Need at least two finite pose/velocity samples')
    if not all(a[0] < b[0] for a, b in zip(samples, samples[1:])):
        raise ValueError('Nonmonotonic timestamps')
    raw, corrected, residual = [], [], []
    for (ta, pa, va), (tb, pb, vb) in zip(samples, samples[1:]):
        dt = (tb-ta)*1e-9
        raw.append(math.dist(pa, pb))
        corrected.append(math.dist(predicted(pa, va, dt+latency),
                                   predicted(pb, vb, latency)))
        residual.append(math.dist([p+v*dt for p, v in zip(pa, va)], pb))
    return dict(samples=len(samples), fixed_latency_ms=latency*1000,
                raw_update_step_m=summary(raw), predicted_update_correction_m=summary(corrected),
                previous_velocity_next_pose_residual_m=summary(residual),
                larger_correction_fraction=sum(b>a for a, b in zip(raw, corrected))/len(raw),
                caveat=__doc__.strip())


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('poses')
    parser.add_argument('--latency-ms', type=float, default=65)
    parser.add_argument('--start-seconds', type=float, default=3)
    args = parser.parse_args()
    if not math.isfinite(args.latency_ms) or not 0 <= args.latency_ms <= 150:
        parser.error('latency must be finite and within 0..150 ms')
    if not math.isfinite(args.start_seconds) or args.start_seconds < 0:
        parser.error('start seconds must be finite and nonnegative')
    with open(args.poses) as source:
        data = list(csv.DictReader(source))
    if data:
        start = int(data[0]['timestamp_ns']) + args.start_seconds*1e9
        data = [r for r in data if int(r['timestamp_ns']) >= start]
    print(json.dumps(analyze(data, args.latency_ms/1000), indent=2))
