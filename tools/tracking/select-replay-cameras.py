#!/usr/bin/env python3
"""Select physical cameras from an existing private replay without altering it."""
import argparse,json,struct
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('source');p.add_argument('output');p.add_argument('cameras',help='e.g. 0,2')
a=p.parse_args();src=Path(a.source);out=Path(a.output);out.mkdir(exist_ok=True,parents=True)
indices=[int(x) for x in a.cameras.split(',')]
cal=json.load(open(src/'calibration.json'));ncam=len(cal['value0']['intrinsics'])
assert 2<=len(indices)<=4 and len(set(indices))==len(indices) and all(0<=x<ncam for x in indices)
for k in ['intrinsics','T_imu_cam','resolution']:cal['value0'][k]=[cal['value0'][k][i] for i in indices]
(out/'calibration.json').write_text(json.dumps(cal,indent=2))
(out/'camera-selection.json').write_text(json.dumps({'source':str(src),'source_camera_indices':indices},indent=2))
with open(src/'events.bin','rb') as inp,open(out/'events.bin','wb') as dest:
    while h:=inp.read(13):
        kind,t,n=struct.unpack('<cQI',h);b=inp.read(n);assert len(b)==n
        if kind==b'C':
            assert n==ncam*76800
            b=b''.join(b[i*76800:(i+1)*76800] for i in indices)
        dest.write(struct.pack('<cQI',kind,t,len(b)));dest.write(b)
