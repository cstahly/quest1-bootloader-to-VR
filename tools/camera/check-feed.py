#!/usr/bin/env python3
"""Bounded read-only camera publication/cohort statistics; no image output."""
import argparse,json,struct,time,statistics
p=argparse.ArgumentParser();p.add_argument('feed');p.add_argument('--seconds',type=int,default=5,choices=range(1,31));a=p.parse_args()
start=time.monotonic();seen={};stale=0
while time.monotonic()-start<a.seconds:
 with open(a.feed,'rb') as f:b=f.read()
 assert len(b)==307328 and struct.unpack_from('<5I',b)==(0x5143414d,1,320,240,4)
 host=time.monotonic_ns();publication=struct.unpack_from('<I',b,20)[0];ts=struct.unpack_from('<4Q',b,24);published=struct.unpack_from('<Q',b,120)[0]
 stale+=host-published>150000000
 seen[publication]=(max(ts)-min(ts),max(0,host-published))
 time.sleep(.005)
elapsed=time.monotonic()-start
print(json.dumps(dict(publications=len(seen),observed_hz=len(seen)/elapsed,synchronized=sum(v[0]<=1000000 for v in seen.values()),max_skew_ms=max(v[0] for v in seen.values())/1e6,stale_reads=stale,exposure_us=struct.unpack_from('<4I',b,72),gain_q4=struct.unpack_from('<4I',b,88)),indent=2))
