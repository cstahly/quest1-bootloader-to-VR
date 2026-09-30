# 7 — Positional tracking (Basalt VIO)

Real 6DoF — walk and the view moves with you. **WIP:** the pipeline runs end-to-end and
the vision half is proven; the blocker is accelerometer calibration, which needs a
controlled motion capture that hasn't been nailed yet. This is both how it works and how
to finish it.

## How it works

**Basalt** = visual-inertial odometry (cameras + IMU → trajectory), plugged into Monado
via the VIT interface (`vit_interface.h`). Offline pipeline:

```
capture-imu.c + record-sensors.py  →  QVRREC01 recording
   → prepare-replay.py             →  events.bin + calibration.json
   → replay-vit.cpp (libbasalt.so) →  trajectory CSV
```

Build Basalt from the **mateosss/basalt** fork (implements Monado's VIT ABI),
**float-only** (`use-double=false`).

## Proven vs. blocking

- ✅ pipeline works end-to-end; vision + camera calibration correct — stationary rig holds
  ~10 cm *when accel is down-weighted*.
- ❌ **accelerometer calibration** is the sole blocker. Factory IMU intrinsics were
  extracted (`imu_calibration.json`: rectification matrix + bias) but applying them naively
  made drift *worse* — convention/units/sign + trust weighting not pinned down.
- ⚠️ **bug in `prepare-replay.py`:** it zeros `calib_accel_bias`/`calib_gyro_bias`, i.e.
  throws away the factory intrinsics. Wire the extracted values in (right convention) as
  part of the fix.

## Finishing it — the motion capture

Earlier tries failed mechanically: **the cord's unplugged while walking**, so USB-SSH
commands died mid-capture. Do it over **Wi-Fi**, backgrounded, confirmed-live before the
walk.

1. Boot, wait for Wi-Fi (stage 5).
2. Restart `oculus-camera`; start `record-sensors.py` streaming to the host over Wi-Fi as
   a background process. Confirm frames are arriving *before* moving. Don't record to
   `/run` — it's tmpfs, wiped by the ~5-min watchdog reboot; stream off-device or write
   persistent.
3. Walk: still ~3s → forward ~2m → back onto the mark → still ~3s. Note the real distance.
4. `prepare-replay.py` (factory bias wired in) → `replay-vit` → trajectory CSV.
5. Tune the rectification/offset convention + `accel_noise_std` / `accel_bias_std` until
   the trajectory has correct scale (matches ~2m) and low drift (ends near the mark), with
   accel *trusted*.

## Worked when

- a Wi-Fi motion capture, stream confirmed live before the walk
- `replay-vit` gives a trajectory whose forward distance matches the walk and returns near
  start, with accel trusted (not suppressed)

## Snags

- commands time out mid-walk → you're on USB, use Wi-Fi
- recording gone after reboot → wrote to `/run` (tmpfs) + watchdog rebooted; stream or go
  persistent
- meters of "drift" while stationary → headset wasn't moving; a stationary clip can't
  validate motion. You need an actual walk.

Detail: `../basalt-positional-tracking.md` (state, factory `imu_calibration.json` values,
the `prepare-replay.py` bug, capture gotchas).

→ [8 — gotchas + dead ends](08-safety-and-dead-ends.md)
