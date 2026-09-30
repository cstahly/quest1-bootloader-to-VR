# Basalt 6DoF positional tracking — state + findings

## Active continuation — visual matching investigation

Current pending physical question is now **60-second precise repeat readiness**,
with two marked spots about 50 cm apart and a marked return location. Previous
readiness questions below are completed. Wait for actual Ready before capture.

Motion03 completed: `vio-motion-03.bin` (901 camera groups, 32,491 IMU samples),
prepared and rectified in `vio-motion-replay-03` / `vio-motion-rectified-03` on Kali.
Unchanged selected settings (grid25, levels2, recovered-distance-squared1.0,
original IMU/noise) produce 901 timestamp-matched poses: peak excursion 0.5016 m,
endpoint offset 0.2855 m. X progresses about 0.014 m at10s, 0.234 at15s, 0.487 at20s,
0.416 at25s, 0.276 at30s. Owner was *trying* to return to the starting point and
offered more precision; do not label the endpoint offset pure tracking drift.
IMU gyro norm median is 3.46 deg/s over27–30s versus1.02 over0–3s, supporting that
the end was not a clean stationary reference.

The same selected setup on the original stationary recording completes451poses:
after excluding the first3s, endpoint displacement4.9mm and maximum excursion24.1mm.
All these results are offline; no positional-tracking code has been deployed.

Latest: second motion recording saved (`vio-motion-02.bin`, 601 camera groups,
22,339 IMU samples); sampled cameras 1/3 are clear. Owner says they did not move
very far, so **50 cm is not ground truth for either existing motion clip**.

Offline `rectify-replay.py` resamples each physical camera to a common virtual
pinhole orientation, retaining camera centres and timestamps. First-frame visual
inspection shows coherent, aligned room views. Valid source coverage per camera is
about 98.0/76.4/98.3/73.1 percent. This transform is not deployed on the headset.
Using original IMU/noise values on motion02:

| Images/settings | Endpoint offset | Maximum excursion | Maximum shared landmarks |
|---|---:|---:|---:|
| Original fisheye, defaults | 20.8246 m | 20.8246 m | 0 |
| Rectified, defaults | 0.3259 m | 1.3786 m | 1 |
| Rectified, grid 25, pyramid levels 2 | 0.0357 m | 0.1940 m | 3 |
| Above, recovered-distance-squared threshold 1.0 | 0.0252 m | 0.1344 m | 5 |
| Above, fixed-depth guess + 20 iterations | 0.0501 m | 0.1251 m | 4 |

These isolate an image-matching problem beyond accelerometer weighting. They do
not establish accurate metric movement; landmark support remains sparse. Testing
the same rectified/grid25/levels2/consistency1 setup on the stationary clip is in
progress. Private configs/results are under `kali:~/quest-camera-work/` in
`vio-motion-rectified-02`, `matching-density-audit`, and `vio-static-rectified-01`.

The **current** pending question asks readiness for a 30-second larger out-and-back
capture between two spots roughly 50 cm apart, cameras clear, pausing at each spot.
Wait for actual Ready, then start and verify Wi-Fi capture before cueing movement.
Earlier readiness questions below have already been answered and are historical.

Update: the experimental patch-rotation initialization produced the exact same
stationary trajectory/feature counts (still no shared-camera landmarks). It was
reversed in the host checkout and the library rebuilt; the saved patch is a failed
experiment, not an accepted change. Only the null-sentinel fix remains applied.

First real movement capture succeeded: `vio-motion-01.bin`, 600 camera publications
and 22,340 IMU samples. Contact-sheet inspection confirmed fingers substantially
occluded cameras 1 and 3; cameras 0 and 2 retained room views. Do not use that clip
as clean validation. Its default replay nonetheless exits 0 with 600 poses, peak
excursion 0.456 m and endpoint offset 0.369 m; accel-noise 3.0 reduces peak motion
to 0.235 m with endpoint offset 0.225 m. Lower stationary drift is therefore not a
justification to ship that weighting. A repeat with all cameras clear was requested;
owner replied Ready, and `vio-motion-02.bin` capture has been started and cued.

- `replay-vit.cpp` accepts an optional FEATURES.csv output. Counts come from
  Basalt's pose-feature extension; the implementation exports inverse depth in
  its `depth` member. The unchanged baseline has median per-camera landmark counts
  17/3/2/8, median 29 unique landmarks, and **zero shared-camera landmarks in all
  451 poses**. This is not evidence of working stereo constraints.
- Independent SIFT correspondence check on the first recorded four views finds
  19/17/9/8/8/13 ratio matches across the six camera pairs. Comparing all 24 factory
  camera assignments favors the existing 0/1/2/3 order: 43 matches below normalized
  epipolar residual 0.01, versus 16 for the next assignment. This is supporting
  evidence, not calibration acceptance. No calibration order was changed.
- Basalt frame-to-frame matching predicts translation from calibration but resets
  patch rotation to identity. An **experimental, host-only** patch initializes
  cross-camera patch rotation from the local calibrated projection Jacobian.
  `patches/basalt/0002-experimental-calibrated-patch-rotation.patch` is being tested;
  do not treat it as accepted or deploy it until replay results are recorded.
- Device was found in fastboot with slot B unbootable/retry count 0. Followed the
  existing recovery procedure: `set_active b`, then normal reboot. No images were
  flashed. USB SSH and Wi-Fi <HEADSET_WIFI_IP> are reachable again. Recovery guard remains
  active and is explicitly renewed during work. Camera service was stopped despite
  its existing default-runlevel link; starting that service restored fresh frames.
