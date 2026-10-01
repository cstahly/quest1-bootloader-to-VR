#!/usr/bin/env python3
"""Mask invalid borders in the existing 320x240/f160 rectified replay experiment.

Output rows: camera x y width height. Does not alter images or calibration.
The margin excludes patches adjacent to synthetic black remap borders.
"""
import argparse,json,runpy
from pathlib import Path
import cv2
import numpy as np
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('factory_calibration');p.add_argument('output');p.add_argument('--margin',type=int,default=8)
a=p.parse_args();assert 0<=a.margin<=32
project=runpy.run_path(str(Path(__file__).parent.parent/'camera/preview-calibration.py'))['project']
cameras=sorted(json.load(open(a.factory_calibration))['CameraCalibration'],key=lambda c:int(c['Id']))
x,y=np.meshgrid((np.arange(320)-159.5)/160.,(np.arange(240)-119.5)/160.)
rays=np.stack([x,-y,-np.ones_like(x)],axis=-1)
with open(a.output,'x') as out:
 for i,c in enumerate(cameras):
  t=np.array(c['DeviceFromCamera']).reshape(4,4);u,v,facing=project(rays+t[:3,3],c)
  valid=((facing>0)&(u/2>=0)&(u/2<319)&(v/2>=0)&(v/2<239)).astype(np.uint8)
  valid=cv2.erode(valid,np.ones((2*a.margin+1,2*a.margin+1),np.uint8),borderType=cv2.BORDER_CONSTANT,borderValue=0)
  # One-pixel-high runs represent the exact exclusion mask, no image content tests.
  count=0
  for row in range(240):
   start=None
   for col in range(321):
    invalid=col<320 and valid[row,col]==0
    if invalid and start is None:start=col
    if not invalid and start is not None:
     out.write(f'{i} {start} {row} {col-start} 1\n');count+=1;start=None
  print(f'camera {i}: retained {valid.mean():.3f}, rectangles {count}')
