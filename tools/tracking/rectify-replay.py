#!/usr/bin/env python3
"""Offline Quest replay experiment: parallel virtual pinhole cameras.

Resamples recorded images using factory Fisheye62 calibration. Preserves each
camera's physical centre, timestamps, and the original recordings. This is not
a deployed passthrough transform or accepted tracking calibration.
"""
import argparse,json,runpy,struct
from pathlib import Path
import cv2
import numpy as np

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('source');p.add_argument('factory_calibration');p.add_argument('output')
a=p.parse_args();src=Path(a.source);out=Path(a.output);out.mkdir(exist_ok=True,parents=True)
project=runpy.run_path(str(Path(__file__).parent.parent/'camera/preview-calibration.py'))['project']
cameras=sorted(json.load(open(a.factory_calibration))['CameraCalibration'],key=lambda c:int(c['Id']))
cal=json.load(open(src/'calibration.json'))
focal=160.;cx=159.5;cy=119.5
x,y=np.meshgrid((np.arange(320)-cx)/focal,(np.arange(240)-cy)/focal)
# Virtual optical +X right, +Y down, +Z forward in the empirical IMU/head frame.
rays=np.stack([x,-y,-np.ones_like(x)],axis=-1)
maps=[]
for c in cameras:
    t=np.array(c['DeviceFromCamera']).reshape(4,4)
    u,v,facing=project(rays+t[:3,3],c)
    mx,my=(u/2).astype(np.float32),(v/2).astype(np.float32)
    valid=(facing>0)&(mx>=0)&(mx<319)&(my>=0)&(my<239)
    mx[~valid]=-1;my[~valid]=-1;maps.append((mx,my))
    print('camera',c['Id'],'valid fraction',float(np.mean(valid)))
for i in range(4):
    # Factory DeviceFromImu rotation is identity on this Quest. Translation in
    # prepared T_imu_cam already refers to the IMU centre and is retained.
    cal['value0']['T_imu_cam'][i].update(qx=1.,qy=0.,qz=0.,qw=0.)
    cal['value0']['intrinsics'][i]=dict(camera_type='pinhole',intrinsics=dict(fx=focal,fy=focal,cx=cx,cy=cy))
(out/'calibration.json').write_text(json.dumps(cal,indent=2))
with open(src/'events.bin','rb') as inp,open(out/'events.bin','wb') as dest:
    while h:=inp.read(13):
        kind,t,n=struct.unpack('<cQI',h);b=inp.read(n);assert len(b)==n
        if kind==b'C':
            imgs=np.frombuffer(b,np.uint8).reshape(4,240,320)
            b=b''.join(cv2.remap(im,*mapping,cv2.INTER_LINEAR,borderMode=cv2.BORDER_CONSTANT).tobytes() for im,mapping in zip(imgs,maps))
        dest.write(h);dest.write(b)
