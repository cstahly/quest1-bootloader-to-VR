# 7 — Positional tracking: current research, not an installation step

The installed headset still has **orientation-only tracking (3DoF)**. Leaning or
walking does not yet move the virtual viewpoint. No live Basalt positional pose has
been connected to Monado or the scene. Thumbstick locomotion is not the goal.

## What runs

Basalt from the mateosss fork, pinned at
`df6e970c8da7636eb401a09e3317fbeaaf829b9a`, implements the VIT interface. The tested
build is float-only. Our offline path is:

```
passive SyncBoss IMU + four-camera preview → QVRREC01 recording
→ prepare-replay.py → optional rectify-replay.py
→ optional select-replay-cameras.py → replay-vit.cpp
→ timestamp-matched pose CSV + feature counts → summarize-trajectory.py
```

Sources are in [tools/tracking](../../tools/tracking). Raw images, owner calibration,
recordings and generated calibration files stay outside Git. Captures stream over
Wi-Fi to the host so unplugging USB does not interrupt them. This is a handheld
measurement task; walking while wearing an unvalidated tracking system is unnecessary.

The preview has four 320×240 monochrome images at about 30 Hz and the IMU about
1,015 Hz. Preparation converts accelerometer g to m/s² and gyro degrees/s to radians/s,
using the measured sensor-to-head mapping `(-Y, -X, -Z)`. MCU ticks and camera times
are different clocks; transport-derived alignment is approximate, not calibrated
exposure timing. Optional clock fitting has not resolved the drift.

## What the evidence actually says

- Replay now drains successfully and checks one pose per camera group at the exact
  input timestamp. Earlier hangs came from excess trailing IMU samples; a separate
  Basalt null end-of-stream pose bug is fixed by patch 0001.
- Original stationary replay drifted about 14 m. Down-weighting acceleration reduced
  that dramatically, but **did not prove visual calibration or correct motion scale**.
- Original images yielded almost no shared-camera landmarks. Offline rectification
  to a common virtual pinhole direction improved results. Camera ordering has
  supporting correspondence evidence, not a completed calibration acceptance test.
- With rectification, grid size 25, pyramid levels 2 and recovered-distance-squared
  threshold 1.0, original noise settings gave about 4.9 mm endpoint drift and 24 mm maximum
  excursion on the stationary clip after the first 3 seconds. This is one clip.
- A short, small movement returned within about 2.5 cm using the first pose as reference;
  its travel distance was not measured. Do not label it a successful 50 cm scale test.
- A 30-second intended 50 cm recording reached about 50 cm, but its endpoint was about 29 cm
  away and the user was trying to return at the end. That is inconclusive.
- The 60-second marked recording is saved. It shows substantial estimated translation
  during an apparently stationary opening. Four-camera replay reached about 87 cm and
  ended 43 cm away; selecting cameras 0/2 improved shared landmarks but did not remove
  drift. Endpoint interpretation also needs the user's movement history.
- Stationary gyro/radial acceleration initialization experiments have not fixed it.
  Accelerometer convention, initialization, camera matching, timing and extrinsics
  remain candidates. There is **no established sole blocker**.

Factory bias arrays currently zeroed in preparation are a documented uncalibrated
baseline, not permission to copy factory numbers into an incompatible convention.
Validate units, axes, matrix direction and bias sign first. Lower drift obtained by
suppressing acceleration is not sufficient acceptance evidence.

## Build status and patch selection

A native aarch64-musl Basalt library and a private19MB dependency bundle now run on
headset. A bounded live probe produced301/301 poses in20seconds from current stereo
camera input at15Hz and full-rate IMU. Median age from driver camera timestamp to
pose polling was43ms, maximum69ms. This is not sensor-to-display latency, accepted
positional accuracy, or a live pose connected to the view.
See [native probe instructions](../../tools/tracking/README.md).

Apply [0001](../../patches/basalt/0001-vit-null-end-of-stream-pose.patch) for the EOS
fix and [0003](../../patches/basalt/0003-headless-cross-build.patch) for the headless
cross-build. **Do not apply every patch in the directory:** experimental 0002 patch
rotation did not improve the tested result and was reverted. Patch 0004 is a host-only
triangulation experiment with mixed replay results; it was reverted and is not an
accepted installation patch. The cross toolchain is
[aarch64-musl-toolchain.cmake](../../tools/tracking/aarch64-musl-toolchain.cmake).
Build dependencies belong in the buildroot, not wholesale on the headset. Runtime
libraries must be audited and staged privately before a device trial.

## Next gates

1. Use the saved recordings to explain stationary startup drift and establish useful
   cross-camera tracks. No further movement capture is presently required.
2. Validate measured displacement, return error and stationary stability across clips;
   document the pose reference and any discarded initialization interval.
3. Check native dependency closure, bounded runtime cost and failure behavior.
4. Integrate through Monado's tracking interface with bounded asynchronous sensor
   delivery. A stalled camera/VIT queue must not block the working orientation thread.
5. Validate coordinate transforms, tracking loss/recovery, latency and wearer behavior
   before enabling positional tracking at boot.

[Detailed experiments](../basalt-positional-tracking.md) ·
[Current work and TODO](09-status-and-next.md) · [Gotchas](08-safety-and-dead-ends.md)
