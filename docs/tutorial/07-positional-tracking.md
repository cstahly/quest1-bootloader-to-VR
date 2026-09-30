# Stage 7 — Positional tracking (Basalt VIO)

**Goal:** real 6DoF — walk and the view moves with you (physical movement, no joystick
locomotion).

**Status: 🚧 in progress.** The pipeline runs end-to-end and the *vision* half is proven.
The blocker is **accelerometer calibration**, and finishing it needs a controlled
**motion capture** that hasn't been completed. This page is both "how it works" and "how
to finish it."

## Concept

**Basalt** is a visual-inertial odometry library: cameras + IMU → a trajectory. It plugs
into Monado through the VIT interface (`vit_interface.h`). The offline pipeline:

```
capture-imu.c + record-sensors.py   →  QVRREC01 recording
        │
        ▼
prepare-replay.py   →  events.bin + calibration.json
        │
        ▼
replay-vit.cpp (loads libbasalt.so)   →  trajectory CSV
```

Build Basalt from the **mateosss/basalt** fork (implements Monado's VIT ABI), **float-only**
(`use-double=false`).

## What's proven vs. what's blocking

- ✅ **Pipeline works** end to end; ✅ **vision + camera calibration correct** — a
  stationary rig holds within ~10 cm *when the accelerometer is down-weighted*.
- ❌ **Accelerometer calibration is the sole blocker.** The factory IMU intrinsics were
  extracted (`imu_calibration.json`: rectification matrix + bias), but applying them
  naively made drift *worse* — the convention/units/sign and trust weighting aren't
  pinned down. A **known-good motion capture** is needed to tune them.
- ⚠️ **Known bug in `prepare-replay.py`:** it currently zeros
  `calib_accel_bias`/`calib_gyro_bias` — i.e. throws away the factory intrinsic
  calibration. Wire the extracted factory values in (correct convention) as part of the
  fix.

## How to finish it — the motion capture

The earlier attempts failed for a mechanical reason worth stating plainly: **the USB cord
is unplugged while the wearer walks**, so USB-SSH commands time out mid-capture. Do it
over **Wi-Fi**, backgrounded, confirmed-live *before* the walk.

1. Boot; wait for **Wi-Fi** (stage 5) to come up on its own.
2. Restart `oculus-camera`; start `record-sensors.py` streaming to the host **over
   Wi-Fi**, as a background process. **Confirm the stream is actually arriving** before
   any movement. (Record to persistent storage or stream off-device — **not** to `/run`,
   which is tmpfs and is wiped by the ~5-min watchdog reboot.)
3. **Physical action — say this, then pause and wait for the wearer:**
   > "Stand still ~3 s → walk forward ~2 m → walk back onto the mark → stand still ~3 s.
   > Tell me when you're back and how far you walked."
4. Run `prepare-replay.py` (with factory bias wired in) → `replay-vit` → trajectory CSV.
5. Tune the factory rectification/offset convention + `accel_noise_std` / `accel_bias_std`
   until the trajectory shows **correct scale** (matches the ~2 m) and **low drift** (ends
   back near the mark).

## Definition of done

- [ ] A motion capture recorded **over Wi-Fi**, stream confirmed live before the walk.
- [ ] `replay-vit` produces a trajectory whose forward distance matches the real walk
      (correct scale) and returns near the start (low drift) — with the accelerometer
      *trusted*, not suppressed.

## When it fails

- **Commands time out mid-walk:** you're on USB — the cord is out when walking. Use
  Wi-Fi.
- **Recording gone after a reboot:** you wrote to `/run` (tmpfs) and the watchdog
  rebooted before you pulled it. Stream off-device or write to persistent storage.
- **Stationary "drift" of meters:** the headset wasn't moving — a stationary recording
  can't validate motion tracking. You need an actual walk.
- **Applying factory calib makes it worse:** convention/units/sign — the exact thing the
  motion capture is there to pin down. Down-weighting accel (proving vision) is a
  diagnostic, not the fix.

## Full detail

`../basalt-positional-tracking.md` — full state, the factory `imu_calibration.json`
values, the `prepare-replay.py` bug, and the capture gotchas.

→ Next: [`08-safety-and-dead-ends.md`](08-safety-and-dead-ends.md)
