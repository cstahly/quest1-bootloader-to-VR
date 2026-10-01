#!/usr/bin/env python3
import hashlib
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('shift', Path(__file__).with_name('shift-replay-camera-time.py'))
shift = importlib.util.module_from_spec(spec)
spec.loader.exec_module(shift)


def event(kind, stamp, payload):
    return struct.pack('<cQI', kind, stamp, len(payload))+payload


class ShiftTest(unittest.TestCase):
    def test_reorder_preserves_imu_and_camera_payload(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory)/'in.bin'
            out = Path(directory)/'out.bin'
            i1 = event(b'I', 100, bytes(range(24)))
            i2 = event(b'I', 200, bytes(reversed(range(24))))
            pixels = b'\xA5'*307200
            source.write_bytes(i1+i2+event(b'C', 210, pixels))
            report = shift.shift(source, out, -60)
            self.assertEqual(out.read_bytes(), i1+event(b'C', 150, pixels)+i2)
            self.assertEqual(report['unchanged_imu_records_sha256'], hashlib.sha256(i1+i2).hexdigest())
            with self.assertRaises(FileExistsError):
                shift.shift(source, out, -60)

    def test_underflow_rejected_without_output(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory)/'in.bin'
            out = Path(directory)/'out.bin'
            source.write_bytes(event(b'I', 1, bytes(24))+event(b'C', 2, bytes(307200)))
            with self.assertRaisesRegex(ValueError, 'out of range'):
                shift.shift(source, out, -10)
            self.assertFalse(out.exists())


if __name__ == '__main__':
    unittest.main()
