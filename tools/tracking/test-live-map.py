#!/usr/bin/env python3
"""Compare live fixed-point lookup against independently saved OpenCV rectification."""
import argparse,struct
import numpy as np
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('map');p.add_argument('raw_events');p.add_argument('rectified_events');a=p.parse_args()
def first_camera(path):
 with open(path,'rb') as f:
  while h:=f.read(13):
   kind,t,n=struct.unpack('<cQI',h);data=f.read(n);assert len(data)==n
   if kind==b'C':return t,np.frombuffer(data,np.uint8).reshape(4,240,320)
 raise AssertionError('no camera events')
t0,source=first_camera(a.raw_events);t1,expected=first_camera(a.rectified_events);assert t0==t1
with open(a.map,'rb') as f:data=f.read()
magic,version,w,h,count=struct.unpack('<5I',data[:20])
assert (magic,version,w,h)==(0x514d4150,1,320,240) and count in (2,4)
ids=list(struct.unpack('<'+str(count)+'I',data[20:20+count*4]))
assert ids==[0,2,1,3][:count]
dtype=np.dtype([('idx','<u4'),('fx','u1'),('fy','u1'),('pad','<u2')])
maps=np.frombuffer(data,dtype,offset=20+4*count).reshape(count,76800)
for j,c in enumerate(ids):
 m=maps[j];valid=m['idx']!=0xffffffff;k=np.where(valid,m['idx'],0).astype(np.int64)
 assert np.all(k<76800) and np.all(m['fx']<32) and np.all(m['fy']<32)
 fx=m['fx'].astype(np.uint32);fy=m['fy'].astype(np.uint32);flat=source[c].ravel().astype(np.uint32)
 right=(k%320<319).astype(int);down=(k//320<239).astype(int)*320
 result=((flat[k]*(32-fx)*(32-fy)+flat[k+right]*fx*(32-fy)+flat[k+down]*(32-fx)*fy+flat[k+down+right]*fx*fy+512)>>10).astype(np.uint8)
 result[~valid]=0
 delta=np.abs(result.astype(int)-expected[c].ravel().astype(int))
 assert delta.max()<=1,(c,int(delta.max()))
 print(f'camera {c}: max pixel difference {delta.max()}, changed pixels {np.count_nonzero(delta)}; PASS')
