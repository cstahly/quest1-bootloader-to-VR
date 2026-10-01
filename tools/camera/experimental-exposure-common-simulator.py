#!/usr/bin/env python3
"""OFFLINE synthetic policy experiment; never imported by capture or deployed.

Compiles a temporary host wrapper around the real exposure-policy.h so the current
policy is not reimplemented. Requires NumPy and a host C compiler. Synthetic scenes
are not real camera evidence or a tracking-quality benchmark.
"""
import argparse
import ctypes
import json
from pathlib import Path
import subprocess
import tempfile

import numpy as np

ROOT = Path(__file__).resolve().parent
STEP = 19  # Reported metadata unit; MCU rounding remains unverified.


def build_reference(folder):
    source = Path(folder) / 'reference.c'
    source.write_text('''#include "exposure-policy.h"
void reference(const unsigned char *p, const unsigned *us, const unsigned *gain,
               unsigned *out, int grouped) {
    struct quest_exposure s[4];
    if(grouped)quest_exposure_group_next((const unsigned char (*)[320*240])p,us,gain,s);
    else for(unsigned c=0;c<4;c++)s[c]=quest_exposure_next(p+c*320*240,us[c],gain[c]);
    for(unsigned c=0;c<4;c++){out[c*3]=s[c].us;out[c*3+1]=s[c].gain;out[c*3+2]=s[c].brightness;}
}
''')
    libpath = Path(folder) / 'reference.so'
    subprocess.run(['cc', '-shared', '-fPIC', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-I', str(ROOT), str(source), '-o', str(libpath)], check=True)
    lib = ctypes.CDLL(str(libpath))
    pointer = ctypes.c_void_p
    lib.reference.argtypes = [pointer, pointer, pointer, pointer, ctypes.c_int]
    lib.reference.restype = None

    def reference(images, us, gain, grouped=True):
        arrays = [np.ascontiguousarray(images, dtype=np.uint8),
                  np.ascontiguousarray(us, dtype=np.uint32),
                  np.ascontiguousarray(gain, dtype=np.uint32)]
        out = np.empty((4, 3), dtype=np.uint32)
        lib.reference(*(a.ctypes.data for a in arrays), out.ctypes.data, int(grouped))
        return out.astype(float)
    return reference


def observed_statistics(images):
    roi = images[:, 30:210:2, 40:280:2].astype(float)
    # Same sampled ROI/p70 as current policy. These are observable proxies only.
    p70 = np.sort(roi.reshape(4, -1), axis=1)[:, 7559]
    clipped = np.mean(roi == 255, axis=(1, 2))
    tiles = roi.reshape(4, 9, 10, 12, 10).transpose(0, 1, 3, 2, 4).reshape(4, 108, 100)
    spread = np.quantile(tiles, .8, axis=2) - np.quantile(tiles, .2, axis=2)
    middle = np.median(tiles, axis=2)
    coverage = np.mean((spread >= 12) & (middle >= 8) & (middle <= 245), axis=1)
    return p70, clipped, coverage


def candidate(reference, images, us, gain):
    """Common shutter compromise; no scene labels or simulated radiance accessed.

    Target products come from the real existing per-camera brightness policy.
    Minimize weighted squared log product errors over feasible common shutters and
    integer gains. Every view retains >=1/4 weight, including covered/dark views.
    Observable spatial contrast affects weight, never establishes occlusion truth.
    Small preference for 4 ms keeps the old nominal operating point. Quantized
    shutter margin, bounded shutter change and per-view product targets limit jumps.
    """
    target = reference(images, us, gain, grouped=False)
    desired = target[:, 0] * target[:, 1]
    _, _, coverage = observed_statistics(images)
    weights = .25 + .75 * coverage
    shutters = np.arange(1026, 7981, STEP, dtype=float)
    # Common shutter is a precondition; one captured exposure cohort is the state.
    assert np.all(us == us[0])
    low = max(1026, us[0] * .75)
    high = max(low, min(7980, us[0] * 1.33))
    allowed = shutters[(shutters >= low) & (shutters <= high)]
    if len(allowed) == 0:  # Initial state may be outside candidate bounds.
        allowed = shutters[np.argmin(abs(shutters - np.clip(us[0], 1026, 7980)))][None]
    gains = np.clip(np.floor(desired[None, :] / allowed[:, None] + .5), 16, 240)
    achieved = allowed[:, None] * gains
    mismatch = np.sum(weights[None, :] * np.log(achieved / desired[None, :]) ** 2, axis=1)
    preference = .01 * np.log(allowed / 4000) ** 2 + .002 * allowed / 4000
    index = np.argmin(mismatch + preference)
    return np.full(4, allowed[index]), gains[index], weights


def quantize(us, mode='floor', cap=12008):
    if mode == 'floor':
        result = np.floor(us / STEP) * STEP
    elif mode == 'ceil':
        result = np.ceil(us / STEP) * STEP
    else:
        result = np.floor(us / STEP + .5) * STEP
    return np.minimum(result, cap)


def scene(name, iteration):
    y, x = np.indices((240, 320), dtype=float)
    textured = 70 + 28 * np.sin(x / 8) * np.sin(y / 10) + 15 * ((x // 31 + y // 29) % 2)
    if name == 'normal':
        factors = [1, .7, 1.3, .9]
    elif name == 'all_bright':
        factors = [50, 50, 50, 50]
    elif name == 'mixed':
        factors = [.06, .15, 8, 12]
    elif name == 'covered_one':
        factors = [0, 1, 1, 1]
    elif name == 'uniform_dark':
        return np.full((4, 240, 320), 2.)
    elif name == 'dark_texture':
        factors = [.05, .06, .04, .08]
    elif name == 'light_switch':
        factors = ([.06, .06, .06, .06] if 12 <= iteration < 30 else [1, .7, 1.3, .9])
    elif name == 'single_useful_dark':
        # Dark textured view versus three bright featureless views: prevents an
        # experiment silently assuming that every bright view is useful.
        return np.stack([textured * .2] + [np.full_like(textured, 1000.)] * 3)
    else:
        raise ValueError(name)
    return np.stack([textured * f for f in factors])


def observe(radiance, us, gains, noise):
    # Explicit toy model: no ISP, optical PSF, vignetting, blur or sensor black level.
    expected = radiance * (us * gains / 192000)[:, None, None]
    sigma = .35 * np.sqrt(np.maximum(expected, 0) * gains[:, None, None] / 48) + gains[:, None, None] / 64
    return np.clip(np.rint(expected + noise * sigma), 0, 255).astype(np.uint8)


def simulate(reference, name, policy, rounding, cap, steps=48):
    rng = np.random.default_rng(731)
    us = np.full(4, 3990.)
    gains = np.full(4, 48.)
    rows = []
    for i in range(steps):
        images = observe(scene(name, i), us, gains, rng.standard_normal((4, 240, 320)))
        p70, clipped, coverage = observed_statistics(images)
        if policy == 'current':
            next_setting = reference(images, us, gains)
            request_us, request_gain = next_setting[:, 0], next_setting[:, 1]
            weights = np.ones(4)
        else:
            request_us, request_gain, weights = candidate(reference, images, us, gains)
        assert np.all(request_us == request_us[0])
        assert np.all((request_gain >= 16) & (request_gain <= 240))
        rows.append(dict(step=i, us=float(us[0]), gains=gains.tolist(), p70=p70.tolist(),
                         clipped=clipped.tolist(), coverage=coverage.tolist(), weights=weights.tolist(),
                         short_rejected=bool(np.any(us < 1000))))
        # Models policy only. Real capture would stop updating scene statistics
        # if metadata fell below 1000, hence this simulator flags rather than
        # falsely modeling that state as a continuously working capture system.
        us = quantize(request_us, rounding, cap)
        gains = request_gain
    tail = rows[-8:]
    shutter = np.array([r['us'] for r in rows])
    changes = np.diff(shutter)
    significant = changes[abs(changes) >= 76]
    reversals = int(np.sum(significant[1:] * significant[:-1] < 0))
    def avg(key):
        return np.round(np.mean([r[key] for r in tail], axis=0), 4).tolist()
    return dict(final_us=rows[-1]['us'], final_gains=rows[-1]['gains'],
                tail_p70=avg('p70'), tail_clipped=avg('clipped'), tail_coverage=avg('coverage'),
                tail_us_span=float(np.ptp([r['us'] for r in tail])),
                shutter_direction_reversals=reversals,
                short_rejected_steps=sum(r['short_rejected'] for r in rows), trace=rows)


def tests(reference):
    images = np.full((4, 240, 320), 80, dtype=np.uint8)
    us = np.full(4, 4000.)
    gains = np.full(4, 48.)
    baseline = reference(images, us, gains)
    assert np.all(baseline[:, :2] == [4000, 48])
    for level in [0, 10, 80, 255]:
        images[:] = level
        shutter, gain, weights = candidate(reference, images, us, gains)
        assert np.all(shutter == shutter[0])
        assert np.all(shutter % STEP == 0)
        assert np.all((shutter >= 1026) & (shutter <= 7980))
        assert np.all((gain >= 16) & (gain <= 240))
        assert np.all((weights >= .25) & (weights <= 1))
    assert quantize(np.array([1000.]), 'floor')[0] == 988
    for mode in ['floor', 'ceil', 'nearest']:
        assert quantize(np.array([1026.]), mode)[0] >= 1000
    # Camera identity/permutation does not create a privileged view.
    images = observe(scene('mixed', 0), us, gains, np.zeros((4, 240, 320)))
    order = np.array([2, 0, 3, 1])
    a = candidate(reference, images, us, gains)
    b = candidate(reference, images[order], us[order], gains[order])
    assert np.array_equal(a[0][order], b[0])
    assert np.array_equal(a[1][order], b[1])
    assert np.array_equal(a[2][order], b[2])
    # Empty common-shutter feasible interval in the documented conflict.
    products = np.array([1920000., 96000., 96000., 96000.])
    assert np.max(products / 240) > np.min(products / 16)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--trace', action='store_true')
    args = parser.parse_args()
    result = dict(disclaimer='Synthetic brightness/contrast proxies, NOT measured tracking performance.',
                  timestep_seconds=.5, candidate='EXPERIMENTAL: bounded weighted common-shutter compromise', cases={})
    with tempfile.TemporaryDirectory(prefix='quest-exposure-sim-') as temp:
        reference = build_reference(temp)
        tests(reference)
        for name in ['normal', 'all_bright', 'mixed', 'covered_one', 'uniform_dark', 'dark_texture',
                     'light_switch', 'single_useful_dark']:
            result['cases'][name] = {}
            for policy in ['current', 'candidate']:
                record = simulate(reference, name, policy, 'floor', 12008)
                if not args.trace:
                    record.pop('trace')
                result['cases'][name][policy] = record
        # Quantization and observed-cap sensitivity, not claims about actual MCU.
        for mode, cap in [('ceil', 12008), ('nearest', 12008), ('floor', 6802)]:
            key = f'mixed_{mode}_cap{cap}'
            result['cases'][key] = {}
            for policy in ['current', 'candidate']:
                record = simulate(reference, 'mixed', policy, mode, cap)
                if not args.trace:
                    record.pop('trace')
                result['cases'][key][policy] = record
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(f'Synthetic policy invariants passed; results: {args.output}')


if __name__ == '__main__':
    main()
