#!/usr/bin/env python3
"""Build a private, fixed-depth stereo passthrough lookup from factory calibration.

Raw camera images and display lenses are separate optical calibrations. This
combines both once offline; runtime only gathers camera pixels. No scene depth is
inferred. Output is device-specific and should not be committed.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct
import numpy as np
project = runpy.run_path(str(Path(__file__).with_name('preview-calibration.py')))['project']


def lens_ray(mesh, eye, channel, gx, gy):
    ix, iy = np.clip(gx.astype(int), 0, 31), np.clip(gy.astype(int), 0, 31)
    u, v = gx-ix, gy-iy
    return ((1-u)*(1-v))[..., None]*mesh[iy, eye, ix, channel] + (u*(1-v))[..., None]*mesh[iy, eye, ix+1, channel] + ((1-u)*v)[..., None]*mesh[iy+1, eye, ix, channel] + (u*v)[..., None]*mesh[iy+1, eye, ix+1, channel]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('calibration');parser.add_argument('lens');parser.add_argument('output')
    args = parser.parse_args()
    cameras = json.load(open(args.calibration))['CameraCalibration']
    assert len(cameras) == 4
    data = Path(args.lens).read_bytes();assert len(data) == 52368
    mesh = np.frombuffer(data, '<f4', offset=96).reshape(33, 2, 33, 3, 2)
    width, height, step = 2880, 1600, 4
    mapping = np.full((height//step, width//step, 3), 0xffffffff, dtype='<u4')
    for eye in range(2):
        # Solve the green forward optical axis and preserve wearer-accepted centers.
        center = np.array([16., 16.])
        for _ in range(12):
            ray = lens_ray(mesh, eye, 1, center[0], center[1])
            dx = (lens_ray(mesh, eye, 1, center[0]+.001, center[1])-ray)/.001
            dy = (lens_ray(mesh, eye, 1, center[0], center[1]+.001)-ray)/.001
            center -= np.linalg.solve(np.stack([dx, dy], axis=1), ray)
        shift = (620 if eye else 820)-center[0]*45
        x, y = np.meshgrid(np.arange(0,1440,step)+step/2, np.arange(0,1600,step)+step/2)
        gx, gy = (x-shift)/45, 32-y/50
        visible = (gx>=0)&(gx<=32)&(gy>=0)&(gy<=32)
        for channel in range(3):
            rays = lens_ray(mesh, eye, channel, gx, gy)
            points = np.stack([rays[...,0]*3+(.03175 if eye else -.03175), rays[...,1]*3, np.full_like(gx,-3)], axis=-1)
            best = np.zeros_like(gx)
            destination = mapping[:,eye*(1440//step):(eye+1)*(1440//step),channel]
            for camera in cameras:
                u, v, facing = project(points,camera)
                valid=visible&(facing>.05)&(u>=2)&(u<638)&(v>=2)&(v<478)&(facing>best)
                sx,sy=np.clip((u/2).astype(int),0,319),np.clip((v/2).astype(int),0,239)
                destination[valid]=int(camera['Id'])*320*240+sy[valid]*320+sx[valid]
                best[valid]=facing[valid]
    assert np.all((mapping==0xffffffff)|(mapping<4*320*240))
    with open(args.output,'wb') as output:
        output.write(struct.pack('<6I',0x51504153,1,width,height,step,3))
        output.write(mapping.tobytes())
    print('Saved fixed3m stereo lookup:',mapping.shape, 'coverage',float(np.mean(mapping!=0xffffffff)))


if __name__ == '__main__':main()
