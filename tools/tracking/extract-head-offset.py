#!/usr/bin/env python3
"""Extract PRIVATE device-origin offset from the owner's IMU calibration.

The current replay pipeline assumes factory DeviceFromImu rotation is identity.
Reject other units rather than silently mix frames. Device origin is our current
head-origin convention; this is not a measured optical-centre calibration.
"""
import argparse,json,math
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('imu_calibration');p.add_argument('output');a=p.parse_args()
t=json.loads(Path(a.imu_calibration).read_text())['ImuCalibration']['DeviceFromImu']
if len(t)!=16 or not all(math.isfinite(v) for v in t):raise ValueError('invalid transform')
expected={0:1,1:0,2:0,4:0,5:1,6:0,8:0,9:0,10:1,12:0,13:0,14:0,15:1}
if any(abs(t[i]-v)>1e-8 for i,v in expected.items()):raise ValueError('unsupported DeviceFromImu rotation')
offset=[-t[i] for i in (3,7,11)]
if math.sqrt(sum(v*v for v in offset))>.5:raise ValueError('implausible translation units')
with Path(a.output).open('x') as f:f.write(' '.join(format(v,'.12g') for v in offset)+'\n')
