# Tracking diagnostics: recordings, replay and bounded live VIT

These tools do not install a positional-tracking runtime. The accepted headset
scene still uses orientation-only Monado tracking. Unit calibration, maps, room
images, recordings, CSV results and binaries stay outside this repository.

## Current four-camera investigation

The installed scene now displays only one composite wall left of the cube. It does
not reduce tracking to one camera. The experimental VIT path accepts physical
cameras0,2,1,3 at320×240; `QUEST_VIO_CAMERA_HZ=30` selects30Hz (default remains10Hz
for four cameras). The current trial launcher uses all four big cores (`taskset -c 4-7`)
with nice10; a controlled recorded-input comparison showed the old4,5 restriction
caused substantial queue latency. `QUEST_VIO_TIMINGS` writes a fresh per-stage timing CSV.
The worker allows at most two submitted image groups without returned poses. Under
load it skips new camera groups while continuing IMU feed, then selects a fresh
image. This avoids blocking image submission starving the same-thread IMU producer.
Always retain an external timeout for native diagnostics; the in-process deadline
cannot interrupt a blocked library call.

Basalt patch0005 skips completely masked detection cells with identical detected
corners in regression. Native30Hz processing passed601/601 groups over20seconds.
Patch0006 corrects each camera's essential matrix: upstream incorrectly reused the
camera0/1 transform for every camera. `test-camera-epipolar.cpp` checks the actual
constructor against a synthetic non-collinear rig; old code fails, corrected code
passes. Recorded marked motion improves, but static and wearer validation remain.
Do not apply all patches blindly:0002 and0004 remain rejected/unaccepted experiments.
See the current [tracking checkpoint](../../docs/basalt-positional-tracking.md).

## Original two-camera native path

`live-vit.cpp` passively opens the already-running SyncBoss IMU stream and reads
atomic camera publications from the existing camera service. It does not issue
sensor commands. The lower camera pair (physical0/2) is rectified at320×240 and
submitted at about15Hz; every received IMU sample is submitted. The tracker runs
in a separate process, never on Monado's working orientation thread.

A one-second warmup estimates the minimum host-arrival-minus-MCU-time offset.
This is approximate transport alignment, **not calibrated exposure timing**. The
output is a diagnostic IMU-origin pose, not a head-origin pose ready for an OpenXR
consumer. It needs frame alignment and the owner IMU-to-head lever arm before use.

`build-live-map.py OWNER_CAMERA_CALIBRATION OUTPUT.bin` generates the private
fixed-point remap. Its sampler was checked against the independent saved OpenCV
rectification: both lower-camera images matched pixel-for-pixel on the tested frame.
`test-live-map.py MAP RAW_EVENTS RECTIFIED_EVENTS` repeats that comparison.

Build `live-vit.cpp` against the same VIT headers and ARM musl `libbasalt` as
`replay-vit.cpp`; link `-lbasalt -pthread`, with C++17 and warnings as errors.
Use `aarch64-musl-toolchain.cmake` / `build-opencv-headless.sh` for native dependencies.
`bundle-native-replay.py` audits ARM ELF dependencies into a new private directory;
it does not install libraries. Source OpenCV version tested:4.13.0, core/imgproc/
features2d/flann only, without OpenGL/OpenCL/LAPACK/GUI dependencies.

The development headset's temporary bundle lives at `/run/quest-vio-lean`.
It vanishes at reboot. A representative bounded invocation, **only after** the
existing camera service and guard are healthy and private assets are staged:

```sh
timeout 35 nice -n 10 taskset -c 4,5 \
  /run/quest-vio-lean/lib/libc.musl-aarch64.so.1 \
  --library-path /run/quest-vio-lean/lib \
  /run/quest-vio-lean/live-vit-v2 \
  /run/quest-vio-lean/check.conf \
  /run/quest-vio-lean/live-map.bin \
  /run/quest-camera-root/tmp/camera-feed \
  /run/quest-vio-lean/NEW-UNUSED-OUTPUT.csv 20
```

