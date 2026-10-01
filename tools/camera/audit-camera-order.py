#!/usr/bin/env python3
"""Read-only camera-ID audit using raw recordings and factory epipolar geometry.
Ranks all 24 channel/calibration assignments. Does not modify a map or calibration.
Room imagery, calibration and result JSON must remain private.
"""
import argparse,itertools,json,struct
from pathlib import Path
import cv2
import numpy as np
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('recording');p.add_argument('calibration');p.add_argument('output');a=p.parse_args()
cams={int(c['Id']):c for c in json.loads(Path(a.calibration).read_text())['CameraCalibration']}
assert set(cams)==set(range(4))

def bearings(pixels,camera):
    f,cx,cy=camera['Projection']['Coefficients']
    assert camera['Distortion']['Model']=='Fisheye62'
    k=np.asarray(camera['Distortion']['Coefficients']);p0,p1=k[6:]
    d=(pixels*2-np.array([cx,cy]))/f;x=d.copy()
    # Invert tangential distortion with a vectorized Newton solve.
    for _ in range(12):
        xx,yy=x.T;r2=xx*xx+yy*yy
        err=x+np.stack([p0*(r2+2*xx*xx)+2*p1*xx*yy,p1*(r2+2*yy*yy)+2*p0*xx*yy],axis=1)-d
        j00=1+6*p0*xx+2*p1*yy;j01=2*p0*yy+2*p1*xx;j11=1+6*p1*yy+2*p0*xx
        det=j00*j11-j01*j01
        x-=np.stack([(j11*err[:,0]-j01*err[:,1])/det,(-j01*err[:,0]+j00*err[:,1])/det],axis=1)
    radius=np.linalg.norm(x,axis=1);theta=np.minimum(radius,1.6)
    for _ in range(40):
        val=theta.copy();derivative=np.ones_like(theta)
        for order,coef in enumerate(k[:6]):
            power=2*order+3;val+=coef*theta**power;derivative+=power*coef*theta**(power-1)
        theta-=np.clip((val-radius)/derivative,-.1,.1)
    residual=theta.copy()
    for order,coef in enumerate(k[:6]):residual+=coef*theta**(2*order+3)
    valid=np.isfinite(theta)&(theta>=0)&(theta<np.pi)&(abs(residual-radius)<1e-6)
    ray=np.column_stack([x*np.sin(theta)[:,None]/np.maximum(radius[:,None],1e-15),np.cos(theta)])
    t=np.array(camera['DeviceFromCamera']).reshape(4,4)
    return ray@t[:3,:3].T,valid,t[:3,3]

frames=[];targets=iter([0,20,40,55]);target=next(targets);first=None
with open(a.recording,'rb') as f:
    assert f.read(8)==b'QVRREC01'
    while h:=f.read(5):
        kind,n=struct.unpack('<cI',h);b=f.read(n);assert len(b)==n
        if kind!=b'C':continue
        t=struct.unpack_from('<Q',b,32)[0]
        if first is None:first=t
        if (t-first)/1e9>=target:
            assert len(b)==307336
            frames.append(((t-first)/1e9,np.frombuffer(b,np.uint8,offset=136).reshape(4,240,320).copy()))
            target=next(targets,None)
            if target is None:break
sift=cv2.SIFT_create(nfeatures=1500);matcher=cv2.BFMatcher()
pairs=[];counts=[]
for time,images in frames:
    data=[sift.detectAndCompute(im,None) for im in images]
    for i,j in itertools.combinations(range(4),2):
        ka,da=data[i];kb,db=data[j]
        if da is None or db is None or len(da)<2 or len(db)<2:continue
        forward=matcher.knnMatch(da,db,k=2);back=matcher.knnMatch(db,da,k=2)
        reverse={m.queryIdx:m.trainIdx for m,n in back if m.distance<.72*n.distance}
        matches=[m for m,n in forward if m.distance<.72*n.distance and reverse.get(m.trainIdx)==m.queryIdx]
        counts.append(dict(time=time,channels=[i,j],matches=len(matches)))
        if len(matches)<4:continue
        pa=np.array([ka[m.queryIdx].pt for m in matches]);pb=np.array([kb[m.trainIdx].pt for m in matches])
        pairs.append((i,j,[bearings(pa,cams[c]) for c in range(4)],[bearings(pb,cams[c]) for c in range(4)]))
results=[]
for order in itertools.permutations(range(4)):
    errors=[]
    for i,j,aa,bb in pairs:
        ra,va,ta=aa[order[i]];rb,vb,tb=bb[order[j]]
        normal=np.cross(tb-ta,ra);den=np.linalg.norm(normal,axis=1)
        valid=va&vb&(den>1e-6)
        err=np.abs(np.sum(normal*rb,axis=1))/np.maximum(den,1e-15)
        errors.extend(err[valid].tolist())
    e=np.array(errors)
    results.append(dict(raw_channel_to_calibration_id=list(order),matches=len(e),inliers_002rad=int(np.sum(e<.02)),median_sine_error=float(np.median(e)) if len(e) else None))
results.sort(key=lambda r:(-r['inliers_002rad'],r['median_sine_error'] or 1))
report=dict(method='mutual SIFT ratio .72; calibrated epipolar plane residual; threshold sin(angle)<.02',pair_counts=counts,rankings=results)
with Path(a.output).open('x') as f:json.dump(report,f,indent=2)
for row in results[:5]:print(row)