- Reinstalled passive record tools in `/run`. **Wi-Fi preflight passed:** 60 camera
  publications and 4,062 IMU samples, local private `vio-wifi-preflight.bin`.
- Pending user question: readiness for a 20-second handheld movement recording,
  moving about half a metre sideways and back while facing the room. **Wait for the
  actual reply, then start/verify recording and cue movement.** No walking required.
  The accepted scene remains running; positional tracking is not deployed.

## Current verified checkpoint — replay completion fixed, 2026-09-30

Host-only work; no headset changes in this checkpoint. Basalt base commit:
`df6e970c8da7636eb401a09e3317fbeaaf829b9a` (mateosss fork, float build).

Two concrete bugs were addressed:

1. The recorder includes about two seconds of IMU after the final camera group.
   Basalt's estimator consumes IMU as images arrive, with a 300-sample queue.
   Replay blocked filling that queue after its last image. The replay now validates
   the complete input first and feeds only the first IMU sample beyond the final
   image, enough to bracket its integration interval. This recording omits 2,037
   unused tail samples. The original recording is preserved.
2. Basalt queues a null state as its completion sentinel, but the VIT wrapper
   previously made a pose object from it and dereferenced it. Apply
   `patches/basalt/0001-vit-null-end-of-stream-pose.patch` and rebuild the library.

`tools/tracking/replay-vit.cpp` now stops the tracker, drains its remaining poses,
and closes output cleanly. It writes to `.csv.partial` until successful completion;
the final CSV is renamed into place only after checks pass. The strict replay
requires one pose per camera group with matching timestamps, so dropped output is
an explicit failure. This is a validation harness, not a general live tracker.

Validation: warnings-as-errors host compilation; six malformed-input checks in
`tools/tracking/test-replay-input.py`; full unchanged-baseline replay exits 0 with
451 camera groups and 451 poses. All four comparison runs below exited 0, yielded
451 finite complete CSV rows, and covered exactly 15.000259 seconds:

| Experiment | accel noise | gyro noise | Endpoint displacement |
|---|---:|---:|---:|
| Unchanged baseline | 0.03 | 0.003 | 14.3821 m |
| Accel weighting only | 3.0 | 0.003 | 0.01216 m |
| Gyro weighting only | 0.03 | 0.3 | 14.0716 m |
| Both weightings | 3.0 | 0.3 | 0.04664 m |

These are stationary-recording results, not physical movement acceptance. They
support investigating accelerometer weighting/calibration, but do not establish
camera calibration correctness, metric scale, or acceptable behavior during motion.
Do not deploy the high-noise setting as a validated tracking fix.

Private host outputs: `kali:~/quest-camera-work/replay-audit-fixed/` (three isolated
variants), `vio-replay-01/poses-fixed.csv` and `replay-fixed.log` (baseline).
`replay-vit-checked` adds strict per-frame timestamp/count checks; canonical source
is `tools/tracking/replay-vit.cpp`. Old `replay-vit` is the original broken harness.

Next: inspect actual visual constraints/features and confirm camera/IMU conventions;
then obtain a controlled motion recording only after the capture stream is verified
ready. No physical question is pending. Positional tracking is not deployed.

## Audit correction — 2026-09-30, resumed primary agent

The earlier interpretations below are historical hypotheses, **not established
diagnoses**. Read this correction first. Direct inspection of the saved Kali files
found:

- Basalt source checkout is clean and the host library built successfully. The
  canonical repository is `/Volumes/vela/src/quest1-nura-port`.
- The experiment described below as "accel noise ×100" actually changed **both**
  `accel_noise_std` from 0.03 to 3.0 **and** `gyro_noise_std` from 0.003 to 0.3.
  It therefore does not isolate accelerometer trust as the cause.
- Every saved trajectory examined has one incomplete final CSV row. Complete rows
  cover different durations. Replay completion/queue draining must be fixed and
  comparisons made over a common interval before further parameter sweeps.
- Recomputed endpoint displacement from complete rows (not a single coordinate):
  original `vio-replay-01/poses.csv`: 14.2502 m / 448 rows / 14.900253 s;
  `replay/trajectory.csv`: 14.2027 m / 448 rows / 14.900253 s;
  `replay/traj_distrust.csv`: 0.0884 m / 418 rows / 13.9002295 s;
  `replay-cal/traj.csv`: 51.9280 m / 413 rows / 13.733556 s;
  `replay-v1/traj.csv`: 18.9088 m / 408 rows / 13.566893 s.
- Stationary hold alone proves neither camera calibration, visual feature
  constraints, metric scale, motion tracking, nor that the accelerometer is the
  only incorrect input. Check feature/depth support and coordinate/timestamp
  conventions before drawing those conclusions.
- Factory IMU/camera calibration was originally extracted read-only from the
  owner's **private partition backup**, not from the stock system image. Originals
  must remain untouched; private values and room recordings stay outside Git.
- The current device state has not been established: read-only SSH attempts to
  USB 172.16.42.1 and saved Wi-Fi <HEADSET_WIFI_IP> both timed out. No reboot, service
  restart, calibration deployment, or partition write was performed in this audit.

Next: repair and instrument host replay termination, reproduce the unchanged
baseline, and separate gyro/accel-noise experiments. A controlled motion recording
is still required for physical 6DoF validation, after capture readiness is verified.

## Earlier handoff (interpretations superseded where corrected above)

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
