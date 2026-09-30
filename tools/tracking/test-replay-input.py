#!/usr/bin/env python3
"""Exercise replay validation without starting a tracker or accessing hardware."""
import pathlib
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
