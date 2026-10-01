#!/usr/bin/env python3
"""Read-only visual/gyro delay diagnostic; not automatic temporal calibration.

Uses prepared pinhole calibration, ordered SO(3) gyro integration, interpolated
interval endpoints and essential-matrix visual rotations. Positive delay compares
images at t to IMU at t+delay. Translation/degeneracy and gyro bias remain confounds.
Requires NumPy, SciPy and OpenCV. No sensor settings or recordings are modified.
"""
import argparse
import json
import struct
import numpy as np
from scipy.spatial.transform import Rotation


class GyroIntegral:
    def __init__(self, times, omega):
        self.t = np.asarray(times, dtype=float)
        self.w = np.asarray(omega, dtype=float)
        if len(self.t) < 2 or self.w.shape != (len(self.t), 3):
            raise ValueError('insufficient or malformed IMU')
        if not np.all(np.isfinite(self.t)) or not np.all(np.isfinite(self.w)) or np.any(np.diff(self.t) <= 0):
            raise ValueError('invalid IMU clock or values')
        # Body-frame angular velocity: orientation increments multiply on right.
        increments = Rotation.from_rotvec((self.w[:-1]+self.w[1:])*.5*np.diff(self.t)[:, None])
        q = [Rotation.identity()]
        for step in increments:
            q.append(q[-1]*step)
        self.q = Rotation.from_quat(np.array([item.as_quat() for item in q]))

    def at(self, t):
        if t < self.t[0] or t > self.t[-1]:
            raise ValueError('integration interval outside IMU support')
        i = min(int(np.searchsorted(self.t, t, side='right'))-1, len(self.t)-2)
        dt = t-self.t[i]
        fraction = dt/(self.t[i+1]-self.t[i])
        end = self.w[i]+fraction*(self.w[i+1]-self.w[i])
        return self.q[i]*Rotation.from_rotvec((self.w[i]+end)*.5*dt)

    def between(self, start, end):
        if end <= start:
            raise ValueError('nonpositive integration interval')
        return self.at(start).inv()*self.at(end)


def visual_in_imu(r_camera_second_from_first, r_imu_from_camera):
    # recoverPose maps old camera coordinates into new camera coordinates;
    # gyro integration maps new body coordinates into old body coordinates.
    return r_imu_from_camera*Rotation.from_matrix(r_camera_second_from_first).inv()*r_imu_from_camera.inv()


def load_events(path, stride):
    frames = []
    imu = []
    count = 0
    origin = None
    with open(path, 'rb') as source:
        while header := source.read(13):
            if len(header) != 13:
                raise ValueError('truncated event header')
            kind, stamp, size = struct.unpack('<cQI', header)
            data = source.read(size)
            if len(data) != size:
                raise ValueError('truncated event')
            if origin is None:
                origin = stamp
            t = (stamp-origin)*1e-9
            if kind == b'I' and size == 24:
                imu.append((t, *struct.unpack('<6f', data)[3:]))
            elif kind == b'C' and size == 307200:
                if count % stride == 0:
                    frames.append((t, np.frombuffer(data, np.uint8).reshape(4, 240, 320)))
                count += 1
            else:
                raise ValueError('unsupported event ABI')
    data = np.array(imu)
    if data.ndim != 2 or len(frames) < 2:
        raise ValueError('insufficient events')
    return frames, GyroIntegral(data[:, 0], data[:, 1:])


def extract_pairs(frames, calibration):
    import cv2
    cv2.setNumThreads(1)
    cv2.setRNGSeed(2026)
    sift = cv2.SIFT_create(nfeatures=800)
    matcher = cv2.BFMatcher()
    pairs = []
    for cam in range(4):
        model = calibration['intrinsics'][cam]
        if model['camera_type'] != 'pinhole':
            raise ValueError('requires prepared pinhole images/calibration')
        k = model['intrinsics']
        K = np.array([[k['fx'], 0, k['cx']], [0, k['fy'], k['cy']], [0, 0, 1]])
        extrinsic = calibration['T_imu_cam'][cam]
        ric = Rotation.from_quat([extrinsic[key] for key in ('qx', 'qy', 'qz', 'qw')])
        previous = None
        for t, images in frames:
            keys, desc = sift.detectAndCompute(images[cam], None)
            if previous is not None and desc is not None and previous[2] is not None:
                pt, pk, pd = previous
                matches = [m for two in matcher.knnMatch(pd, desc, k=2) if len(two) == 2
                           for m, n in [two] if m.distance < .7*n.distance]
                if len(matches) >= 30:
                    x = np.float32([pk[m.queryIdx].pt for m in matches])
                    y = np.float32([keys[m.trainIdx].pt for m in matches])
                    e, mask = cv2.findEssentialMat(x, y, K, method=cv2.RANSAC, prob=.999, threshold=.75)
                    if e is not None and e.shape == (3, 3):
                        n, r, _, _ = cv2.recoverPose(e, x, y, K, mask=mask)
                        visual = visual_in_imu(r, ric)
                        angle = visual.magnitude()
                        if n >= 20 and .02 < angle < .7:
                            pairs.append(dict(camera=cam, start=pt, end=t, visual=visual,
                                              inliers=n, matches=len(matches)))
            previous = (t, keys, desc)
    return pairs


def scan(pairs, gyro, delays_ms):
    # Same pairs at every delay: edge support must not change the ranking.
    pairs = [p for p in pairs if p['start']+min(delays_ms)*.001 >= gyro.t[0]
             and p['end']+max(delays_ms)*.001 <= gyro.t[-1]]
    errors = np.array([[np.degrees((p['visual'].inv()*gyro.between(
        p['start']+d*.001, p['end']+d*.001)).magnitude()) for d in delays_ms] for p in pairs])
    reports = []
    for cam in (None, 0, 1, 2, 3):
        selected = [i for i, p in enumerate(pairs) if cam is None or p['camera'] == cam]
        report = dict(camera='all' if cam is None else cam, pairs=len(selected))
        if selected:
            values = errors[selected]
            med = np.median(values, axis=0)
            best = int(np.argmin(med))
            # Scatter of individual optima is diagnostic, not confidence interval.
            optima = np.array(delays_ms)[np.argmin(values, axis=1)]
            report.update(best_delay_ms=int(delays_ms[best]), median_error_deg=float(med[best]),
                          individual_best_delay_ms_quantiles=np.quantile(optima, [0, .25, .5, .75, 1]).tolist(),
                          curve=[dict(delay_ms=int(d), median_error_deg=float(m),
                                      p90_error_deg=float(np.quantile(values[:, j], .9)))
                                 for j, (d, m) in enumerate(zip(delays_ms, med))])
        reports.append(report)
    return reports


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('events')
    parser.add_argument('calibration')
    parser.add_argument('--stride', type=int, default=3)
    args = parser.parse_args()
    if args.stride < 1:
        parser.error('stride must be positive')
    calibration = json.load(open(args.calibration))['value0']
    frames, gyro = load_events(args.events, args.stride)
    pairs = extract_pairs(frames, calibration)
    report = dict(stride=args.stride, sampled_frames=len(frames),
                  delay_convention='positive: compare camera t to IMU t+delay',
                  caveat='Essential-matrix degeneracy, bias and correlated pairs/cameras prevent treating curve minima as calibrated offsets.',
                  results=scan(pairs, gyro, list(range(-80, 81, 5))))
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
