#!/usr/bin/env python3
"""Offline IMU propagation versus future VIO consistency; NEVER ground truth.

Inputs must be already prepared Basalt-body SI IMU events and VIT T_w_i poses.
IMU integration is causal (left hold, no sample later than the target). Future VIO
position/attitude interpolation is used only as the scoring reference. Optimized
per-pose biases are absent from VIT CSV, so default zero bias is an assumption.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import struct

import numpy as np

GRAVITY = np.array([0., 0., -9.81])


def exp_so3(vector):
    v = np.asarray(vector, dtype=float)
    theta = np.linalg.norm(v)
    skew = np.array([[0., -v[2], v[1]], [v[2], 0., -v[0]], [-v[1], v[0], 0.]])
    if theta < 1e-7:
        a = 1 - theta * theta / 6
        b = .5 - theta * theta / 24
    else:
        a = np.sin(theta) / theta
        b = (1 - np.cos(theta)) / (theta * theta)
    return np.eye(3) + a * skew + b * skew @ skew


def rotation_from_quaternion(q):
    q = np.asarray(q, dtype=float)
    norm = np.linalg.norm(q)
    if not np.all(np.isfinite(q)) or norm < 1e-12:
        raise ValueError('invalid quaternion')
    x, y, z, w = q / norm
    return np.array([[1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w)],
                     [2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w)],
                     [2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y)]])


def slerp(a, b, fraction):
    a, b = a / np.linalg.norm(a), b / np.linalg.norm(b)
    dot = float(a @ b)
    if dot < 0:
        b, dot = -b, -dot
    dot = np.clip(dot, -1, 1)
    if dot > .9995:
        q = a + fraction * (b-a)
        return q / np.linalg.norm(q)
    angle = np.arccos(dot)
    return (np.sin((1-fraction)*angle)*a + np.sin(fraction*angle)*b) / np.sin(angle)


def propagate(start_ns, target_ns, position, velocity, rotation, times, samples,
              accel_bias=None, gyro_bias=None, max_gap_ns=10000000):
    """Integrate body specific force and angular rate, retaining latest sample.

    p/v are world-frame; rotation maps IMU body to world. No head lever arm.
    A delayed VIO seed could be propagated to now using IMU buffered through now;
    this does not predict unknown future acceleration beyond received IMU.
    """
    if target_ns < start_ns:
        raise ValueError('negative horizon')
    p, v, r = np.array(position, dtype=float), np.array(velocity, dtype=float), np.array(rotation, dtype=float)
    ba = np.zeros(3) if accel_bias is None else np.asarray(accel_bias, dtype=float)
    bg = np.zeros(3) if gyro_bias is None else np.asarray(gyro_bias, dtype=float)
    if not all(np.all(np.isfinite(x)) for x in (p, v, r, ba, bg)):
        raise ValueError('nonfinite state')
    i = int(np.searchsorted(times, start_ns, side='right')) - 1
    if i < 0 or start_ns-times[i] > max_gap_ns:
        raise ValueError('no recent seed IMU')
    cursor = int(start_ns)
    while cursor < target_ns:
        endpoint = int(min(target_ns, times[i+1])) if i+1 < len(times) else int(target_ns)
        # Last-held measurement may not bridge an unbounded sensor gap.
        if endpoint-times[i] > max_gap_ns:
            raise ValueError('IMU gap')
        dt = (endpoint-cursor)*1e-9
        sample = samples[i]
        if not np.all(np.isfinite(sample)):
            raise ValueError('nonfinite IMU')
        force, omega = sample[:3]-ba, sample[3:]-bg
        half = r @ exp_so3(.5*dt*omega)
        acceleration = half @ force + GRAVITY
        p += v*dt + .5*acceleration*dt*dt
        v += acceleration*dt
        r = r @ exp_so3(dt*omega)
        cursor = endpoint
        if i+1 < len(times) and cursor == times[i+1]:
            i += 1
    return p, v, r


def load_imu(path):
    times, samples = [], []
    with Path(path).open('rb') as f:
        while header := f.read(13):
            if len(header) != 13:
                raise ValueError('truncated event header')
            kind, timestamp, size = struct.unpack('<cQI', header)
            if kind == b'I' and size == 24:
                data = f.read(size)
                if len(data) != size:
                    raise ValueError('truncated IMU')
                times.append(timestamp)
                samples.append(struct.unpack('<6f', data))
            elif kind == b'C' and size in (153600, 307200):
                f.seek(size, 1)
            else:
                raise ValueError('unknown event shape')
    times = np.array(times, dtype=np.int64)
    samples = np.array(samples, dtype=float)
    if len(times) < 2 or np.any(np.diff(times) <= 0) or not np.all(np.isfinite(samples)):
        raise ValueError('invalid IMU stream')
    return times, samples


def load_poses(path):
    with Path(path).open() as f:
        rows = list(csv.DictReader(f))
    times = np.array([int(r['timestamp_ns']) for r in rows], dtype=np.int64)
    values = np.array([[float(r[k]) for k in ('x','y','z','vx','vy','vz','qx','qy','qz','qw')] for r in rows])
    if len(times) < 2 or np.any(np.diff(times) <= 0) or not np.all(np.isfinite(values)):
        raise ValueError('invalid pose stream')
    return times, values[:, :3], values[:, 3:6], values[:, 6:]


def reference_at(target, times, positions, quaternions):
    j = int(np.searchsorted(times, target))
    if j == 0 or j == len(times):
        raise ValueError('reference outside interval')
    if times[j]-times[j-1] > 100000000:
        raise ValueError('reference gap')
    f = float(target-times[j-1]) / float(times[j]-times[j-1])
    p = positions[j-1] + f*(positions[j]-positions[j-1])
    q = slerp(quaternions[j-1], quaternions[j], f)
    if not np.all(np.isfinite(p)):
        raise ValueError('invalid reference')
    return p, rotation_from_quaternion(q)


def distribution(values):
    a = np.asarray(values)
    if len(a) == 0:
        return None
    return dict(median=float(np.median(a)), p95=float(np.quantile(a,.95)), maximum=float(np.max(a)))


def angle_error(a, b):
    return float(np.degrees(np.arccos(np.clip((np.trace(a.T@b)-1)*.5, -1, 1))))


def evaluate(pose_path, imu_path, quality_path=None):
    times, positions, velocities, qs = load_poses(pose_path)
    imu_times, samples = load_imu(imu_path)
    ready = None
    if quality_path:
        with Path(quality_path).open() as f:
            rows = list(csv.DictReader(f))
        if [int(r['timestamp_ns']) for r in rows] != list(times):
            raise ValueError('quality timestamps mismatch')
        ready = np.array([int(r['ready']) for r in rows])
    result = {'pose_count':len(times), 'imu_count':len(imu_times), 'gravity_mps2':GRAVITY.tolist(),
              'bias_assumption':'zero optimized accel/gyro bias; calibration fixed transforms assumed identity',
              'reference':'Future SAME-estimator VIO, linearly interpolated position and slerped attitude. NOT ground truth.',
              'horizons':{}}
    for ms in (20,35,65,100):
        sets = {'all_after3s':{}, 'ready_seed_after3s':{}}
        rejected = {}
        for i, timestamp in enumerate(times):
            if timestamp-times[0] < 3000000000:
                continue
            target = int(timestamp)+ms*1000000
            try:
                truth_p, truth_r = reference_at(target,times,positions,qs)
                initial_r = rotation_from_quaternion(qs[i])
                p, v, r = propagate(int(timestamp),target,positions[i],velocities[i],initial_r,imu_times,samples)
            except ValueError as error:
                reason = str(error);rejected[reason] = rejected.get(reason,0)+1
                continue
            cv = positions[i]+velocities[i]*(ms*.001)
            values = {'hold_position_error_m':np.linalg.norm(positions[i]-truth_p),
                      'cv_position_error_m':np.linalg.norm(cv-truth_p),
                      'imu_position_error_m':np.linalg.norm(p-truth_p),
                      'hold_attitude_error_deg':angle_error(initial_r,truth_r),
                      'imu_attitude_error_deg':angle_error(r,truth_r),
                      'imu_cv_difference_m':np.linalg.norm(p-cv)}
            for name in sets:
                if name.startswith('ready') and (ready is None or not ready[i]):
                    continue
                for key,value in values.items():
                    sets[name].setdefault(key,[]).append(float(value))
        summary = {}
        for name, values in sets.items():
            summary[name] = {key:distribution(value) for key,value in values.items()}
            summary[name]['samples'] = len(values.get('imu_position_error_m',[]))
            if values:
                summary[name]['imu_better_than_cv_fraction'] = float(np.mean(np.array(values['imu_position_error_m']) < np.array(values['cv_position_error_m'])))
        result['horizons'][str(ms)] = {'sets':summary,'rejected':rejected,
            'accel_bias_position_bound_for_0_1_mps2_m':.5*.1*(ms*.001)**2,
            'gyro_bias_gravity_position_small_angle_bound_for_0_01_radps_m':9.81*.01*(ms*.001)**3/6}
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--poses',type=Path,required=True)
    parser.add_argument('--imu-events',type=Path,required=True)
    parser.add_argument('--quality',type=Path)
    parser.add_argument('--calibration',type=Path,required=True,help='Prepared Basalt calibration; fixed IMU transforms must be identity.')
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args()
    calibration = json.loads(args.calibration.read_text())['value0']
    for key,length in [('calib_accel_bias',9),('calib_gyro_bias',12)]:
        values = calibration[key]
        if len(values) != length or any(float(v) != 0 for v in values):
            raise ValueError('nonidentity fixed IMU calibration unsupported; do not silently skip it')
    result = evaluate(args.poses,args.imu_events,args.quality)
    result['fixed_imu_calibration_verified_identity'] = True
    result['inputs'] = {str(p):hashlib.sha256(p.read_bytes()).hexdigest()
                        for p in [args.poses,args.imu_events,args.calibration]+([args.quality] if args.quality else [])}
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(f'Consistency analysis saved: {args.output}; future VIO is NOT ground truth.')


if __name__ == '__main__':
    main()
