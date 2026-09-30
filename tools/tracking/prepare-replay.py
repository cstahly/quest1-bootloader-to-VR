#!/usr/bin/env python3
"""Prepare a PRIVATE Basalt replay from QVRREC01 and owner calibration.

Experimental timing: map MCU microseconds using minimum observed host arrival
offset. This is a transport-bound approximation, NOT calibrated exposure timing.
Use empirical IMU axes (-Y,-X,-Z); factory offset units/sign remain unverified.
Noise parameters are initial engineering estimates, not measured calibration.
"""
import argparse, csv, io, json, struct
from pathlib import Path
import numpy as np
from scipy.spatial.transform import Rotation

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('recording'); p.add_argument('camera_calibration')
p.add_argument('imu_calibration'); p.add_argument('output')
a = p.parse_args(); out = Path(a.output); out.mkdir(parents=True, exist_ok=True)
cameras = json.load(open(a.camera_calibration))['CameraCalibration']
imu_cal = json.load(open(a.imu_calibration))['ImuCalibration']
cam_records = []; imu_rows = []
with open(a.recording, 'rb') as f:
    assert f.read(8) == b'QVRREC01'
    while h := f.read(5):
        kind, size = struct.unpack('<cI', h); b = f.read(size); assert len(b) == size
        if kind == b'C': cam_records.append(b)
        elif kind == b'I': imu_rows = list(csv.DictReader(io.StringIO(b.decode())))
assert cam_records and imu_rows
offset = min(int(r['host_monotonic_ns']) - int(r['device_timestamp'])*1000 for r in imu_rows)
events = []
for r in imu_rows:
    t = int(r['device_timestamp'])*1000 + offset
    ax,ay,az,gx,gy,gz = (float(r[k]) for k in ('ax_g','ay_g','az_g','gx_deg_s','gy_deg_s','gz_deg_s'))
    vals = [-ay*9.80665,-ax*9.80665,-az*9.80665,-gy*np.pi/180,-gx*np.pi/180,-gz*np.pi/180]
    events.append((t, b'I', struct.pack('<6f', *vals)))
skipped = 0
for b in cam_records:
    times = struct.unpack_from('<4Q', b, 32)
    # VIT requires identical per-camera timestamps. Only group near-synchronous
    # captures, retain originals in the recording, and report this approximation.
    if max(times)-min(times) > 1_000_000:
        skipped += 1; continue
    events.append((sum(times)//4, b'C', b[136:]))
with open(out/'events.bin', 'wb') as f:
    for t,kind,data in sorted(events, key=lambda v:v[0]):
        f.write(struct.pack('<cQI', kind,t,len(data))); f.write(data)
cal = dict(T_imu_cam=[],intrinsics=[],resolution=[],calib_accel_bias=[0.]*9,
           calib_gyro_bias=[0.]*12,imu_update_rate=1e6/985,
           accel_noise_std=[.03]*3,gyro_noise_std=[.003]*3,
           accel_bias_std=[.001]*3,gyro_bias_std=[.0001]*3,
           cam_time_offset_ns=0,vignette=[])
imu_from_device = np.linalg.inv(np.array(imu_cal['DeviceFromImu']).reshape(4,4))
for c in sorted(cameras,key=lambda c:int(c['Id'])):
    assert c['Projection']['Model']=='PinholeSymmetric' and c['Distortion']['Model']=='Fisheye62'
    t = imu_from_device @ np.array(c['DeviceFromCamera']).reshape(4,4)
    quat = Rotation.from_matrix(t[:3,:3]).as_quat()
    cal['T_imu_cam'].append(dict(zip(('px','py','pz','qx','qy','qz','qw'),[*t[:3,3],*quat])))
    focal,cx,cy = c['Projection']['Coefficients']
    keys=['fx','fy','cx','cy']+[f'k{i}' for i in range(1,7)]+['p1','p2','s1','s2','s3','s4']
    vals=[focal/2,focal/2,cx/2,cy/2]+c['Distortion']['Coefficients']+[0.]*4
    cal['intrinsics'].append(dict(camera_type='fisheye624',intrinsics=dict(zip(keys,vals))))
    cal['resolution'].append([320,240])
(out/'calibration.json').write_text(json.dumps({'value0':cal},indent=2))
report = dict(camera_publications=len(cam_records),imu_samples=len(imu_rows),
              unsynchronized_groups_skipped=skipped,estimated_host_minus_mcu_ns=offset,
              caveats=__doc__)
(out/'preparation.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
