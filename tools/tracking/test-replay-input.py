#!/usr/bin/env python3
"""Exercise replay validation without starting a tracker or accessing hardware."""
import pathlib
import os
import struct
import subprocess
import sys
import tempfile

binary = str(pathlib.Path(sys.argv[1]).resolve())
event = lambda k, t, n: struct.pack('<cQI', k, t, n)
cases = {
    'empty': (b'', 'no camera events'),
    'short-header': (b'C\0', 'truncated event header'),
    'short-payload': (event(b'C', 1, 307200) + b'\0', 'truncated event payload'),
    'bad-size': (event(b'I', 1, 25), 'invalid event type/size'),
    'bad-kind': (event(b'X', 1, 24), 'invalid event type/size'),
    'backward-clock': (event(b'I', 2, 24) + bytes(24) + event(b'I', 1, 24), 'nonmonotonic'),
}
with tempfile.TemporaryDirectory() as directory:
    root = pathlib.Path(directory)
    for name, (data, expected) in cases.items():
        source = root / name
        source.write_bytes(data)
        output = root / (name + '.csv')
        result = subprocess.run([binary, 'must-not-open-config', str(source), str(output)],
                                capture_output=True, text=True, timeout=5)
        assert result.returncode == 2, (name, result)
        assert expected in result.stderr, (name, result.stderr)
        assert not output.exists() and not pathlib.Path(str(output)+'.partial').exists()
        print(name, 'PASS')
    for name, mask in {
        'mask-negative-camera': '-1 0 0 10 10\n',
        'mask-camera-range': '4 0 0 10 10\n',
        'mask-outside-image': '0 319 0 2 1\n',
        'mask-truncated': '0 1 2\n',
        'mask-nonnumeric': 'invalid\n',
    }.items():
        mask_path = root / name
        mask_path.write_text(mask)
        output = root / (name + '.csv')
        result = subprocess.run([binary, 'must-not-open-config', 'must-not-open-events',
                                 str(output), str(root/'features.csv'), '4', str(mask_path)],
                                capture_output=True, text=True, timeout=5)
        assert result.returncode == 2 and 'invalid mask' in result.stderr, (name, result)
        assert not output.exists()
        print(name, 'PASS')

# Deterministic VIT waits inside image push. Reject recordings that cannot provide
# the strictly later IMU sample before entering that potentially blocking call.
with tempfile.TemporaryDirectory() as directory:
    root = pathlib.Path(directory)
    camera = event(b'C', 10, 307200) + bytes(307200)
    equal_imu = event(b'I', 10, 24) + bytes(24)
    next_camera = event(b'C', 20, 307200) + bytes(307200)
    for name, data, message in (
        ('unbracketed-final', camera, 'final camera lacks future IMU bracket'),
        ('equal-is-not-future', camera+equal_imu, 'final camera lacks future IMU bracket'),
        ('unbracketed-intermediate', camera+next_camera, 'camera lacks future IMU bracket before next camera'),
    ):
        source = root/name
        source.write_bytes(data)
        result = subprocess.run([binary, 'must-not-open-config', str(source), str(root/'output.csv')],
                                env=dict(os.environ, QUEST_REPLAY_BRACKET_IMU='1'),
                                capture_output=True, text=True, timeout=5)
        assert result.returncode == 2 and message in result.stderr, (name, result)
        assert not (root/'output.csv.partial').exists()
        print(name, 'PASS')
    result = subprocess.run([binary, 'must-not-open-config', 'must-not-open-events', str(root/'out.csv')],
                            env=dict(os.environ, QUEST_REPLAY_BRACKET_IMU='0'),
                            capture_output=True, text=True, timeout=5)
    assert result.returncode == 2 and 'must be 1 or unset' in result.stderr
    print('bracket-option-validation PASS')
