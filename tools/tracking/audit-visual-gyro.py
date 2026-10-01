#!/usr/bin/env python3
"""Read-only rotation consistency audit of prepared, rectified Quest replay.

Camera poses come from independent image correspondences, not Basalt poses.
Ranks proper signed axis permutations of the prepared gyro; identity is expected.
Approximate short-interval gyro integration and essential-matrix degeneracy make
this a diagnostic, not an automatic calibration procedure. Writes no device data.
"""
import argparse,itertools,json,struct
import cv2
import numpy as np
from scipy.spatial.transform import Rotation
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('events');p.add_argument('--camera',type=int,default=0,choices=range(4))
p.add_argument('--stride',type=int,default=6);a=p.parse_args()
assert a.stride>0
frames=[];imu=[];count=0
with open(a.events,'rb') as f:
 while h:=f.read(13):
  assert len(h)==13
  kind,t,n=struct.unpack('<cQI',h);b=f.read(n);assert len(b)==n
  if kind==b'I':imu.append((t,*struct.unpack('<6f',b)[3:]))
  elif kind==b'C':
   assert n==307200
   if count%a.stride==0:frames.append((t,np.frombuffer(b,np.uint8).reshape(4,240,320)[a.camera].copy()))
   count+=1
assert len(imu)>1 and len(frames)>1
imu=np.array(imu);sift=cv2.SIFT_create(nfeatures=800);matcher=cv2.BFMatcher()
k=np.array([[160.,0,159.5],[0,160.,119.5],[0,0,1]])
d=np.diag([1.,-1.,-1.]);samples=[];previous=None
for t,im in frames:
 keys,desc=sift.detectAndCompute(im,None)
 current=(t,keys,desc)
 if previous is not None and desc is not None and previous[2] is not None:
  pt,pk,pd=previous
  matches=[m for pair in matcher.knnMatch(pd,desc,k=2) if len(pair)==2 for m,n in [pair] if m.distance<.7*n.distance]
  if len(matches)>=30:
   x=np.float32([pk[m.queryIdx].pt for m in matches]);y=np.float32([keys[m.trainIdx].pt for m in matches])
   e,mask=cv2.findEssentialMat(x,y,k,method=cv2.RANSAC,prob=.999,threshold=.75)
   if e is not None and e.shape==(3,3):
    n,r,_,inliers=cv2.recoverPose(e,x,y,k,mask=mask)
    rot=Rotation.from_matrix(d@r.T@d).as_rotvec()
    if n>=20 and .03<np.linalg.norm(rot)<.7:samples.append((pt,t,rot,n))
 previous=current
candidates=[]
for perm in itertools.permutations(range(3)):
 for signs in itertools.product([-1,1],repeat=3):
  m=np.eye(3)[list(perm)]*np.array(signs)[:,None]
  if np.linalg.det(m)<.5:continue
  for delay_ms in [-60,-30,0,30,60]:
   errors=[]
   for start,end,visual,n in samples:
    selected=imu[(imu[:,0]>=start+delay_ms*1e6)&(imu[:,0]<=end+delay_ms*1e6)]
    if len(selected)<10:continue
    gyro=np.trapz(selected[:,1:],selected[:,0]*1e-9,axis=0)
    errors.append(float(np.linalg.norm(m@gyro-visual)*180/np.pi))
   if errors:candidates.append(dict(axes=perm,signs=signs,delay_ms=delay_ms,median_error_deg=float(np.median(errors)),p90_error_deg=float(np.quantile(errors,.9))))
candidates.sort(key=lambda x:x['median_error_deg'])
identity=[x for x in candidates if tuple(x['axes'])==(0,1,2) and tuple(x['signs'])==(1,1,1)]
print(json.dumps(dict(usable_rotation_pairs=len(samples),sample_angles_deg=[round(float(np.linalg.norm(s[2])*180/np.pi),2) for s in samples],best=candidates[:8],identity=identity),indent=2))