The config selects the matching two-camera calibration, float VIT and two worker
threads. Duration is limited to1–180seconds; always retain an outer timeout because
upstream queue push/teardown can block if input fails. A successful exit requires
one pose per submitted camera pair. A failed run can leave a partial CSV; require
exit0 and its completion count, not just file existence. The tool refuses to
truncate an existing output file. The hardware/recovery watchdogs are independent.

`driver_timestamp_age_ns` is age at the time the pose is polled, measured from the
camera driver timestamp. It includes capture delivery, rectification, queuing and
compute; it is **not** photon-to-display latency. CSV positions/quaternions are
Basalt's IMU pose in its gravity-aligned world; there is no relocalization/quality
flag or production tracking-loss contract yet. No position is published to the
renderer or Monado.

## Replay timing

Default replay retains historical33ms-per-camera pacing for comparisons. It batches
IMU between cameras, so its timing is not representative of live input delivery.
Set `QUEST_REPLAY_REALTIME=1` for absolute timestamp scheduling of every event,
including IMU. Set `QUEST_REPLAY_TIMING=/private/new-file.csv` to record
submission-to-poll age and outstanding camera groups. This excludes sensor transport
and image rectification; distinguish it from the live probe's driver timestamp age.

On the tested headset with the scene active,30Hz timestamp-paced lower-stereo replay
accumulated about290ms median age.15Hz images with all IMU retained measured40ms
median/66ms maximum and no outstanding later camera groups. Those are workload-
specific results. Do not infer performance from the old paced loop's wall time alone.

## Validation and experiments

- `test-replay-input.py`: malformed event streams and image-mask metadata.
- `test-live-input.py`: malformed maps rejected before sensors/tracker are opened.
- `test-live-map.py`: fixed-point sampler vs saved offline rectified real imagery.
- `select-replay-cameras.py`: permutes/subsets images and their matching calibration.
- `rectification-masks.py`: optional black-border mask experiment, not the default.
- `summarize-trajectory.py`: displacement statistics, explicitly not ground truth.

Basalt patches0001/0003 are the accepted build/EOS fixes.0002 and0004 are experiments
and are reverted in the active build; do not apply the directory wholesale. Ranked
triangulation0004 reduced some jumps in reordered-camera replay but worsened the
original-order motion replay, so it is not promoted.

Latest evidence and next work: [tracking handoff](../../docs/basalt-positional-tracking.md)
and [status/TODO](../../docs/tutorial/09-status-and-next.md).

## Experimental mailbox and positional adapter

`QUEST_VIO_MAILBOX=/absolute/new/path` adds a128-byte atomic mailbox to the live
probe. The path must not already exist. Without `QUEST_VIO_HEAD_OFFSET_FILE`, ready stays
zero. With an explicit private offset file, the experimental `vio-quality.h` gate
can mark supported, initialized poses ready; the installed default scene does not
consume them. Keep it in
private temporary storage. A reader must check freshness, quality and transforms;
file existence alone never means usable tracking. Publication uses a new mode0600
file and rename, without per-frame disk sync (intended for `/run` tmpfs).

The source headers under `patches/monado/runtime-src` are experimental helpers,
also embedded in optional Monado patch0003. Its file reader runs on a separate
polling thread, enabled only by `MONTEREY_VIO_POSE_FILE`.
Geometry adapter holds the last position on stale/lost input and reanchors after
restart; it does not predict position or alter the existing orientation provider.
Its single quiet angular-speed check does not replace a proper startup quality gate.

Local checks (also cross-compiled and passed on the headset):

```sh
cc -std=c11 -O2 -Wall -Wextra -Werror tools/tracking/test-vio-adapter.c -lm -o /tmp/test-vio-adapter
/tmp/test-vio-adapter
cc -std=c11 -O2 -Wall -Wextra -Werror tools/tracking/test-vio-mailbox.c -lm -o /tmp/test-vio-mailbox
/tmp/test-vio-mailbox
```

