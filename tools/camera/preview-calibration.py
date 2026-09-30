#!/usr/bin/env python3
"""Offline factory-calibrated mono panorama from one atomic camera feed.

No device writes. Unit-specific calibration stays outside the repository.
Fisheye62 equations: https://facebookresearch.github.io/projectaria_tools/docs/tech_insights/camera_intrinsic_models
This is a chosen-depth projection, not depth-aware stereo or positional tracking.
"""
import argparse
import json
import struct
import numpy as np


def project(points, camera):
    assert camera['Projection']['Model'] == 'PinholeSymmetric'
    assert camera['Distortion']['Model'] == 'Fisheye62'
    transform = np.array(camera['DeviceFromCamera']).reshape(4, 4)
    rotation, position = transform[:3, :3], transform[:3, 3]
    assert np.allclose(rotation.T @ rotation, np.eye(3), atol=1e-5)
    assert abs(np.linalg.det(rotation) - 1) < 1e-5
    local = (points - position) @ rotation
    x, y, z = np.moveaxis(local, -1, 0)
    radius = np.hypot(x, y)
    theta = np.arctan2(radius, z)
    coefficients = camera['Distortion']['Coefficients']
    assert len(coefficients) == 8
    distorted = theta.copy()
    for order, value in enumerate(coefficients[:6]):
        distorted += value * theta ** (2 * order + 3)
    scale = distorted / np.maximum(radius, 1e-12)
    xr, yr = x * scale, y * scale
    r2, p0, p1 = xr*xr + yr*yr, coefficients[6], coefficients[7]
    xd = xr + p0*(2*xr*xr + r2) + 2*p1*xr*yr
    yd = yr + p1*(2*yr*yr + r2) + 2*p0*xr*yr
    focal, cx, cy = camera['Projection']['Coefficients']
    return focal*xd+cx, focal*yd+cy, z/np.maximum(np.linalg.norm(local, axis=-1), 1e-12)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('calibration')
    parser.add_argument('feed')
    parser.add_argument('output')
    parser.add_argument('--distance', type=float, default=3.0)
    args = parser.parse_args()
    if not .2 <= args.distance <= 100:
        parser.error('distance must be between 0.2 and100 metres')
    calibration = json.load(open(args.calibration))['CameraCalibration']
    assert len(calibration) == 4
    data = open(args.feed, 'rb').read()
    assert len(data) == 307328
    assert struct.unpack_from('<5I', data) == (0x5143414D, 1, 320, 240, 4)
    source = np.frombuffer(data, np.uint8, offset=128).reshape(4, 240, 320)
    width, height = 1200, 700
    yaw = np.linspace(-120, 120, width)*np.pi/180
    pitch = np.linspace(70, -70, height)*np.pi/180
    yaw, pitch = np.meshgrid(yaw, pitch)
    points = args.distance*np.stack([np.cos(pitch)*np.sin(yaw), np.sin(pitch), -np.cos(pitch)*np.cos(yaw)], axis=-1)
    best = np.zeros((height, width))
    result = np.zeros((height, width), dtype=np.uint8)
    coverage = np.zeros_like(result)
    for camera in calibration:
        index = int(camera['Id'])
        assert 0 <= index < 4 and camera['ImageSize'] == [640, 480]
        u, v, facing = project(points, camera)
        valid = (facing > .05) & (u >= 2) & (u < 638) & (v >= 2) & (v < 478)
        coverage += valid
        replace = valid & (facing > best)
        sx, sy = np.clip((u/2).astype(int), 0, 319), np.clip((v/2).astype(int), 0, 239)
        result[replace] = source[index, sy[replace], sx[replace]]
        best[replace] = facing[replace]
    tone = (np.maximum(0, (result.astype(float)-4)/251)**(1/2.2)*255).astype(np.uint8)
    with open(args.output, "wb") as output:
        output.write(f"P5\n{width} {height}\n255\n".encode()+tone.tobytes())
    print(f'Factory projection preview at {args.distance:g}m: {np.mean(coverage>0):.1%} coverage, {np.mean(coverage>1):.1%} overlap. Not depth-aware stitching.')


if __name__ == '__main__':
    main()
