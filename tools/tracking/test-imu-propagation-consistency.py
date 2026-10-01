#!/usr/bin/env python3
import importlib.util
from pathlib import Path
import unittest
import numpy as np
spec=importlib.util.spec_from_file_location('imu_consistency',Path(__file__).with_name('imu-propagation-consistency.py'))
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)

class PropagationTests(unittest.TestCase):
    def setUp(self):
        self.t=np.arange(0,101000000,100000,dtype=np.int64)
        self.zero=np.zeros(3)
    def run_motion(self,force,gyro=np.zeros(3),initial=np.eye(3)):
        samples=np.column_stack((np.tile(force,(len(self.t),1)),np.tile(gyro,(len(self.t),1))))
        return m.propagate(0,100000000,self.zero,self.zero,initial,self.t,samples)
    def test_stationary_and_tilted_gravity(self):
        for r in [np.eye(3),m.exp_so3(np.array([.7,-.4,.2]))]:
            p,v,q=self.run_motion(r.T@(-m.GRAVITY),initial=r)
            np.testing.assert_allclose(p,0,atol=1e-12);np.testing.assert_allclose(v,0,atol=1e-12)
            np.testing.assert_allclose(q,r,atol=1e-12)
    def test_constant_acceleration(self):
        a=np.array([2.,-1.,.5]);p,v,r=self.run_motion(a-m.GRAVITY)
        np.testing.assert_allclose(p,.5*a*.1**2,atol=1e-12)
        np.testing.assert_allclose(v,a*.1,atol=1e-12)
    def test_rotating_stationary_imu(self):
        omega=np.array([.9,-.5,.2]);r0=m.exp_so3(np.array([.3,.2,-.1]))
        force=np.array([(r0@m.exp_so3(omega*(t*1e-9))).T@(-m.GRAVITY) for t in self.t])
        samples=np.column_stack((force,np.tile(omega,(len(self.t),1))))
        p,v,r=m.propagate(0,100000000,self.zero,self.zero,r0,self.t,samples)
        # Left-hold specific force has first-order discretization error while
        # body rotates. At100us intervals it must remain small, not cancel by fiat.
        self.assertLess(np.linalg.norm(p),3e-6);self.assertLess(np.linalg.norm(v),6e-5)
        np.testing.assert_allclose(r,r0@m.exp_so3(.1*omega),atol=1e-11)
    def test_no_future_imu_used(self):
        samples=np.tile([0.,0.,9.81,0.,0.,0.],(len(self.t),1))
        target=50123456
        before=m.propagate(0,target,self.zero,self.zero,np.eye(3),self.t,samples)
        samples[self.t>target]=np.nan
        after=m.propagate(0,target,self.zero,self.zero,np.eye(3),self.t,samples)
        for a,b in zip(before,after):np.testing.assert_array_equal(a,b)
    def test_gap_and_invalid_state(self):
        times=np.array([0,20000000]);samples=np.tile([0.,0.,9.81,0.,0.,0.],(2,1))
        with self.assertRaisesRegex(ValueError,'IMU gap'):
            m.propagate(0,20000000,self.zero,self.zero,np.eye(3),times,samples)
        with self.assertRaises(ValueError):m.rotation_from_quaternion([0,0,0,0])
        with self.assertRaisesRegex(ValueError,'nonfinite state'):
            m.propagate(0,100,self.zero,np.array([np.nan,0,0]),np.eye(3),times,samples)
    def test_reference_endpoints_and_quaternion_sign(self):
        times=np.array([0,100000000]);pos=np.array([[0.,0.,0.],[1.,2.,3.]])
        qs=np.array([[0.,0.,0.,1.],[0.,0.,0.,-1.]])
        p,r=m.reference_at(35000000,times,pos,qs)
        np.testing.assert_allclose(p,[.35,.7,1.05]);np.testing.assert_allclose(r,np.eye(3))
        with self.assertRaises(ValueError):m.reference_at(100000001,times,pos,qs)

if __name__=='__main__':unittest.main()