`extract-head-offset.py OWNER_IMU_CALIBRATION NEW_OUTPUT.txt` prepares the private
device-origin lever arm under the current identity-rotation convention. Do not
substitute arbitrary zero offsets. `test-vio-quality.c` tests the startup/feature
acceptance gate. Its thresholds are provisional, not absolute-accuracy guarantees.

`run-positional-trial.sh RUNTIME BUNDLE SECONDS` is a bounded developer harness that
stops the accepted scene, starts the isolated runtime/worker/renderer, and restores
the accepted scene on exit. Leave both recovery mechanisms enabled. An optional
`MONTEREY_START_FILE` must be fresh and may be created only after wearer readiness.
The runtime directory needs `usr/bin/monado-service`, `usr/lib/libopenxr_monado.so`
and `monterey-head-mesh-fb`; the bundle additionally needs `live-vit-quality`,
`head-offset.txt`, `check.conf`, `live-map.bin`, and its private library directory.
This is not the default boot service. A successfully created CSV does not establish
that any poses passed quality or reached the view: inspect readiness and renderer
`tracked`/position logs, then obtain wearer acceptance.

`test-vio-runtime-writer.c` is ONLY for an offscreen `--pose-only` integration test.
It generates a synthetic10cm translation then stops. Never connect it to a visible
scene. The tested OpenXR path delivered10cm and held position with tracking cleared
when that writer went stale.

`QUEST_REPLAY_QUALITY=/private/new.csv` enables the same feature/startup gate during
two-camera replay, reporting positive projections, image coverage, shared support
and readiness. It assumes fresh input at each recorded timestamp, so it tests
support/geometry only and cannot establish live latency. Require a complete,
successful replay; a failed run can leave partial quality CSV. Build source in its
repository layout with `vio-quality.h` and `patches/monado/runtime-src` available.


## Four-camera diagnostic (2026-09-30)

`build-live-map.py CALIBRATION NEW_MAP --four` builds physical order **0,2,1,3**.
The live frontend accepts either the old two-camera map or this four-camera map;
its VIT calibration must have the same order and camera count. Four-camera input
is limited to about10Hz, lower-pair input to15Hz. Do not mix calibration/map order.
All four live rectified images matched saved offline OpenCV output pixel-for-pixel
on the regression frame. Malformed count/order/size/weights are rejected before
opening sensors.

The multi-camera gate requires at least two supported camera views (three positive
landmarks across two image cells each), including one stronger view (six across
three cells),12 unique landmarks and shared support, plus the existing startup,
freshness and continuity checks. It does not guarantee position accuracy.
`QUEST_REPLAY_QUALITY` now supports2–4 camera replay; appended CSV fields report
upper logical slots without changing the original columns. Live CSV likewise
appends upper-slot counts and coverage. Logical slots0/1/2/3 correspond to physical
cameras0/2/1/3 in the four-camera bundle.

Owner trial with the lower pair briefly displayed LIVE then mostly HOLD. Four
cameras are an investigation, not yet wearer-accepted tracking. Keep baseline
installed runtime unchanged; see the handoff for the latest measured results.

Brief unsupported intervals up to250ms now preserve established initialization and
world alignment. Unsupported samples remain unready/untracked and hold position;
only a fresh supported sample with bounded displacement resumes tracking. Longer
loss or invalid data still resets initialization/alignment. The synthetic offscreen
writer includes a brief unready interval during its10cm motion to catch accidental
reanchoring; never launch it with a visible renderer.


The bounded trial now supplies `MONTEREY_TRACKING_RESET_REQUEST`. Holding a stick
requests a fresh positional estimator epoch as well as a scene-origin reset. The
launcher preserves its original deadline, archives the prior mailbox and logs each
worker in a separate CSV/log segment. The renderer waits for the replacement to
publish and become tracked. Orientation/runtime/display remain running throughout.
Outside this trial, the button only recenters the scene; it does not restart sensors.

