#!/usr/bin/env python3
"""Private lower-stereo rectification lookup for the bounded live VIT probe.

Matches the offline f160 virtual pinhole transform; no device writes.
"""
import argparse,json,runpy,struct
from pathlib import Path
import numpy as np
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('factory_calibration');p.add_argument('output');p.add_argument('--four',action='store_true',help='Use physical order 0,2,1,3 with matching four-camera calibration');a=p.parse_args()
project=runpy.run_path(str(Path(__file__).parent.parent/'camera/preview-calibration.py'))['project']
cameras={int(c['Id']):c for c in json.load(open(a.factory_calibration))['CameraCalibration']}
x,y=np.meshgrid((np.arange(320)-159.5)/160.,(np.arange(240)-119.5)/160.)
rays=np.stack([x,-y,-np.ones_like(x)],axis=-1)
ids=[0,2,1,3] if a.four else [0,2]
with open(a.output,'xb') as out:
 out.write(struct.pack('<5I',0x514d4150,1,320,240,len(ids))+struct.pack('<'+str(len(ids))+'I',*ids))
 for cid in ids:
  c=cameras[cid];t=np.array(c['DeviceFromCamera']).reshape(4,4);u,v,facing=project(rays+t[:3,3],c)
  u=(u/2).astype(np.float32);v=(v/2).astype(np.float32);valid=(facing>0)&(u>=0)&(u<319)&(v>=0)&(v<239)
  # OpenCV INTER_LINEAR's five-bit remap coordinate quantization.
  qx=np.rint(u*32).astype(np.int64);qy=np.rint(v*32).astype(np.int64)
  ix=qx//32;iy=qy//32;valid&=(ix>=0)&(ix<=319)&(iy>=0)&(iy<=239)
  for xx,yy,fx,fy,ok in zip(ix.flat,iy.flat,(qx%32).flat,(qy%32).flat,valid.flat):
   out.write(struct.pack('<IBB2x',int(yy*320+xx) if ok else 0xffffffff,int(fx),int(fy)))
