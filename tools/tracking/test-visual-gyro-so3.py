#!/usr/bin/env python3
"""Rotation order, endpoint interpolation, extrinsics and delay-convention tests."""
import importlib.util
from pathlib import Path
import unittest
import numpy as np
from scipy.spatial.transform import Rotation

spec = importlib.util.spec_from_file_location('audit', Path(__file__).with_name('audit-visual-gyro-so3.py'))
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class IntegralTest(unittest.TestCase):
    def test_fractional_linear_endpoints(self):
        g = audit.GyroIntegral([0, 1, 2], [[0, 0, 0], [0, 0, 2], [0, 0, 4]])
        expected = Rotation.from_rotvec([0, 0, 1.25**2-.25**2])
        self.assertLess((expected.inv()*g.between(.25, 1.25)).magnitude(), 1e-12)

    def test_noncommuting_order(self):
        g = audit.GyroIntegral([0, 1, 2], [[2, 0, 0], [0, 0, 0], [0, 2, 0]])
        expected = Rotation.from_rotvec([1, 0, 0])*Rotation.from_rotvec([0, 1, 0])
        result = g.between(0, 2)
        self.assertLess((expected.inv()*result).magnitude(), 1e-12)
        self.assertGreater((Rotation.from_rotvec([1, 1, 0]).inv()*result).magnitude(), .4)

    def test_arbitrary_calibrated_camera_rotation(self):
        ric = Rotation.from_euler('xyz', [.3, -.5, .7])
        body = Rotation.from_rotvec([.1, .3, -.2])
        recovered_camera = (ric.inv()*body.inv()*ric).as_matrix()
        visual = audit.visual_in_imu(recovered_camera, ric)
        self.assertLess((body.inv()*visual).magnitude(), 1e-12)

    def test_known_positive_delay(self):
        t = np.linspace(0, 3, 3001)
        g = audit.GyroIntegral(t, np.column_stack([np.sin(t*3), np.cos(t*2), t*t]))
        pairs = [dict(camera=i%4, start=s, end=s+.1,
                      visual=g.between(s+.025, s+.125)) for i, s in enumerate(np.linspace(.2, 2.5, 20))]
        report = audit.scan(pairs, g, list(range(-80, 81, 5)))
        for row in report:
            self.assertEqual(row['best_delay_ms'], 25)
            self.assertLess(row['median_error_deg'], 1e-10)

    def test_out_of_support_rejected(self):
        g = audit.GyroIntegral([0, 1], [[0, 0, 0], [0, 0, 0]])
        with self.assertRaises(ValueError):
            g.between(-.1, .5)
        with self.assertRaises(ValueError):
            g.between(.5, 1.1)
        with self.assertRaises(ValueError):
            g.between(.5, .5)

    def test_no_pairs_no_estimate(self):
        g = audit.GyroIntegral([0, 1], [[0, 0, 0], [0, 0, 0]])
        for row in audit.scan([], g, [-5, 0, 5]):
            self.assertEqual(row['pairs'], 0)
            self.assertNotIn('best_delay_ms', row)


if __name__ == '__main__':
    unittest.main()
