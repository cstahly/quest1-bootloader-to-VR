# Basalt 6DoF positional tracking — state + findings

_Roadmap item #4 (camera/IMU positional tracking, physical movement — no joystick
locomotion). The offline-replay toolchain the prior agent built is complete and RUNS;
the blocker is IMU accelerometer calibration/tuning, which needs a controlled MOTION
recording to finish. 2026-09-30._

## TL;DR
- **The whole pipeline works end-to-end** (capture → prepare → Basalt VIO → trajectory).
- **Vision + camera calibration are CORRECT** — proven: a stationary recording holds
  within ~10 cm when the accelerometer is weighted appropriately.
- **The accelerometer is the one broken input.** With default (uncalibrated) accel it
  drifts ~10 m (Y/vertical to 9.8 m — impossible) via ½·a·t² integration of an
  uncorrected ~0.05 m/s² bias.
- **The factory IMU calibration exists** (rectification matrix + bias, pulled from the
  stock system image), but naively applying it made drift *worse* — the drift is
  dominated by accel *trust* at low noise, and the Meta convention + noise tuning need
  nailing. **This can only be finished against a real motion capture**, not a stationary
  clip (you can trivially make stationary hold still by distrusting accel, which ruins
  motion tracking).

## Pipeline (all pieces exist)
```
record-sensors.py + capture-imu  →  QVRREC01 recording (cams+IMU)
   →  prepare-replay.py  →  events.bin + Basalt calibration.json
   →  replay-vit (links libbasalt.so, Basalt VIT ABI)  →  trajectory CSV
```
- **On-headset capture** (`tools/tracking/`): `capture-imu.c` (IMU ~1015 Hz),
  `record-sensors.py` (interleaves the live camera feed + IMU into `QVRREC01`).
  - The headset rootfs has **no C compiler** → cross-build `capture-imu` for aarch64
    in the kali pmbootstrap chroot (`pmbootstrap chroot -r -- cc -O2 -static ...`) and
    push the binary. `/run` is tmpfs, so re-deploy every boot.
  - Camera feed: `/run/quest-camera-root/tmp/camera-feed`, 4× 320×240 L8 @ ~30 fps,
    from the `oculus-camera` service. **It often needs `rc-service oculus-camera
    restart`** after boot (it races startup and the feed stays empty otherwise).
- **Basalt**: mateosss/basalt fork at `kali:~/quest-vio-basalt`, built as
  `build/libbasalt.so`. **Built float-only → must run `use-double=false`** (double
  support not compiled; `use-double=true` fatals).
- **replay-vit**: `kali:~/quest-camera-work/replay-vit` (x86-64, links libbasalt.so).
  Reads `events.bin` (I=6 floats accel+gyro SI; C=4×76800 L8), pushes through the VIT
  interface, drains poses to CSV. Run with
  `LD_LIBRARY_PATH=~/quest-vio-basalt/build` and a CLI11 "unified config":
  ```
  cam-calib="…/replay/calibration.json"
  config-path="~/quest-vio-basalt/data/default_config.json"
  num-threads=4
  use-double=false
  ```
- **Calibration** (pulled from the stock system image / calibration store, in
  `kali:~/quest-camera-work/`): `camera_calibration_v2.json` (has `CameraCalibration`,
  4 cams, `PinholeSymmetric`+`Fisheye62` → mapped to Basalt `fisheye624`);
  `imu_calibration.json` (`Accelerometer`/`Gyroscope` `RectificationMatrix`+`Offset`,
  `DeviceFromImu`). **NOTE `camera_calibration.json` (v1) is the wrong shape (`HeadSets`,
  0 cams) — use v2.**

## Validation results (on the existing stationary recording)
Recording: `vio-record-01.bin` (QVRREC01, 140 MB, 451 cam groups + 17,262 IMU samples).
The headset was **stationary** for this clip → a correct VIO should output ~zero.

