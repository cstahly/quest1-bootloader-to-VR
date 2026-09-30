#!/usr/bin/env python3
"""Offline ordering diagnostic; never modifies calibration or device state.

SIFT ratio matches are hypotheses, not ground truth. Scores only flag candidates
for further inspection; a high score alone does not validate calibration.
"""
import argparse, itertools, json, struct
import cv2
import numpy as np
from scipy.optimize import root

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('recording');p.add_argument('calibration');a=p.parse_args()
with open(a.recording,'rb') as f:
    assert f.read(8)==b'QVRREC01'
    kind,n=struct.unpack('<cI',f.read(5));assert kind==b'C'
    b=f.read(n)
images=np.frombuffer(b,np.uint8,offset=136).reshape(4,240,320)
cams=sorted(json.load(open(a.calibration))['CameraCalibration'],key=lambda c:int(c['Id']))
sift=cv2.SIFT_create(800,contrastThreshold=.01)
features=[sift.detectAndCompute(im,None) for im in images]
print('SIFT detections:',[len(k) for k,d in features])

def rays(points,c):
    f,cx,cy=c['Projection']['Coefficients'];k=np.array(c['Distortion']['Coefficients'])
    target=(points*2-[cx,cy])/f
    def project(q):
        x,y=q;r=x*x+y*y
        return np.array([x+k[6]*(2*x*x+r)+2*k[7]*x*y,y+k[7]*(2*y*y+r)+2*k[6]*x*y])
    result=[]
    for xy in target:
        sol=root(lambda q:project(q)-xy,xy)
        assert sol.success and np.linalg.norm(project(sol.x)-xy)<1e-7
        rho=np.linalg.norm(sol.x)
        radial=lambda t:t+sum(k[j]*t**(2*j+3) for j in range(6))-rho
        theta=root(radial,rho);assert theta.success
        t=float(theta.x[0]);scale=np.sin(t)/max(rho,1e-12)
        result.append([*list(sol.x*scale),np.cos(t)])
    transform=np.array(c['DeviceFromCamera']).reshape(4,4)
    return np.array(result)@transform[:3,:3].T,transform[:3,3]

pairs=[]
for i,j in itertools.combinations(range(4),2):
    k1,d1=features[i];k2,d2=features[j]
    if d1 is None or d2 is None:continue
    matches=[m for m,n in cv2.BFMatcher().knnMatch(d1,d2,k=2) if m.distance<.7*n.distance]
    if not matches:continue
    pts1=np.array([k1[m.queryIdx].pt for m in matches]);pts2=np.array([k2[m.trainIdx].pt for m in matches])
    pairs.append((i,j,[rays(pts1,c) for c in cams],[rays(pts2,c) for c in cams]))
    print('pair',i,j,'ratio matches',len(matches))
scores=[]
for order in itertools.permutations(range(4)):
    errors=[]
    for i,j,ra,rb in pairs:
        u,pu=ra[order[i]];v,pv=rb[order[j]]
        normals=np.cross(pv-pu,u);normals/=np.maximum(np.linalg.norm(normals,axis=1)[:,None],1e-12)
        errors.extend(np.abs(np.sum(normals*v,axis=1)))
    e=np.array(errors);scores.append((int(np.sum(e<.01)),float(np.median(e)),order))
for count,error,order in sorted(scores,key=lambda s:(-s[0],s[1]))[:5]:
    print('camera assignment',order,'epipolar residual <.01:',count,'median:',error)
