#!/usr/bin/env python3
"""Synthetic clock and packed-camera ABI checks for the read-only timing audit."""
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('timing', Path(__file__).with_name('audit-capture-timing.py'))
timing = importlib.util.module_from_spec(spec)
spec.loader.exec_module(timing)


class TimingTest(unittest.TestCase):
    def test_known_clock_rate_and_camera_offsets(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'test.bin'
            with path.open('wb') as out:
                out.write(b'QVRREC01')
                for n in range(3):
                    b = bytearray(307336)
                    camera = 10_000_000_000+n*33_333_333
                    struct.pack_into('<5I', b, 8, 0x5143414d, 1, 320, 240, 4)
                    struct.pack_into('<Q', b, 0, camera+9_000_000)
                    struct.pack_into('<4Q', b, 32, camera, camera, camera, camera)
                    struct.pack_into('<4I', b, 80, 4000, 4000, 4000, 4000)
                    struct.pack_into('<Q', b, 128, camera+2_000_000)
                    out.write(struct.pack('<cI', b'C', len(b))+b)
                csv = 'host_monotonic_ns,device_timestamp\n'
                for n in range(10001):
                    tick = n*1000
                    host = 10_000_000_000+tick*1000+n*100
                    csv += f'{host},{tick}\n'
                data = csv.encode()
                out.write(struct.pack('<cI', b'I', len(data))+data)
            result = timing.audit(path)
            self.assertAlmostEqual(result['transport_fit_rate_ppm'], 100, places=6)
            self.assertAlmostEqual(result['transport_fit_drift_over_recording_ms'], 1, places=6)
            self.assertEqual(result['driver_to_publication_ms']['median'], 2)
            self.assertEqual(result['publication_to_recorder_ms']['median'], 7)
            self.assertEqual(result['exposure_us_per_camera'][3]['median'], 4000)

    def check_rejected(self, records, message):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'bad.bin'
            path.write_bytes(b'QVRREC01'+b''.join(
                struct.pack('<cI', kind, len(payload))+payload for kind, payload in records))
            with self.assertRaisesRegex(ValueError, message):
                timing.audit(path)

    @staticmethod
    def camera_record():
        payload = bytearray(307336)
        struct.pack_into('<5I', payload, 8, 0x5143414d, 1, 320, 240, 4)
        return b'C', payload

    def test_single_camera_rejected(self):
        imu = b'host_monotonic_ns,device_timestamp\n10000000,0\n11000000,1000\n12000000,2000\n'
        self.check_rejected([self.camera_record(), (b'I', imu)], 'insufficient camera/IMU')

    def test_two_imu_samples_rejected(self):
        imu = b'host_monotonic_ns,device_timestamp\n10000000,0\n11000000,1000\n'
        self.check_rejected([self.camera_record(), self.camera_record(), (b'I', imu)],
                            'insufficient camera/IMU')

    def test_sparse_lower_envelope_rejected(self):
        imu = b'host_monotonic_ns,device_timestamp\n10000000,0\n11500000,1000\n12000000,2000\n13500000,3000\n'
        self.check_rejected([self.camera_record(), self.camera_record(), (b'I', imu)],
                            'insufficient distinct points')

    def test_bad_camera_header_rejected(self):
        for index, value in enumerate((0, 2, 640, 480, 3)):
            with self.subTest(field=index):
                kind, payload = self.camera_record()
                struct.pack_into('<I', payload, 8+index*4, value)
                self.check_rejected([(kind, payload)], 'invalid camera ABI header')

    def test_unknown_record_rejected(self):
        self.check_rejected([(b'X', b'')], 'unknown record type')

    def test_truncated_header_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'bad.bin'
            path.write_bytes(b'QVRREC01C')
            with self.assertRaisesRegex(ValueError, 'truncated'):
                timing.audit(path)


if __name__ == '__main__':
    unittest.main()