| config | stationary drift | meaning |
|---|---|---|
| default (raw accel, no bias), normal trust | ~10 m (Y→9.8 m) | uncorrected accel bias integrates away |
| accel noise ×100 (distrust accel) | **~10 cm** | **vision is correct**; it pins a stationary rig |
| factory rectification + bias, normal trust | ~45 m | naive convention made it worse |
| factory accel rect+bias, raw gyro, normal trust | ~19 m | still worse than raw |

**Interpretation:** the orientation/gyro is fine (quaternion stable). Position diverges
purely from the accelerometer. Vision works. At `accel_noise_std=0.03` the drift is
dominated by accel *trust*, so small calibration tweaks don't cleanly help — the fix is
(a) the correct Meta rectification/offset convention and (b) accel noise + `accel_bias_std`
tuned so Basalt trusts accel enough for motion while estimating bias online.

## Factory IMU calibration (already extracted, in `imu_calibration.json`)
- **Accelerometer** `RectificationMatrix` ≈ axis-align `[-Y,-X,-Z]` + small scale/misalign;
  `Offset` (LinearTemperatureDependence) `OffsetAtZeroDegC = [-0.0230,-0.0340,+0.0516]`.
- **Gyroscope** rectification (same ≈ `[-Y,-X,-Z]`) + `ConstantOffset [0.0127,-0.0093,0.0010]`.
- **Open convention questions** (why naive apply failed): is it `R@raw - offset` or
  `R@(raw - offset)`? offset units (assumed m/s²/rad·s⁻¹)? temperature term (only used
  the 0 °C offset)? gyro-bias sign. Prime suspect for the 45 m result was applying the
  gyro bias with the wrong sign → orientation tilt → gravity leak → huge translation.

## The remaining work (needs a MOTION capture)
1. Capture a **controlled walk**: stand still ~3 s → walk straight ~2 m → walk back onto
   the same mark → still ~3 s. Known out-and-back gives scale (does it read ~2 m?) and
   closed-loop drift (does it return to origin?).
2. With real motion, tune: the accel rectification/offset convention, `accel_noise_std`,
   `accel_bias_std`, and the cam↔IMU `cam_time_offset_ns`, until the trajectory shows
   correct metric translation + low drift.
3. Then wire Basalt into Monado live (the VIT tracker) for real-time 6DoF.

## Capture gotchas (learned the hard way — READ before attempting)
- **`/run` is tmpfs** → recordings AND deployed tools are wiped on reboot. Re-deploy each
  boot; do NOT record to `/run` and pull later — a reboot wipes it first.
- **The guarded image has a ~5-min watchdog** → returns to fastboot. A capture must
  complete within the window.
- **USB is UNPLUGGED during the walk** (untethered). So the capture MUST run over **Wi-Fi**,
  not USB (172.16.42.1). USB-NCM is also flaky on macOS after replug. USB is only for the
  reboot-from-fastboot + deploy while stationary.
- **Wi-Fi** comes up ~2 min after boot (cnss-daemon autostarts — it DOES work, just slow).
  The headset's DHCP IP on the eero varies (<LAN_SUBNET>.x, seen .35). Get it from the eero
  app or `ip addr show wlan0` over USB *before* unplugging.
- **Recommended method:** stream `record-sensors.py` stdout **directly to the host over
  Wi-Fi** (`ssh root@<wlan-ip> 'record-sensors.py --seconds N' > walk.bin`), launched as a
  background process so you can confirm it's live and cue the walk — nothing in `/run` to
  lose. Alternatively record to persistent (non-`/run`) storage and pull after. **Confirm
  the stream is actually flowing before asking the wearer to move.**

## Artifacts
- `kali:~/quest-camera-work/` — `vio-record-01.bin`, `replay-vit`(+.cpp), calibration
  JSONs, `unified-config.toml`, the `replay-cal`/`replay-v1` calibrated-prepare experiments.
- `kali:~/quest-vio-basalt/` — Basalt build (`build/libbasalt.so`).
- `tools/tracking/` (repo) — `capture-imu.c`, `record-sensors.py`, `prepare-replay.py`,
  `replay-vit.cpp`. `tmp/capture-imu-arm64` (Mac) — cross-built aarch64 binary.
