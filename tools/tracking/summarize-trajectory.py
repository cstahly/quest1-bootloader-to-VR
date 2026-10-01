#!/usr/bin/env python3
"""Report measured replay diagnostics, without claiming ground-truth accuracy."""
import argparse,csv,json,math,statistics
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('poses');p.add_argument('--features');p.add_argument('--start-seconds',type=float,default=3)
a=p.parse_args()
with open(a.poses) as f:
    reader=csv.reader(f);header=next(reader);rows=list(reader)
base=['timestamp_ns','x','y','z','qx','qy','qz','qw']
assert header in (base, base+['vx','vy','vz'])
assert rows and all(len(r)==len(header) for r in rows), 'Incomplete CSV'
r=[list(map(float,x)) for x in rows]
assert all(math.isfinite(x) for row in r for x in row)
assert all(x[0]<y[0] for x,y in zip(r,r[1:])), 'Nonmonotonic poses'
t0=r[0][0];s=[x for x in r if x[0]>=t0+a.start_seconds*1e9]
assert len(s)>1, 'Recording too short'
origin=s[0][1:4]
tail=[x for x in s if x[0]>=s[-1][0]-5e9]
tail_mean=[statistics.mean(x[i] for x in tail) for i in range(1,4)]
report=dict(poses=len(r),duration_seconds=(r[-1][0]-t0)/1e9,
    reference_time_seconds=(s[0][0]-t0)/1e9,
    endpoint_offset_m=math.dist(origin,s[-1][1:4]),
    peak_excursion_m=max(math.dist(origin,x[1:4]) for x in s),
    largest_pose_step_m=max(math.dist(x[1:4],y[1:4]) for x,y in zip(s,s[1:])),
    last_five_seconds_max_deviation_m=max(math.dist(tail_mean,x[1:4]) for x in tail),
    caveat='Displacement and stability only; accuracy requires known physical reference positions.')
if a.features:
    f=list(csv.DictReader(open(a.features)))
    assert len(f)==len(r)
    assert all(int(x['timestamp_ns'])==int(y[0]) for x,y in zip(f,r))
    shared=[int(x['multicamera_landmarks']) for x in f]
    report.update(shared_landmarks_median=statistics.median(shared),shared_landmarks_max=max(shared))
print(json.dumps(report,indent=2))
