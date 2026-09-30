#!/usr/bin/env python3
"""Build a mono combined-camera image for the floor monitor, not immersive stereo.
Factory calibration is read-only. Hard seams and unknown scene depth remain explicit.
"""
import json, runpy, struct, sys
from pathlib import Path
import numpy as np
project=runpy.run_path(str(Path(__file__).with_name('preview-calibration.py')))['project']
cameras=json.load(open(sys.argv[1]))['CameraCalibration']
focal=160/np.tan(np.deg2rad(50))
x,y=np.meshgrid((np.arange(320)+.5-160)/focal,(120-np.arange(240)-.5)/focal)
points=np.stack([x*3,y*3,np.full_like(x,-3)],axis=-1)
best=np.zeros_like(x);mapping=np.full(x.shape,0xffffffff,dtype='<u4')
for camera in cameras:
    u,v,facing=project(points,camera)
    valid=(facing>best)&(facing>.05)&(u>=2)&(u<638)&(v>=2)&(v<478)
    sx,sy=np.clip((u/2).astype(int),0,319),np.clip((v/2).astype(int),0,239)
    mapping[valid]=int(camera['Id'])*320*240+sy[valid]*320+sx[valid]
    best[valid]=facing[valid]
assert np.all((mapping==0xffffffff)|(mapping<4*320*240))
with open(sys.argv[2],'wb') as output:
    output.write(struct.pack('<4I',0x51464c52,1,320,240));output.write(mapping.tobytes())
print('Floor composite map valid; coverage',float(np.mean(mapping!=0xffffffff)))