`audit-visual-gyro.py RECTIFIED_EVENTS` independently fits relative camera rotation
from SIFT/essential-matrix correspondences and compares short gyro integrals. It
ranks signed proper axis permutations and coarse clock offsets. This is a read-only
diagnostic with excitation/degeneracy limits, not automatic calibration.

## Experimental position prediction (r6)

r6 uses a new160byte `QVIO002` mailbox carrying estimator velocity. It requires the
matching v2 worker; r5's128byte `QVIO001` worker is incompatible. Keep both bundles
isolated. `MONTEREY_VIO_PREDICT=1` enables bounded constant-velocity render prediction,
including the IMU-to-head lever-arm velocity. It caps horizon at100ms and distance
at15cm. Unsupported or stale data holds the last displayed position; it does not
extend the quality gate or create fresh tracking. This is not full high-rate IMU
propagation. Geometry/mailbox/quality/prediction tests pass on host and headset;
OpenXR offscreen comparison confirms smoother samples and a fixed stale endpoint.
See current handoff for wearable validation status.

## Rejected r6 prediction and offline diagnostics

The wearer rejected the r6 constant-velocity prediction trial as substantially worse.
It remains opt-in and is not installed as the default runtime. Four-core r5 enabled
walking in good light but remains laggy and unreliable in low light. Synthetic
constant-speed tests do not validate real estimator corrections.

Replay and live CSVs now append `vx,vy,vz` (m/s in Basalt world coordinates).
Earlier logs lack these values and cannot reconstruct prediction precisely.
`analyze-prediction.py POSES.csv --latency-ms 65` compares raw position jumps with
predicted corrections when a new measurement arrives. It models the r6 bounds but
omits head lever arm, quality gating, and frame alignment; it reports discontinuities,
not accuracy or a reconstruction of the failed wearer trial. Compare multiple fixed
latencies and use the original recording as evidence, without committing private data.
`summarize-trajectory.py` accepts both the original and velocity-extended replay CSV.

Future trial logs include `manifest.txt`: requested prediction/camera rate, CPU
placement, selected numeric camera settings from configuration, and artifact hashes.
The manifest explicitly labels configured camera settings as unconfirmed running state.
It does not copy calibration contents or dump the environment. Keep it alongside the
CSV and wearer observation so filter changes cannot silently confound a prediction
comparison. Older staged launchers do not contain this logging yet.

## Repeatable recorded-input comparisons

For a private replay config with `deterministic=1`, set `QUEST_REPLAY_BRACKET_IMU=1`.
Deterministic Basalt waits for a pose inside the last camera push, so submitting
that image before its next IMU sample can deadlock a single-threaded feeder. This
option delays submission of one image group until the first strictly later IMU
sample has been submitted. Sensor timestamps and per-stream order are unchanged.
Recordings without that bracket are rejected before creating the tracker. Keep an
external timeout; this is an offline tool, not a live scheduler change. Default
replay behavior remains unchanged when the option is absent.

Two451-pose static and two1798-pose movement replays with the saved0005+0006
library produced byte-identical CSV output in this mode. Verify repetition on the input being compared, and keep
feature-extension flags, calibration, library and settings identical. This does not
make recorded scale ground truth or turn host elapsed time into native latency.

`test-replay-bracket.py VIT_INCLUDE_DIRECTORY` compiles the real feeder against a
fake tracker and verifies normal/bracket order with two/four cameras, strict-future
brackets, payload lifetime and trailing IMU cutoff. This is a scheduling regression,
not an estimator test. Basalt's bounded IMU queue can still block on long initial
IMU-only prefixes/equal-time bursts; retain external timeout. Current private
recordings have short prefixes and complete. Do not infer support for arbitrary
camera-time offsets or malformed sensor streams from these comparisons.
