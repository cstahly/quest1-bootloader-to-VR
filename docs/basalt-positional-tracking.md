# Basalt 6DoF positional tracking — state + findings

## Final handoff — 2026-10-01 (supersedes historical active-state wording)

Owner requested knowledge dump, commit/push and stop. Device was explicitly returned
with `/usr/sbin/reboot-mode bootloader`; Mac `fastboot devices` verified
`<SERIAL> fastboot`. USB-connected fastboot is the parked state, not poweroff.
No candidate was installed persistently. All three investigation agents are stopped;
no guard-renewal process or autonomous follow-up is intended. Prediction is disabled.

Pending isolated test: `CAMERA_CPU_INVALIDATE=1`, original exposure policy (scene-only
and ack modes off), 90 seconds, full-frame diagnostics. Prepared launcher on Mac:
`/Users/<user>/work/quest-pmos-bringup/camera-work/run-cpu-invalidate-bench.sh`.
Candidate on Kali: `/home/<user>/quest-camera-work/cpu-invalidate.urbUNy/stock-camera-cpu-invalidate.so`;
Mac copy: `/Users/<user>/work/quest-pmos-bringup/camera-work/stock-camera-cpu-invalidate.so`.
SHA256 `9da5407ac8b01aced9713892b7a39b6e1b0541b29521fb9484ae72b99318f86b`.
Not staged/run on device. Host ABI/bounds and ASan/UBSan checks, shell syntax and
Bionic `-Wall -Wextra -Werror` build passed during development.

Implementation: private `/dev/ion` client, outer `ION_IOC_CUSTOM=0xc0104906`, inner
`ION_IOC_INV_CACHES=0xc0184d01`, 24-byte flush / 16-byte custom AArch64 structs.
Invalidate all 311296 mapped bytes before metadata/pixel reads; payload is 307840.
`raw[-1]` points at private wrapper; offsets 0x228 fd, 0x230 mapping, 0x238 length.
`raw[6]` is data pointer; low 32 bits of `raw[7]` are DMA fd, NOT length. Exact owner
HAL hash, fd/pointer/length identity, alignment and overflow are gated. EINTR retries;
other errors return the owned buffer, clean up and fail closed. Generic ION_SYNC
is not a replacement: the exact kernel's path synchronizes toward the device.

Kernel is `4.4.205-perf`, source commit `6929f734ce0e602018790ff3a52dc7bad646af60`.
The stock CPU gralloc lock path supports this operation, but its effectiveness on
our capture pipeline remains untested. Historical statements that a scene bank
physically refused long exposure are retracted pending trustworthy CPU readback.
Historical complete-image repetition analysis did not demonstrate frozen full images;
that does not rule out independently stale metadata/cache lines.

This commit preserves existing experimental source, including r8 Monado packaging,
raw capture tools and unaccepted Basalt variants. Committed does not mean installed
or validated for motion. Private asset locations/hashes are in [assets inventory](assets-inventory.md).
Token-management preference: single-agent, bounded hypotheses, targeted reads, usage
checks around substantial batches. No further investigation is authorized by this
final documentation step.

## CPU invalidation experiment — 2026-10-01, built but not run

After the usage interruption, the headset was found in fastboot (slotB bootable).
Parent issued normal reboot; no flash or slot change. Built opt-in
CAMERA_CPU_INVALIDATE=1 using the verified MSM ION custom CPU-invalidate ABI.
Each owned HAL buffer is identity/length/alignment checked, then invalidated through
a private ION client before any metadata or image read. Kernel imports/releases
that DMA fd for the call. Unknown HAL hash, invalid layout, or ioctl failure stops
capture. This does not use generic ION_SYNC or change sensor firmware.

Host ABI/bounds tests and Bionic-Werror build pass. Private Kali build directory
cpu-invalidate.urbUNy; binary SHA2569da5407ac8b01aced9713892b7a39b6e1b0541b29521fb9484ae72b99318f86b.
First planned90s RAM bench uses ORIGINAL exposure policy (scene-only/ack modes off)
plus invalidation and diagnostics to isolate coherency. All-raw-frame metadata
counter matches/mismatches are aggregated beyond the10s trace window. Hardware
validation is pending; do not call this a demonstrated fix yet.

## Critical diagnosis correction — stale camera metadata, 2026-10-01

STOP interpreting prior requested-versus-readback mismatches as proven MCU or sensor
failures. Staged01 shows frozen sensor counters in returned DMA buffers while the
HAL driver sequence and timestamps advance: affected camera3 phase holds metadata
counter64/tagbyte241; camera2 holds counter140/tagbyte81. Different metadata fields
can update independently. This is strong stale-buffer evidence; coherency is under
investigation. All target acknowledgments currently inspect these same potentially
stale bytes, so more exposure-policy tuning is paused until reads are trustworthy.

Independent binary tracing found cached ION allocation and a DQBUF cache-op path
whose behavior depends on stream mode/callbacks. The null start argument is an
external-FD-array option, not a missing cache callback. Exact stock CPU invalidation
requirements are being traced; no guessed ioctls or cache operations have been run.
Scene/controller packet/tag distinctions remain supported by stock binary evidence,
but earlier claims that a bank physically refused a long exposure are unproven.

No persistent changes. The90s staged bench restored original camera/scene services;
parent continues manual recovery-guard renewals. Headset remains on baseline while
three agents audit cache handling, stale metadata, and recorded pixel repetition.

## Latest active checkpoint — controller-first startup, 2026-10-01

The first target-ack hardware run FAILED safely and is not installed. In
scene-ack-01, camera2 produced38µs/gain48/tag1 on essentially every raw frame,
with no valid controller tag2. Other cameras alternated normally. Publication
remained zero and the scene target timed out; local command-write success did not
prove that the controller-bank configuration applied. The90s harness restored
original camera and scene services. Parent checked the original live feed afterward:
all four fresh timestamps within12µs, all gains240, hardware watchdog enabled.

A staged candidate is now being implemented/reviewed: initialize and verify the
controller bank first with bounded same-target retries, then initialize scene
settings and run target-confirmed automatic exposure. Quarantine publication until
initial/recovery scene confirmation. Keep publication flowing during later ordinary
AE changes; do not introduce170ms gaps for every target. Explicit class1 metadata
must also fit the configured scene envelope (981..12019µs, gain16..240), which
rejects malformed class1/38µs while retaining valid1000→988µs quantization.

Controller-first sequencing also removes back-to-back bank writes; their role in
the partial application is a hypothesis, not a proven hardware cause. No firmware,
bootloader or persistent runtime changes have been made. The device is on its normal
baseline while the next RAM-only candidate builds; guard is manually renewed.

## Scene-bank correction bench — 2026-10-01, latest active state

No persistent camera/Monado changes yet; baseline services restored between tests.
Independent review and host tests passed for opt-in CAMERA_SCENE_BANK_ONLY=1.
Bank0 gets scene settings/tag1; bank1 restored once to stock38µs/gain48/tag2.
Explicit class selection replaces exposure/parity guesses in this opt-in mode.
Unknown owner HAL hash or local command-write error fails closed. Automatic control
requires a fresh synchronized quartet after the last request. Combined raw-capture
and scene-only mode is rejected pending raw-format low-exposure semantics.

- scene-bank-01: controller readback38/48 all4; scene cadence~30Hz and30Hz feed.
  Camera3 nevertheless alternated older/newer gains across scene frames in the first
  ten seconds. This is not explainable merely by a one-second diagnostic alias.
- scene-bank-02: repeat has only ordinary one-frame command-transition mismatches;
  all4 scene settings settle7999µs/240. Per-camera raw sequence parity differs within
  the run, reinforcing class/timestamp selection rather than fixed sequence parity.
- scene-fixed-01: auto off, scene8000/gain48. After first10rawframes all observed
  scene frames report7999/48, controller38/48, with no mismatches on any camera.
  Exactly one bank0 and one bank1 command.30s bench restored camera/scene services.
- Private logs named above are in quest-pmos-bringup/camera-work. Candidate binary
  stock-camera-scene-bank.so SHA2562f425038915171f627e1e9ba5770acc109ce274a0379246619adc906a888c429.
- scene-readback-01:90s with aggregate all-frame counters. Final mismatches
  0/12/18/136 for cameras0..3, unchanged after~14s through89s. Thus early partial
  setting application is real, not just one-second sampling; later both phases settle.
  Final matched counts2136/2124/2118/2000;535 transition-excluded each.30Hz persists.
  Counters deliberately exclude captures within100ms of a command.
- Next: bounded target-confirmation control in scene-only mode. Hold each requested
  quartet until two consecutive scene cohorts match it; retry the same target at
  500ms while unconfirmed, instead of calculating new targets from old gain values.
  Unchanged confirmed targets should not cause repeated writes or per-frame AE work.
  Separate helper/tests, integration and independent review in progress. No wearer
  prompts tonight; no motion-quality claim; no persistent changes yet.

## Latest bench checkpoint — 2026-10-01

Parent continued with a normal boot and bounded, RAM-only stationary camera probes;
no wearer interaction, firmware writes, or persistent package changes. The first
probe safely rejected an incorrect buffer-length interpretation (15 was an fd).
The corrected probe validates the known owner HAL SHA256, private buffer mapping
length/pointer/fd, and writes only one explicitly armed four-camera snapshot.

- Second probe completed: full640×480 raw images, all four physical cameras at
  sequence660, timestamp skew14µs. Requested shutter8000µs/gain240; metadata reports
  6118µs/gain240. Startup exposure verification passed. This is evidence of a
  request/readback mismatch, not proof that a specific bank needs rewriting.
- A bounded10s all-frame metadata/command trace was captured before filtering and
  publication. Earlier one-second telemetry showed strong odd/even exposure
  correlation; the new trace is being analyzed to avoid sampling-phase aliasing.
- Private evidence: quest-pmos-bringup/camera-work/raw-cohort-kbgcpj.bin,
  raw-trace-kbgcpj.log, and raw-trace-kbgcpj-bench.log. No room images in this repo.
- The50s probe exited successfully and restored original camera and scene services;
  both report started. Independent watchdog remains enabled. Active bench session
  currently running; final device state must be recorded when investigation stops.
- Deterministic FAST min5→2, guessed camera-time shifts, and zero-bias IMU position
  propagation did not show reliable improvement and were rejected. Production
  prediction remains disabled. r8 APK is built/tested but not installed.
- Follow-up20s RAM-only bank identification passed: bank0 requested4000µs,
  bank1 requested2000µs, both gain16; all four cameras reported4009/1995µs on
  alternating frames. Both original services restored. Sequence parity identifies
  phases only within this run; earlier restarts changed which parity followed the
  longer request. Next bounded step must retain phase identity within one stream.
- Same-cohort filter audit used actual C filter and deployed integer remap. FAST5
  detections133→1, but unfiltered stereo descriptor corroboration was zero and raw
  medians5–6/255. This is not evidence that filtering destroyed useful landmarks;
  no denoise rollback justified. Private raw-filter-audit-kbgcpj contains metrics.
- Native ARM worker with bounded brief-stale initialization retention built and
  archived privately as live-stale-retention-FAYlKuWd. Host quality, mailbox,
  adapter/prediction and10 malformed-input cases pass. ARM ELF/dependencies checked;
  not executed or installed. It uses QVIO002 and must not replace an r5 QVIO001
  worker in isolation. Existing linked Basalt build may contain opt-in0007; archive
  includes no Basalt library, so runtime candidate selection remains explicit.
- Same-stream step now proves bank identity: after bank0=4000/bank1=2000,
  requesting8000 from both changes bank0 to7999 but leaves bank1 at1995 for the
  remaining~7s. This is bank1 failing to apply the long request, not a universal
  shutter clamp. Private bank-step-01.log records both writes and all-frame readback.
- Stock caller/table audit identifies scene bank0/tag1 and controller bank1/tag2,
  with controller startup38µs/gain48. An opt-in RAM-only candidate is being built
  to restore that distinction and select scene-class metadata rather than parity
  or the old exposure<1000 heuristic. No persistent camera change yet.
- Separate code audit found auto-exposure could inspect a mixed/stale latest-frame
  set outside the publication cohort gate. Candidate will require synchronized,
  fresh, post-request frames before changing automatic exposure.

## Second autonomous pass — 2026-10-01, active offline work

Owner challenged premature stopping after first audits. Continued all three agents
and parent independently. This section describes the earlier offline phase; see
the newer stationary bench checkpoint above for subsequent headset access.

- Prediction helper now uses pure queries and sensor/update-time HOLD. Explicit
  loss freezes at first observed loss time; stale data holds the capped endpoint;
  reanchor retains predicted continuity while raw baseline coordinates remain.
  Host regressions cover history/future/no-query permutations, bounds, repeated loss,
  restart and200 rotation cases. Parent integrated both external invalidation paths
  through mv_deactivate; patch regenerated/pkgrel8/checksum. Cross-build passed22/22
  tests; r8 APK/log saved privately on Mac, not deployed.
- Root added opt-in QUEST_REPLAY_BRACKET_IMU=1 to recorded-input feeder. Supplies
  one future IMU sample before deterministic Basalt's blocking camera submission.
  Default replay unchanged.15input validation cases pass; static451poses repeated
  byte-identically. Marked1798poses also byte-identical across two completed runs. Private directory
  Kali camera-work/deterministic-audit-dl0hpbpi, binary replay-vit-bracket, library
  basalt-epipolar-only/libbasalt.so.2. Never use unaccepted0007 by accident.
- Exposure common-shutter optimization simulator was REJECTED: better clipping at
  cost of useful dark views, oscillation, and noisy darkness masquerading as texture.
  Production policy unchanged. Lowlight agent now compares only FAST min5→2 with
  deterministic input/identical feature extension on static and marked recordings.
- Timing agent traced saved libraries direct DQBUF timestamp copies and available
  normal kernel path to VFE completion IRQ time, not photon midpoint. Exact deployed
  path/flags unresolved. New SO(3)+calibrated-camera rotation audit running on saved
  images, with endpoint interpolation and synthetic tests; do not apply a guessed
  exposure correction based only on EOF arithmetic.

No persistent package, camera binary, policy or settings changes in this phase.
The later RAM-only bench phase is recorded above.

## Autonomous parallel investigation — 2026-09-30

Owner explicitly authorized autonomous investigation and agent orchestration after
rejecting the prediction trial. Three bounded agents audited prediction geometry,
camera/IMU timing, and low-light capture while the parent reviewed/tests/integrated.
No agent contacted the headset. It remains in USB Update Mode; no physical prompts.

- [Prediction audit](basalt-positional-tracking.md#archived-prediction-audit): found/fixed a real lever-arm
  derivative inconsistency. Stationary-head synthetic rotations previously introduced
  up to6.92mm translation in the tested cases;200/200 corrected cases pass, with160
  failures using the old derivative. This small geometric correction does not explain
  the entire wearer regression. Separate historical-query/HOLD dependence remains
  reproduced but unresolved; current renderer queries now only. Prediction stays off.
- [Timing audit](basalt-positional-tracking.md#archived-timing-audit): raw marked recording shows~1.64ms
  transport-fit clock drift across62s, not evidence of a large drift problem. Fixed
  exposure/IMU offset remains unknown. No timing correction applied.
- [Low-light audit](CAMERA-ROADMAP.md#archived-lowlight-audit): shared shutter has conflicting
  bright/dark constraints, a possible1000us quantization/classification boundary needs
  readback verification, and denoise/prediction were confounded inr6. Policy unchanged.
  Added CAMERA_DIAGNOSTICS=1 (default off), capped at one five-line group/sec, reporting
  requested/observed exposure, startup verification, p70/clipping, discarded shorts and
  publications. Statistics tests and isolated ARM64 Bionic build pass. Not deployed.

Parent regenerated patch0003/checksum and bumped experimental APK tor7; full package
cross-build completed successfully on2026-10-01; all22/22 package tests pass. APK
and build log are saved privately in camera-work/ as monado-oculus-monterey-25.1.0_git20260822-r7.apk
and monado-r7-build.log. Camera diagnostics binary is stock-camera-diagnostics-offline.so.
These are OFFLINE artifacts, not installed or wearer-accepted.
Host adapter/mailbox/prediction/rotation/quality checks pass;
camera statistics/resample/exposure/cohort/characterization checks pass. The new timing
analysis passes7 tests with a working NumPy interpreter (see audit for local command).
Future launcher writes a restricted manifest of settings and artifact hashes before
stopping the baseline; secret/environment dump excluded. Shell syntax/privacy check
passed. None of these source changes were installed; r5 remains the better wearer
candidate and r6 remains rejected. All prior hardware/boot/watchdog restrictions apply.

## Current: r6 prediction REJECTED by wearer; no more physical tests tonight

Owner response to GFdfKL: “no, that's much worse. i'm hands off device for the rest
of the night.” Do not request or launch another wearer test tonight. Trial expired
normally; confirmed installed orientation-only renderer running, no positional worker.
r6 was never installed globally. Preserve four-core r5 as the better tested candidate.
Private evidence: `camera-work/prediction-GFdfKL-complete.tar.gz` under the bring-up
workspace. After reset:3696 poses,3210ready, age median65.21ms/p95111.65/max171.71.
Before reset:1373poses,768ready, age median46.94/p95120.65/max172.22ms.
Renderer ended cleanly after12805frames, near72fps. Higher ready counts and synthetic
smoothness are NOT acceptance: wearer result overrides them. Investigate velocity
and estimator corrections offline. Existing CSV omitted velocity, so the actual
prediction error cannot be reconstructed completely from this trial; add telemetry.
No firmware/boot/default runtime changes. Watchdog remains enabled.

End-of-night device state: explicit guard poweroff succeeded but USB caused a new
`androidboot.mode=charger` boot. Issued documented `reboot-mode bootloader` once;
verified serial in fastboot. Device is NOT fully powered off. No camera/scene processes
run in USB Update Mode. Do not loop shutdown/reboots or ask the hands-off owner to
perform more physical tests. `/run` trial bundles were lost at reboot; private r5 base
and r6 extension archives remain on the Mac. Baseline files remain installed.

Added world-velocity CSV columns to replay/live probes; built host replay and ARM
live probe with warnings as errors. Eleven replay validation cases pass. No updated
probe was deployed. Offline correction analyzer added; constant-motion cancellation,
noisy-velocity amplification, bounds and invalid-clock checks pass.


## Offline follow-up after rejection

Built velocity-logging host replay and ARM worker; ARM binary not deployed. Basalt
source `src/vit/vit_tracker.cpp` returns `vel_w_i`, in the same world frame as position;
no obvious world/body velocity-frame mismatch found. Recorded runs used the saved
0005+0006-only library, not experimental0007. Both completed: marked1798/1798,
static451/451. Private CSVs `prediction-velocity-{marked,static}.csv` on Kali and Mac.

The new analyzer models translation only at fixed delay, without quality gating,
head lever arm or frame alignment. At65ms, marked raw update p95=5.53mm versus
predicted correction p95=5.15mm; median1.07→1.08mm,64% of corrections enlarged.
At100ms, predicted p95=6.73mm and85% enlarged. Static median correction0.612→0.770mm,
p953.98→3.95mm,66% enlarged. These mixed results do NOT establish the cause of the
wearer regression; they show why the perfect constant-speed synthetic test was
insufficient. Static replay itself drifts6.24cm from3s toend. Marked replay is
nondeterministic and this run returned40.8cm from3s reference; no scale acceptance.

Next work, in order:
- Preserve r5 four-core baseline; keep r6 prediction disabled. Reject0007 and tuning
  experiments pending evidence. Do not globally install r6.
- Isolate the capture prefilter from prediction: GFdfKL also used the newly installed
  CAMERA_DENOISE=1, so comparing it with dicDpI does not isolate prediction alone.
  Filter remains provisionally installed; record/set it explicitly for any future
  controlled comparison, inspecting configuration first. Do not claim it rolled back.
- Record velocity plus pose/quality/latency on the next explicitly agreed movement
  check, and model estimator corrections, stop/reversal, lever arm and loss/reacquire.
  Existing GFdfKL positions alone cannot reconstruct what prediction did.
- Improve actual capture/timing/estimator reliability before masking it with display
  smoothing. Inspect camera/IMU exposure-time alignment and low-light feature support.
- No physical prompts tonight. Headset confirmed in USB Update Mode, not powered off.

The sections below are chronological history and may describe superseded pending tests.

## r6 position prediction build + capture prefilter validation

Owner reports dicDpI better in full light, largely able to walk, still slow/poor in
low light. Asked whether stock uses additional sensors. Explained Meta Insight uses
camera+IMU, ours also does; current display lacked render-time position prediction.
Owner previously objected to stopping before independent work was exhausted: KEEP
WORKING without requiring another wearer test to proceed with software validation.

r6 APK built successfully on Kali,22/22 tests passed; NOT installed. Runtime helpers
and patch0003 now use160byte QVIO002 (was128byte QVIO001): added VIT world velocity
and validity. New bounded constant-velocity mv_render_position: max100ms/15cm;
unsupported/stale input freezes last presented position, new epoch preserves it;
lever-arm velocity included. Opt-in MONTEREY_VIO_PREDICT=1. Geometry, mailbox, quality
and new72Hz synthetic prediction tests pass locally. Existing r5 device/restore
bundles still require v1 worker; DO NOT overwrite them with current v2 source build.
New ARM worker live-vit-predict-v2-arm built on Kali. r6 runtime tar built there.
OFFSCREEN synthetic comparison completed: prediction OFF advances20/62 render samples,
ON58/60 during constant-speed ramp. Both finish at10cm; predicted stale tail range0.
863pose samples,326tracked. Saved vio-openxr-offscreen-r6[-control].log privately.
Now running offscreen LIVE20s integration through r6/velocity-v2/0006/fourcores/filter.
No synthetic input is used with a visible scene. r6 files under/run/quest-vio-runtime-r6;
v2 worker/config links under/run/quest-vio-predict. r5/v1 bundle remains unchanged.

Offscreen test preparation details: renderer pose-only now logs every frame (normal
visible logging unchanged); synthetic writer now supplies velocity and50ms latency.
Build outputs monterey-head-predict-test and test-writer-predict are building.

Capture optional CAMERA_DENOISE=1 uses centred3x3 binomial filter before2x decimation
in camera-resample.h. Calibration centres unchanged; constant/impulse/alias/gradient
tests pass. New preload stock-camera-filter.so installed with .pre-filter backup;
config backup /etc/conf.d/oculus-camera.pre-filter. First native run151/151 synchronized
publications in5s, no stale reads, maxskew.026ms. CPU comparison: filter-on28.2% and26.2% of ONE core, off14.8%; all30Hz. Roughly
0.12 additional CPU core, accepted provisionally for noise/alias reduction. Filter
restoredON; tracking-quality improvement still requires motion validation.
Default orientation scene briefly failed startup while headset moving; restart
restored service, guard and hardware watchdog preserved.

Host replay experiments: iterations10 produced1565ready but return error.458m
(vs baseline.130m), reject for now. pyramid3 has partial output and optimizer warnings,
process no longer running, not accepted. None deployed.


## Completed wearer trial dicDpI — observations pending

Owner explicitly requested movement check. First launch BLeofD failed IMU stable
startup (no scene); told owner set down briefly and retried. dicDpI completed
180s, r5/0005+0006/bounded queue/affinity4–7/requested30Hz. Confirmed renderer,
gave GO put on/hold stick1s/start here/normal leaning and turns. Pending question
asks body lag and LIVE reliability. Leave scene unchanged until response or expiry.
Launcher log /run/quest-vio-runtime-r5/fourcores-wearer-retry.log. Save full trial
privately and announce orientation-only restoration on expiry.

Full private archive fourcores-dicDpI-complete.tar.gz saved; baseline restored and
announced. Pending wearer observations. Do not relaunch or change test yet.

Two independent HOST parameter replays (iterations10/pyramid3) launched before
user interrupted to request test; may still run on Kali. These configs are NOT
deployed. Rendering-prediction work only considered, no implementation changes.


## Current handoff boundary: low-latency build staged; physical validation needed

Owner latest: lights off, headset pointed at wall. No physical question pending.
Continue independent work if useful, but do not launch another wearer trial without
Ready. Current accepted/default scene is orientation-only, single wall composite,
automatic exposure. Guard and hardware watchdog remain intact. Positional r5 is
experimental and NOT installed as default.

Best candidate now:0005+0006, bounded two-image worker, all four big cores4–7,
requested30Hz. Private restore archive:
`/Users/<user>/work/quest-pmos-bringup/camera-work/positional-r5-epipolar-fourcores-staging.tar.gz`.
This supersedes earlier archive launchers restricted to4,5. Next wearer trial command:
`QUEST_VIO_CAMERA_HZ=30 /run/quest-vio-runtime-r5/run-trial.sh /run/quest-vio-runtime-r5 /run/quest-vio-four 180`.
Renew guard/verify device first, cue only after scene exists, preserve user's response
time, save logs and announce baseline restore at expiry. Need physical validation of
normal motion with the verified latency fix before accepting tracking. Renderer
still consumes discrete position samples; render-time positional prediction/smoothing
is not implemented. Do not hide unsupported tracking with interpolation or looser gates.

0007 upper-stereo seeding remains UNACCEPTED: static support improves, but marked
movement scale changes and native300group benchmark has long stalls (median38.4ms,
p95 397.7,max579.6ms), despite all four big cores. Do not replace candidate0006 library
with0007. It is isolated under `/run/quest-vio-upper`; private ARM copy saved on Mac.
Kali source/build presently include opt-in0007; env absent preserves base behavior.
For reproducing0006-only builds revert just experimental frame_to_frame header using
saved frame-to-frame-before-pair.h or reverse0007; retain0005 and0006. No blanket reset.

## Latest verified latency fix: use all four performance cores

Controlled native test, SAME first10seconds of marked recording300groups,0005+0006,
same settings/active baseline scene, real-time replay (submission-to-poll age, NOT
sensor/display latency): affinity4,5 median181.35ms,p95 406.65,max459.43; affinity4–7
median36.05ms,p95 65.56,max76.57. Both completed300/300. Earlier lighting-confounded
live comparison is superseded by this controlled result. Trial launcher now uses
4–7 and is staged on headset. Nice10 remains; no overclock/governor changes.
Evidence private cores2-time.csv/cores4-time.csv. Archive needs refresh for new launcher.

Upper-pair experiment0007 is opt-in QUEST_VIO_UPPER_STEREO=1. Seeds empty cells on
logical2 and matches into3, rejects occupied target cells, applies correct upper-pair
epipolar filter. Host marked/static replays completed. Versus0006-only, upper-pair seeding yields
marked ready1552→1628/1798, upper medians[3,7]→[29,6], return .130→.095m, but
20→40s displacement .516→.370m (scale concern; do not call accuracy improvement).
Weak static clip ready0→361/451 and3s→end drift.062→.024m. Building ARM for
private benchmark. Do not apply0007 by default or claim accepted yet.

## Latest owner feedback: still frequently loses lock, body updates lag/stutter

Piigko trial ended and baseline restored; full archive wearer-Piigko-complete.tar.gz.
Owner: a little better, can kinda move, loses lock frequently with quick/almost any
movement; body-motion updates visibly laggy/stutter/redraw. NOT accepted. Segments
poses/poses-1/poses-2:966/2306/737 outputs,550/1073/129 ready; age medians90/108/97ms,
stale>150ms11/19/12. Ready→unready edges total:9 stale,12support,1step. Two explicit
reset restarts worked. Need lower latency AND better feature retention, not looser gates.

Owner is turning room light OFF and leaving headset pointed at wall. Do not ask to
wear it while independent work continues. Current raw images now almost black except
monitor and tiny lit detail; exposure at bounded limit. Do not compare feature counts
across this lighting change as software-only improvements.

Testing worker affinity4–7 vs old4,5: first four-core live run597poses median age33.6ms,
but quality0 after lighting change, so NOT controlled proof of scheduling improvement.
Preparing identical first10s marked replay for native two-vs-four-core comparison.
No new wearer question pending. Baseline orientation-only wall scene remains running.

## Completed wearer trial: Piigko

Owner answered YES/Ready. Started180second r5 trial `/run/quest-positional-trial.Piigko`
with QUEST_VIO_CAMERA_HZ=30; launcher log `/run/quest-vio-runtime-r5/epipolar-wearer.log`.
Confirmed renderer and tracked poses, then gave GO: put on, hold either stick1second,
wait START HERE SET, ordinary leaning/turning. Asked whether LIVE holds and box stays
in place. Leave scene unchanged while owner observes; no repeated physical prompts.
At expiry announce orientation-only baseline restored. Save all trial files privately.

## Prior checkpoint: revised build verified

Integrated r5 trial `hLJcFo` completed30seconds with wall renderer,0005+0006,
automatic common-shutter exposure and bounded two-group camera queue at requested30Hz.
791poses,709quality-ready; median age81.26ms,max142.78ms. Renderer tracked162/176
logged samples; maximum resting displayed displacement7.9mm. Clean stop1760frames;
baseline restored. Private evidence epipolar-integration-hLJcFo.tar.gz.

Asked owner Ready for meaningful normal-movement test. DO NOT launch trial before
reply. On Ready: renew guard, verify/recover Linux if needed, restore current archive
`positional-r5-epipolar-bounded-staging.tar.gz` if /run lost, launch r5 trial with
`QUEST_VIO_CAMERA_HZ=30` for180s. Cue wearer to hold either stick1second for start
here after putting it on, then ordinary leaning/turning. Wait for observations;
leave scene unchanged. Trial restores orientation-only baseline at expiry; announce
that transition. Neither hardware watchdog nor recovery deadline was disabled.

Wall composite + auto exposure are persistent. Positional0006/r5/queue changes stay
isolated in /run, NOT default-installed. All benchmarks so far are stationary or
recorded replay; normal physical movement acceptance is still outstanding.


## Latest checkpoint: automatic exposure installed; four-camera geometry bug found

Single wall composite remains installed and baseline scene runs. Automatic exposure
preload installed with `.pre-auto` backup; config backup `/etc/conf.d/oculus-camera.pre-auto`.
Two service starts verified startup settings on all four cameras,151/151 synchronized
publications in5seconds, no stale reads. Settled common5339us with per-camera gains.
Native optimized auto-exposure30Hz run:601/601 images/poses in20s,20013 IMU samples,
no unsynchronized skips. Upper cameras still had zero positive landmarks at end;
not a wearer-ready tracking claim.

Recovery: slotB retry count exhausted to0/unbootable. Used documented SAME-slot
`set_active b`, normal reboot; Linux and scene recovered, hardware watchdog disable0.
No image flashing. Runtime bundle restored and actual libbasalt.so.2 updated0005.

New concrete bug in upstream `OpticalFlowTyped` constructor: loop over camera i
always computes E[i] using T_i_c[1]. Upper-camera matches therefore get filtered
against the lower stereo pair's epipolar geometry. Correct this independently and
replay before combining feature-seeding changes. A separate opt-in upper-pair seed
experiment built on Kali (saved library basalt-upper-experiment), then was reverted
from source to isolate the geometry correction. No seeding changes deployed.
Synthetic regression test-camera-epipolar.cpp invokes the actual constructor: old
header fails camera2 residual0.116705, corrected header passes all60 correspondences.
Host rebuild completed. Marked before/after: ready969→1552/1798, upper positive
medians[0,4]→[3,7],20→40s displacement.4959→.5162m,20→end.1618→.1300m,
maxstep.0777→.1113m. Separate weak static clip ready0both,3s→end.0100→.0624m:
geometry correction is proven, but overall static accuracy not solved. Nondeterministic
replays are diagnostic, not ground truth. Saved private epipolar-replay-comparison.tar.gz.
ARM build passed. Native0006 at unbounded30Hz stalled after350poses: median age177ms,
max526ms. Explicit TERM stopped diagnostic; baseline unaffected. Both frontend and
backend median~32ms and queues accumulated. Camera push and IMU feed share one
thread; Basalt image queue push blocks. Added max TWO outstanding image groups in
live-vit.cpp, skipping incoming images under pressure while continuing IMU input.
Bounded run with external35s timeout completed566/566 groups,20309 IMUs, no unsync,
682 capacity checks skipped,478/566 quality-ready. Median age66.68ms,max143.64ms;
upper landmark medians[1,3]. This is a resting-view diagnostic, not wearer acceptance.
Private binary live-vit-bounded-arm now staged as trial live-vit-quality. Running
30s integration smoke via r5 launcher with QUEST_VIO_CAMERA_HZ=30; baseline restore
expected. Native0006 library backed up previous as `.pre-epipolar`.
Private restore archive positional-r5-wall-auto-maskskip-staging.tar.gz now includes
wall renderer, profile worker and0005 library (still pre-0006).

## Autonomous investigation after wearer rejection

Owner: only holds position while staring at a very bright light and moving extremely
slowly; ordinary movement loses lock. Explicitly asked to work autonomously until
figured out or actual help is needed. Do not request another unchanged wearer test.
MdeafM complete log was lost when the guard returned the device to fastboot between
turns; partial snapshot is saved as `revised-trial-MdeafM-snapshot.tar.gz`. Normal
boot of existing image restored Linux; no flash or slot changes.

Additional owner request implemented: one composite wall to the left of cube,
removing four standing panels and the floor surface. Only one texture/effect pass
now processes76800pixels instead of four; edge/trail effects moved to composite.
Wall/recenter/HUD/ink native regressions pass. Installed renderer backup `.pre-wall`.
Baseline scene verified running with wall; measured~63.8fps in resting view,
camera draw~3.17ms. No claim of fixed stitching/parallax or wearer acceptance.

Camera cohort guard added: producer publishes only when all four timestamps agree
within1ms. Unit race/completion/skew test passes. Installed preload backed up as
`stock-camera.so.pre-cohort`. Service restart initially left one sensor at different
exposure, so no synchronized publication; an explicit uniform4000us/64 runtime
command restored all four. Capture startup exposure reliability remains unresolved.
Observed91/91 synchronized publications in3seconds, maxskew.021ms thereafter.

New bounded `auto-exposure.c` diagnostic with `exposure-policy.h` prefers4ms exposure,
bounds gain16..240 and exposure1..8ms, central70th-percentile target80. All cameras
must use COMMON exposure duration (different gains allowed): independently varying
exposure caused mixed scene/controller timing, so that experiment is rejected.
Current stationary result common7999us, gains223/98/240/178; brighter imagery.
Not installed as a persistent daemon/config yet. Startup remainsmanual12000/16.

VIT per-stage timing and optional10/15/30Hz added to live probe. Native10Hz profile
showed~21.7ms wasted in other-camera detection, much in fully masked cells. New
Basalt0005 skips entirely masked grid cells before conversion/FAST. Dedicated
12-image fully/partially masked regression produced IDENTICAL corners/responses
before/after. Host and ARM builds passed. Native bundle actually loads libbasalt.so.2
(not .so.2.0.1); updated correct file with `.pre-maskskip` backup.Optimized native30Hz profiling completed:451/451 image groups in15s,15233 IMU
samples, no unsynchronized skips. Median pose age36.52ms; full frontend14.49ms,
other-camera detection0.652ms (previously21.7ms). Worst pose age265.55ms and only
88/451 quality-ready in the resting view remain limitations, not wearer acceptance.
The initial30Hz run accidentally used original .so.2 and backlogged.

Automatic-exposure preload is built on Kali as stock-camera-auto.so, not yet
installed. Shared policy now enforces common shutter across cameras with individual
gain. Startup metadata verification retries the initial command every second until
all four sensors agree. Native exposure policy tests pass. Next: deploy with backup,
verify repeated service starts and30Hz synchronized feed, then investigate upper
camera landmark starvation. Device returned to fastboot during context transition;
normal reboot requested, SSH reconnection pending. Runtime /run bundles need restore.

## Active revised wearer trial: MdeafM

Owner replied Ready. Started bounded180second trial `/run/quest-positional-trial.MdeafM`
with r5, four-camera grace worker and estimator-restart button. Launcher log
`/run/quest-vio-runtime-r5/revised-wearer.log`. Confirmed active renderer and worker,
then told GO: put to face, face forward, hold either stick1second, stay still until
START HERE SET, then lean. Pending question asks whether LIVE stays during leaning
or drops to HOLD. Leave scene unchanged while checking. Report trial expiry clearly.

## Current checkpoint: revised reset ready, wearer Ready reply pending

Original start-from-here button reset scene origin but kept a diverged estimator.
Revised trial now sets `MONTEREY_TRACKING_RESET_REQUEST` to a private request file.
A long stick click requests a fresh positional worker epoch and queues origin reset.
The trial launcher stops/joins only the old positional worker, archives its mailbox,
starts the replacement with the ORIGINAL remaining trial duration, then acknowledges
once the new worker publishes. Monado/orientation/rendering continue. Renderer waits
for acknowledgment AND tracked position before setting its origin. No false LIVE or
untrusted estimates are introduced. Long restart/quiet startup still use gates.

On-device25second smoke `pBfFgO` passed: request acknowledged, mailbox epoch increased,
new CSV segment produced, no old worker remained, renderer stayed running until the
original deadline, baseline restored and verified. Native renderer tests also cover
coalesced request creation and long/short button edges. New private portable bundle:
`camera-work/positional-r5-restart-staging.tar.gz` (supersedes recenter-only archive).
Private smoke logs `camera-work/restart-smoke-pBfFgO.tar.gz`.

Pending user question asks Ready for revised reset. LEAVE resting until GO. On Ready:
renew guard; launch `/run/quest-vio-runtime-r5/run-trial.sh /run/quest-vio-runtime-r5
/run/quest-vio-four 180`, wait for active renderer, then tell owner GO: put to face,
look forward, hold stick1second, briefly stay still for START HERE SET, then lean.
Do not silently start the timer before their Ready reply. Do not alter during check.
At expiry explicitly report baseline rotation-only return. Installed baseline still
has recenter control, installed runtime r3; r5 isolated.

Independent visual/gyro audit on marked rectified recording found9 usable rotating
image pairs. Existing axes rank first across24 proper signed permutations: median
rotation error.964deg atzero time shift; -30ms nearly tied. This supports current
axes but cannot establish precise timing or all head-turn behavior. Tool:
`tools/tracking/audit-visual-gyro.py`; private report `visual-gyro-marked.json`.

## Latest result: reset works; positional trial still failed

Owner reports “I press it, it flashes live for a split second then drops.” Trial
MEMalH log confirms TWO START HERE resets, so controller/recenter path works.
Estimator then loses shared features (lower-right and both upper logical slots zero)
and raw position diverges to roughly144m by the end. Quality gate rejects this;
view stays held. Do NOT loosen the gate or describe89% offline readiness as a live
fix. Trial180seconds completed; baseline POS OFF restored and service verified
started, hardware watchdog disable=0. Owner told they can set it down. No new
physical task pending. Private full logs `camera-work/recenter-trial-MEMalH.tar.gz`.
Next investigate loss during large head rotations, including visual/gyro frame and
timing consistency and frontend matching; avoid another unchanged wearer test.

## Completed wearer check — r5 and start-from-here control

Trial `/run/quest-positional-trial.MEMalH`, bounded180seconds, launched after native
HUD/recenter regressions and offscreen OpenXR test passed. r5 APK built with all22
package tests passing. Offscreen synthetic10cm motion with a100ms unsupported
interval retained full10cm displacement, then held untracked after staleness
(87samples,33tracked, finalnorm~.1m). Private log `vio-openxr-offscreen-r5.log`.
Owner told GO: pick up, face forward, hold either stick click1second, wait START
HERE SET then lean. The owner replied as recorded above. The180s bound expired;
launcher restores baseline POS OFF. No user acceptance yet. Owner-private restart archive:
`camera-work/positional-r5-recenter-staging.tar.gz` (26entries; restore under `/run`).
It contains four-camera calibration/map, private libs, grace worker, r5 and recenter
renderer; not the older two-camera archive.

Installed renderer now has recenter control; backup `.pre-recenter` beside it.
Installed runtime remains r3. r5 under `/run/quest-vio-runtime-r5`; four-camera bundle
`/run/quest-vio-four`, whose live-vit-quality is now the grace-enabled worker.
Also fixed baseline launcher readiness race by truncating its old service log
BEFORE launching background Monado; previously stale “Gravity initialized” could
let a renderer connect during fresh runtime initialization. Original script backup
`run-head-mesh-fb.sh.pre-log-reset`. Baseline restart after offscreen test failed
before this fix; trial launch itself successfully initialized.

## Owner walked but did not translate; camera order and exposure audit

**New owner-requested control:** hold either thumbstick click1second to “start
from here.” Renderer resets position origin and heading while preserving gravity;
it does not reset sensors or estimator. During HOLD the request queues until a
fresh tracked position arrives. HUD says RESET WAITING FOR LIVE, then START HERE
SET for3seconds. Short stick click still cycles effects on release; long hold does
not also cycle effects. Owner plans to pick the headset up and put it to their
face only AFTER we say go. Do not request continuous wearing while developing.
Renderer controller-edge, deferred reset, zero-origin, subsequent50cm displacement
and gravity-preservation tests added; final on-device checks underway.


**Brief-loss recovery fix underway:** the old gate erased initialization after one
unsupported frame, and Monado discarded alignment on every unready packet. This
could turn a momentary loss into HOLD until the wearer stopped moving. New worker
preserves established initialization across unsupported intervals up to250ms but
NEVER marks those unsupported samples ready. Freshness, continuity and feature
requirements remain. New Monado adapter separately tracks alignment and active
status; a short gap holds/untracks then resumes the original coordinates after a
validated good sample. Long loss, epoch change, invalid data, fusion reset or jumps
still require quiet reanchoring. No extrapolation. Package revision5 building;
installed r3 baseline unchanged. Native helper tests cover brief-loss motion,
long-loss quiet recovery and invalid data. Synthetic offscreen test pending.

Same marked four-camera recording: old gate1202/1798 ready, new gate1606/1798
(89.3%). Saved `marked-four-grace.csv`; this is support/geometry replay, not live
latency or wearer acceptance. Following the owner's unobstructed placement,
background30s atgain16 accepted255/301 with median65ms age; another atgain64
accepted81/301. Subsequent view/gate run hadzero readiness, so do not claim stable
current live tracking. Exposure restored to startupgain16. No persistent gain edit.


Owner clarified that the headset was resting on a couch with the lower cameras
covered during the subsequent background diagnostics. Treat those runs as
obstructed-view tests, NOT evidence that four-camera tracking fails in an open room.
Do not assume the owner is continuously wearing or holding the headset. Later they
reported all four sufficiently unblocked; a new stationary background test is in
progress. Ask for wear only when a meaningful new build is ready.

Four-camera live sampler vs recorded offline rectification: all four images exact
pixel match on the tested frame. Host malformed-input regressions10/10 passed.
Saved marked recording four-camera quality replay:1798/1798 poses,1202 gate-ready
(vs historical lower-pair1058/1798). This remains incomplete tracking, not a fix.
Upper logical slots median2 positive landmarks each, so additional input helps
only modestly with the current frontend. Private `marked-four-quality.csv`.


Latest wearer trial `quest-positional-trial.EDGLno` completed its 180-second bound
and restored the rotation-only baseline. Owner reports: “I saw it switch to live
only one time and never again.” Saved CSV contains 2701 poses, 803 gate-ready;
OpenXR briefly tracked displacement but subsequently held. This is NOT accepted
body tracking. No physical question is pending. Do not repeat this unchanged
lower-pair trial. Both recovery mechanisms remain enabled; installed Monado is r3.
Private complete logs: `camera-work/wearer-trial-EDGLno.tar`.

Current work: four-camera frontend using physical order 0,2,1,3 and the already
replayed matching calibration. New map builder `--four`, dynamic live frontend,
and multi-camera support gate are implemented. Native compilation and quality
regression pass, including loss of the lower-right view while other cameras
support tracking and rejection when only one view remains. Separate temporary
bundle `/run/quest-vio-four` preserves the lower-pair fallback. First 20-second
background diagnostic: 201/201 poses, 20308 IMU, zero synchronization skips,
median output age60ms, max92ms. Zero gate-ready; upper cameras zero triangulated
landmarks in this view. Investigating image brightness and frontend support before
another wearer trial. The renderer remains baseline POS OFF during diagnostics.

Owner previously reported slight sway, no movement while walking, and wrong-looking
floor stitching after an earlier bounded trial had already restored rotation-only.
Always announce when a positional trial ends and confirm actual OpenXR `tracked=1`
before asking the owner to assess translation.

Camera ordering was independently audited from four frames of the saved marked
recording, using mutual SIFT matches and factory epipolar geometry across all24
channel/calibration permutations. Identity0,1,2,3 explains469/492 matches at sine
error<.02 (median.00654); runner-up0,3,2,1 only180 (median.12557). Thus no evidence
for swapping stream camera IDs. Device floor-map is byte-identical to a fresh
factory-calibrated build. Tracking uses individual rectified0/2 streams, not this
floor composite. Its3m chosen-depth hard-seam stitch still cannot align nearby
objects at other depths; do not claim seamless or depth-correct passthrough.

Current imagery uncovered a separate major problem: all cameras at12008us and
Q4gain240 (15x), mean222–237/255, badly clipped. Temporary command54 from the
already verified camera-control path reduced gain to16 (1x), retaining exposure;
metadata verified12008us/16. New means40.8–71.1; clipped fractions2.5–5.9%, room
detail clearly visible. `tools/camera/set-exposure.c` reproduces bounded runtime
settings; no firmware or factory calibration writes. Persistence subsequently fixed: the probe now accepts bounded CAMERA_EXPOSURE_US
and CAMERA_GAIN_Q4 settings. Owner `/etc/conf.d/oculus-camera` uses12000/16;
service restart verified metadata12008/16 on all four streams. Old preload backed
up as `stock-camera.so.pre-exposure-control` alongside the installed preload.
Automatic exposure remains TODO; this is a manual indoor profile.
The earlier blown-out view caused divergent raw estimates; gate correctly rejected
all of them. Never remove quality gating to make a scene appear to move.

Private images and feed snapshots: owner `camera-work/current-camera-raw.png`,
`current-floor-composite.png`, `exposure-{before,after}.bin`, `exposure-after.png`.
Camera order audit on Kali: `~/quest-camera-work/camera-order-audit.json`; reusable
read-only auditor at `tools/camera/audit-camera-order.py`. r4 runtime and native VIT
bundle restaged in `/run`; baseline remains active until explicit live trial. Updated renderer now installed
with backup `monterey-head-mesh-fb.pre-positional-hud`; restarted baseline displays
POS OFF (no positional source), versus HOLD (source enabled but not accepted) and
LIVE (OpenXR position tracked). Installed Monado remains r3 until trial acceptance.
This renderer-only change makes fallback state explicit rather than silently
returning to an indistinguishable rotation-only scene.


## Opt-in OpenXR positional integration built and tested (2026-09-30)

Latest positive-control replay: the same support/startup gate accepted1058/1798
poses from the saved60second lower-stereo marked recording. First ready11.300s;
ready counts by recorded time:0–15s111/450,15–20s150/150,20–40s458/598,
40–50s39/300,50–60s300/300. This proves the gate can accept real recorded motion
and drop weak sections, not absolute accuracy or live latency. Short15Hz stationary
opening accepted only1/150 because the second camera's support was concentrated in
one image cell. Do not relax thresholds merely to pass the current obstructed view.
Private results copied locally as `camera-work/{opening,marked}-quality.csv`.
Historical checkpoint; subsequent wearer trial and current work are recorded above.


This supersedes the earlier "not wired into Monado" mailbox checkpoint below.
New Monado package r4 applies `0003-experimental-positional-mailbox.patch` after
0001/0002. Build completed; all22 package tests passed. It adds an opt-in
`MONTEREY_VIO_POSE_FILE` reader thread, with no file access on IMU/get-pose threads.
The adapter preserves the accepted orientation, holds position on stale/lost input,
and reanchors on restart. Disabled by default. Installed r3 remains unchanged;
r4 service/client and updated renderer are isolated under `/run/quest-vio-runtime`.

The renderer now consumes OpenXR position in the same heading-recentered frame as
its orientation. HUD shows `POS LIVE/HOLD` and XYZ metres. Native raster/geometry
regressions passed, including bounds/both eyes/camera panels/floor/ink checks.

**End-to-end offscreen test passed:** new service -> OpenXR client -> renderer
pose-only path received a synthetic10cm translation.87 pose samples,34 tracked;
final norm0.099999522m. After publisher stopped, tracked flag cleared and all samples
after8seconds held the same position. No synthetic positions were displayed.
Accepted scene automatically restored after test. Private evidence:
`camera-work/vio-openxr-offscreen.log`.

Live worker now enables VIT feature telemetry and an experimental acceptance gate:
>=6/3 positive-depth projections in the two cameras, >=12 unique landmarks,
>=1 shared landmark and >=3/2 occupied4x4 image cells; age<=150ms; initial3second
warmup plus1second of quiet estimates; continuity/jump and quaternion checks.
These are conservative engineering thresholds, not calibrated confidence or a
proof of correct scale. Unit tests cover startup, loss/reacquisition, supported
motion, stale timestamps, jumps and invalid/reversed data. The prior always-zero
ready flag is now enabled only with explicit owner head-offset calibration AND a
passing gate. CSV-only/no-offset use cannot publish a ready runtime pose.

`extract-head-offset.py` derives the device-origin lever arm from private owner
DeviceFromImu, refusing nonidentity rotation that the current replay pipeline does
not support. Device origin is the current head-origin convention, not a measured
optical-centre calibration. No owner calibration values are committed.

First live quality run:301/301 poses,20298IMU, zero unsynchronized skips, but zero
ready poses. Right camera had only0–1 positive landmarks. Actual camera capture
shows a close-up surface/strap and bright ceiling. Asked owner once to set it
upright, cameras clear, facing room detail; **that Ready response is pending**.
Do not lower the gate just to obtain a ready flag or repeat an old marked capture.

`run-positional-trial.sh` stages a bounded10–180second trial with fresh logs/mailbox,
optional fresh wearer-readiness gate, independent recovery guard, and cleanup that
restores the installed accepted scene. It does not install packages, modify boot
partitions, change sensor calibration or extend watchdogs automatically. A10second native live-camera trial completed151/151 poses,10157IMU and zero
unsynchronized skips. In the same obstructed view it stayed at zero position/HOLD,
then cleanly restored baseline. The first smoke run exposed a launcher timeout
race (renderer expired before worker teardown); renderer now gets10seconds margin.
Retest logs saved in `camera-work/vio-trial-smoke2.tar`. Wearer translation acceptance
is still outstanding. New binaries/archive are also saved in the owner's local
`camera-work`: `live-vit-quality-arm`, `monterey-head-mesh-vio`,
`monado-vio-r4.tar.gz`, `test-vio-hud`. Reboot clears all staged `/run` assets.


## Recovery, camera startup and pose mailbox checkpoint (2026-09-30)

Owner asked whether the battery had run out. Headset was in fastboot at4277mV;
normal reboot of the existing image restored Linux, battery100%. No flashing or
slot change. Guard expiry is consistent with the observed state, but the previous
boot's exact reboot cause was not recovered. Mac direct USB/Wi-Fi SSH timed out;
SSH through Kali (`ProxyJump`) reached the same verified headset host key.

Camera autostart root cause found in the preceding boot: OpenRC restored a future-
dated `/var/cache/rc/deptree` that omitted `oculus-camera`, despite its enabled
runlevel link. `rc-update -u` rebuilt the graph. Preserved old cache as
`/var/cache/rc/deptree.pre-camera`, then atomically copied regenerated `deptree` and
`depconfig` from `/run/openrc` into the cache. **This normal reboot verified the
fix:** camera and scene started without manual service start, fresh307328-byte
feed present. Hardware watchdog disable remains0; recovery guard explicitly renewed.
Further cold-power-cycle and clean-image packaging verification remains.

New experimental `patches/monado/runtime-src/monterey_vio.h` handles coordinate
alignment, IMU-to-head lever arm, stale-pose hold, restart re-anchoring and jump
rejection. It is a standalone tested helper, **not yet wired into Monado**. Tests
cover translation, axis rotation, pure rotation about an offset head origin, stale
hold, restart, quiet re-anchor and invalid values; passed on Mac and native headset.
A quiet angular-speed sample is not proof of translational stillness or quality.

`monterey_vio_io.h` defines a128-byte versioned little-endian mailbox and a bounded
regular-file reader intended for a separate polling thread. It rejects wrong
size/version/reserved bits, symlinks, FIFOs and directories. Host/native tests pass.
The optional `QUEST_VIO_MAILBOX=/absolute/new/path` in `live-vit.cpp` atomically
publishes diagnostic poses. **Every packet has ready=0 and head offset zero.**
No runtime can accept it through the adapter; quality gating and private head-origin
calibration are deliberately outstanding. CSV-only invocation remains supported.

New20-second device run:300pairs/300poses,20307IMU, zero unsynchronized skips.
Driver timestamp age median57.3ms, p95 85.5ms, max110.8ms; this is a different run,
not a controlled comparison proving mailbox overhead. Saved128-byte mailbox matches
last CSV pose and ready=0. Source compiles with warnings as errors on host and ARM;
seven malformed-map cases still pass. Scene/camera services remained started,
battery100%, watchdog enabled. Private outputs are
`camera-work/live-vio-mailbox-{test.csv,test.log,pose.bin}` in the owner workspace.

Next concrete integration step: expose/check feature support and startup stability,
load owner IMU-to-head calibration, then connect an opt-in mailbox polling thread to
Monado and rotate its position by the renderer's heading origin. Preserve baseline
orientation, track-loss holding and fallback; do not flip ready blindly. No new
wearer movement capture is needed to continue this work.


## Live native sensor-to-pose probe passed — current checkpoint

`tools/tracking/live-vit.cpp` now reads live SyncBoss IMU plus the existing atomic
camera feed, applies a private lower-camera0/2 rectification map, and runs the
isolated native VIT tracker at15Hz images/full IMU rate. No pose is connected to
Monado or the renderer. No sensor configuration or persistent system changes.

Device5second test:75/75 image groups/poses,5079IMU, zero unsynchronized skips;
median driver-timestamp age37.3ms, p95 46.3ms, max56.8ms. Device20second test:
301/301 groups/poses,20308IMU, zero skips; median43.5ms, p95 58.3ms, max68.6ms.
Observed position after the first3seconds spans34mm from that reference with a43mm
largest step. No claim of independently measured stillness or absolute accuracy.
The accepted scene/cameras stayed started; hardware watchdog remained enabled.

Map sampler matches saved OpenCV rectification pixel-for-pixel on the tested real
frame after matching float32 coordinate quantization. Host and ARM builds pass
warnings-as-errors; seven malformed-map cases reject before sensor/tracker access.
Final source fixes a camera-publication timestamp race and close-error handling.
See`tools/tracking/README.md` for the bounded command, dependency setup and caveats.
Private results:`camera-work/live-vio-{first,twenty}.csv`; temporary device files
under`/run/quest-vio-lean`. Live probe binary v2 is separate from baseline replay.

Timing audit: old33ms-after-image pacing artificially withholds bracketing IMU.
New optional timestamp-paced mode measured~291ms median age at30Hz on CPUs4/5
with the scene active. Decimating to15Hz images while retaining all IMU gave40ms
median,57ms p95,66ms max and zero outstanding later camera groups. This motivated
15Hz live input. Replay submission age and live driver-timestamp age are distinct;
neither measures photon-to-display latency.

Experimental triangulation patch0004 tested then REVERTED: original-order marked04
endpoint1.21m; reordered marked04 provisional20→40s0.438m and20s→end0.105m, max
step20mm. Results are mixed rather than a general fix. Active host/device libraries
retain accepted patches0001/0003 only. Camera ordering0,2,1,3 remains a promising
separate offline experiment; live baseline uses lower pair0/2.

Next: quality/startup-jump and tracking-loss handling, IMU-to-head lever arm and
world-frame alignment, then opt-in Monado positional-pose integration. Do not feed
raw diagnostic CSV positions directly to rendering or replace accepted3DoF blindly.
User's attempted marked return is recorded below; no physical question pending.


## Owner clarification — marked return

Owner: "i tried to leave it there in line with my marker". Treat the end as an
attempted marked return with placement uncertainty, not evidence of deliberately
moving again afterward. The recording begins on the table before the held movement;
choose held segment reference times explicitly rather than calling table-to-end
translation drift. No physical question pending and no new capture requested.

Host-only experiment underway: choose landmark triangulation candidates by parallax
and reject candidates that fail reprojection checks, instead of accepting the first
positive-depth candidate in camera order. Preserve baseline host library and keep
the device's native bundle/accepted renderer unchanged until replay comparisons.


## Native headset replay verified — latest checkpoint

Minimal upstream OpenCV4.13.0 build completed; archive SHA256
`1d40ca017ea51c533cf9fd5cbde5b5fe7ae248291ddf2af99d4c17cf8e13017d`.
`build-opencv-headless.sh` records the configuration. `bundle-native-replay.py`
collects ARM ELF dependencies without system installation; private bundle12files,
19,119,176bytes (versus~253MB with distro OpenCV's graphics dependencies). Includes
its own musl loader, explicitly invoked with a private library path. All12file hashes
verified on headset. Staged only in`/run/quest-vio-lean`; disappears at reboot.

Both emulated and real-device3frame smoke tests produce3/3 poses. On headset,
300camera groups over9.967s produced300timestamp-matched poses. Low-priority CPUs0/1:
16.98s wall,23.29s user,6.92s system. Native endpoint magnitude0.093mm; max positional
difference from host reference12.8mm (different OpenCV/compiler/float execution).
Low-priority CPUs4/5:10.47s wall,11.42s user,4.69s system,300/300 poses. Replay inserts
33ms per-frame input pacing; these wall times are not unconstrained throughput or
sensor-to-display latency measurements. Real-time latency and dropped-frame handling
remain unverified. Accepted display/controller/camera services and watchdog preserved.

Private host results: `camera-work/native-{opening,perf}-poses.csv`,
`native-opening-features.csv`, `native-perf-verification.log`, native bundle archive.
Kali build/results:`~/quest-vio-deps`, `~/quest-camera-work/native-lean-01`.
No native positional pose wired to Monado, no persistent library replacement.

Next: use the clarified marked-return intent to assess error, then test live asynchronous camera/IMU
feeding with latency instrumentation before exposing positional pose to rendering.
Use lower-camera stereo as the stable baseline; four-camera0,2,1,3 ordering remains
an offline promising experiment, with pose jumps/return error requiring investigation.


### Camera order experiment — promising, not accepted live tracking

Ordering physical cameras0,2,1,3 (images AND matching calibration entries together)
changes triangulation initialization materially. This is not correcting mislabelled
factory cameras. On marked04, shared landmarks rise to median3/max19 versus median0/
max4 in0,1,2,3 order; stationary15s position is about12mm from origin rather than
about86cm. Provisional20→40s displacement0.487m;20s→final0.131m. A97mm largest pose
step remains, so this is not ready to deploy as a positional pose. Other-clip checks completed: motion03 has0.573m peak and0.097m endpoint
(after3s reference), shared landmarks median10/max28; original stationary clip
has10mm endpoint/15.4mm peak after3s, max pose step2.3mm. These improve confidence
in the ordering experiment but do not establish ground-truth accuracy. Source inspection shows triangulation accepts the first qualifying
observation; ordering is a hypothesis for why the result changes, not proof of root cause.

Masking rectified black borders did NOT resolve the four-camera failure (shared
median0/max4,236mm max step). Keep masks optional/off by default. Independent sparse
SIFT/stereo PnP had only7–12 inliers across selected long intervals and mixed motion
estimates; it is not ground truth.


## Resumed investigation — isolate camera selection before changing IMU calibration

Documentation audit completed; tracking work resumed. The two-camera0/2 opening is
stable: the first10seconds return within1.1mm, with an initial51mm transient.
Its poses exactly match that interval in the full two-camera recording. The prior
large stationary-opening drift belongs to the **four-camera** replay, not all setups.
Do not carry the earlier broad startup-drift statement forward without this distinction.

Saved images show table placement through15s, lift by20s, sideways travel and return
while held. Comparing20s to40s with lower-camera stereo gives0.401m;20s tofinal gives
0.083m. These timestamps are provisional, not verified marked hold positions.
Upper-camera1/3 stereo gives0.662m and0.179m respectively. Physical endpoint history
is pending from the owner. No new capture requested.

Trimming the first3seconds did not resolve full-clip endpoint disagreement. A new
host-only experiment masks invalid rectification borders through VIT image masks;
did not fix drift (see newer result above). Source `rectification-masks.py`; replay optional MASKS.txt contains
camera/x/y/width/height integer rows. Input validation tests include malformed masks.

ARM dependency audit found packaged OpenCV core transitively pulls GL/Gallium/LLVM,
about253MB total. A minimal OpenCV4.13.0 ARM build (core/imgproc/features2d/flann,
OpenGL/OpenCL/LAPACK/GUI disabled) is running on Kali under`~/quest-vio-deps` before
native device staging. No system headset libraries replaced.


## Latest checkpoint — marked capture saved; documentation audit pause

The60second marked recording already completed: `vio-motion-04-marked.bin`,1798
camera groups and62952IMU samples, private host copies. Older readiness prompts
below are historical. No more movement capture is currently needed. Clarification
about any movement after the marked return remains unresolved.

Selected rectified four-camera replay has about0.87m peak/0.43m endpoint displacement,
including substantial estimated movement during an apparently stationary opening.
Two-camera0/2 replay improves shared tracks (median3/max20), but still drifts.
Clock fitting and stationary gyro/radial acceleration initialization did not fix it.
Do not describe acceleration calibration as the proven sole blocker or camera
calibration/vision as proven correct.

Native aarch64-musl Basalt and replay built. ARM-buildroot emulated smoke replay of
three real camera groups completed with3/3 poses. No native headset VIO deployment
or positional Monado integration yet. Accepted patches are0001 and0003; experimental
0002 remains reverted. Next: diagnose startup using saved data and audit native
runtime dependencies. Device tracking work is paused for the owner's tutorial audit.
See [current work/TODO](tutorial/09-status-and-next.md).


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


<a id="archived-prediction-audit"></a>

## Archived investigation: prediction-audit (2026-10-01)

Historical audit, retained verbatim. The final checkpoint supersedes active/pending wording.

# Offline audit of rejected r6 positional prediction

2026-09-30. No headset access, deployment, or wearer trial. The wearer rejected r6;
prediction must remain opt-in and disabled in the accepted candidate. These findings
do not establish the cause of that regression. GFdfKL omitted velocity telemetry,
and the capture prefilter also changed relative to the earlier accepted improvement.

## Fixed: interval-average lever velocity mixed with instantaneous IMU velocity

`monterey_vio.h` formed head position as `p_imu + R * head_offset`, correctly.
But its predicted head velocity was instantaneous Basalt world IMU velocity plus
`(current_rotated_offset - previous_rotated_offset) / dt`. That second term is a
chord/interval-average velocity. During constant-rate rotation about a stationary
head origin it does not cancel the instantaneous IMU velocity. Thus prediction
introduces translation even with ideal, noise-free pose and velocity input.

At 30 Hz poses and 65 ms prediction delay, representative synthetic cases produce:

| Lever length | Angular speed | False head displacement |
| --- | --- | --- |
| 3 cm | 1 rad/s (57°/s) | 0.0325 mm |
| 3 cm | 3 rad/s (172°/s) | 0.2924 mm |
| 3 cm | 8 rad/s (458°/s) | 2.0759 mm |
| 10 cm | 3 rad/s | 0.9747 mm |
| 10 cm | 8 rad/s | 6.9196 mm |

The lengths and rates are test parameters, not measured GFdfKL motion or a claim
about the owner's calibration. Ordinary slower turns make this a small error;
this result cannot explain the entire reported regression on its own.

The helper now stores the previous accepted orientation. It estimates world angular
velocity from `q_current * inverse(q_previous)`, chooses the quaternion sign giving
the shortest increment, and adds `omega_world × current_rotated_offset` to the
world IMU velocity. This is the correct endpoint lever derivative for constant
angular velocity. It still approximates angular acceleration across the pose interval;
synchronized gyro data would be needed to measure instantaneous angular velocity.
Existing freshness, displacement, velocity, readiness, and prediction bounds remain.

`test-vio-prediction-regression.c` uses analytic stationary-head motion across 200
combinations: five rotation axes (including oblique axes), identity/nonidentity
initial orientation, antipodal quaternion representations, positive/negative/zero
angular rates, and two lever lengths. The unpredicted position remains zero.
Replacing only the production derivative with its previous chord formula fails
160/200 cases, maximum 6.919648609 mm. The corrected helper passes 200/200 at
1e-10 m tolerance. Existing adapter and prediction tests also pass.

```
cc -std=c11 -O2 -Wall -Wextra -Werror tools/tracking/test-vio-prediction-regression.c -lm -o /tmp/quest-prediction-audit
/tmp/quest-prediction-audit
```

Parent integration regenerated Monado patch 0003 and its package checksum, bumped
the experimental package to r7, and completed the cross-build with22/22 package tests passing. The five host adapter,
mailbox, prediction, rotation-regression and quality checks pass. Nothing was installed
on the headset; see the main checkpoint for final package-build status.

## Fixed offline: HOLD state is independent of pose query order

Follow-up 2026-10-01. `mv_render_position` previously wrote `presented` for every
query, although `get_tracked_pose` is not a display-submission callback. A 1 m/s
sample queried at measurement+65 ms yielded 6.5 cm; an additional historical query
at +10 ms changed subsequent loss HOLD to 1 cm. A speculative future query could
also determine HOLD. No evidence establishes that these query patterns occurred in
GFdfKL; its renderer requests current time once per frame.

The selected policy now uses the sensor/update timeline, with a pure const getter:

- On the **first explicit loss update**, `mv_deactivate(state, reception_time)`
  freezes the last accepted state's bounded prediction at that reception time.
  Subsequent failed polls do not move the frozen value. Unsupported/invalid/jump
  rejection paths inside `mv_update` use this helper.
- On **silent staleness**, the getter returns the fixed prediction-horizon endpoint.
  The normal prediction already reaches its 100 ms / 15 cm bound before the 150 ms
  freshness expiry, so current-time queries do not snap back at that boundary.
- On **long loss or epoch restart**, a separate prediction-coordinate offset keeps
  the predicted output at the frozen position. Raw measurement position and its
  coordinate origin remain unchanged, preserving prediction-disabled behavior.
- On **brief supported recovery**, existing coordinates are retained and estimator
  corrections remain visible. The code does not silently reanchor to disguise them.

`presented`/`have_presented` were removed. Historical, future, and absent queries
cannot change sensor state, HOLD, or reanchor. `test-vio-prediction-regression.c`
now includes all cases by default; `--query-order` is no longer a failing opt-in
mode. It verifies query purity, both query permutations and no queries, explicit
loss, repeated loss, silent expiry, external deactivation, same/different epoch
reanchors, moving/quiet startup, raw-path invariance, high-speed bounds and brief
recovery. Adapter, prediction, and all 200 lever-geometry cases pass.

**Integration requirement:** both external Monado invalidation paths (IMU gap and
mailbox/fusion failure) must call `mv_deactivate(&d->vio, now_ns_or_now)` before
clearing `anchored`, instead of assigning `active=false` directly. The helper cannot
capture an update-time freeze if its caller bypasses it. Parent owns patch/package
integration; no headset deployment occurred during this audit.

### Policy comparison and remaining continuity limits

Holding the last raw measurement would be query-independent and preserve the
baseline, but would snap back by as much as 15 cm when abandoning a prediction.
That policy was rejected. Holding the largest queried target timestamp would allow
speculative queries to choose the frozen position, so it was also rejected.

The chosen policy gives a deterministic update-time position, **not a claim about
what was actually displayed**. Between the last current-time render and loss
reception, it may advance by the bounded elapsed prediction; normally this is a
frame/poll interval. A client rendering a far-future predicted pose can still see
that pose corrected to the loss reception-time hold. A correct API cannot infer
last-displayed content from arbitrary pose queries. Strict display continuity
would require submission feedback or a separate explicitly defined presentation
filter; neither is present here.

The accumulated prediction coordinate offset after reanchors is a local coordinate
choice, not a recovered physical displacement. Brief reacquisition and estimator
corrections can still cause jumps. The 100 ms / 15 cm limit bounds each prediction
relative to its predicted coordinate anchor, not accumulated world-origin offsets.
This change solves query-order dependence and avoids raw-position snapback at loss;
it does not establish comfortable wearer motion or fix low-light tracking.

## Other inspected behavior and remaining limits

- Renderer requests current monotonic time through `xrConvertTimespecTimeToTimeKHR`;
  it does not request predicted scanout time. No obvious seconds/nanoseconds or
  future-display-time double prediction was found in this path.
- Initial alignment combines current fusion orientation with delayed VIO orientation.
  The quiet-anchor speed limit reduces but does not formally bound timing error;
  no new reproducible frame-convention error was established here.
- Short unsupported intervals freeze the reception-time prediction; reacquisition can
  expose estimator corrections immediately. Long outages re-anchor. Synthetic
  constant-speed ramps do not validate correction/stop/reversal behavior.
- Prediction caps its horizon at 100 ms while allowing tracking ages through 150 ms.
  At that bound even ideal constant velocity stops advancing until another accepted
  measurement arrives. This is an intentional safety bound, not evidence of low latency.
- Finite-difference angular velocity remains sensitive to VIO orientation corrections.
  The fix removes a specific geometric inconsistency; it does not solve estimator
  corrections, sensor timing, low-light tracking, or display pacing.


<a id="archived-imu-prediction-audit"></a>

## Archived investigation: imu-prediction-audit (2026-10-01)

Historical audit, retained verbatim. The final checkpoint supersedes active/pending wording.

# Short-horizon IMU propagation audit — 2026-10-01

**No robust positional advantage over constant velocity was demonstrated.** Keep
runtime prediction disabled. This is an offline consistency diagnostic, not a
tracking fix, accuracy validation, or proposal to replace existing orientation
tracking. Nothing was deployed and no device interaction was required.

## Conventions checked against Basalt source

- `src/vit/vit_tracker.cpp` exports position/orientation from `T_w_i` and velocity
  from `vel_w_i`: the quaternion maps IMU-body vectors into the world, and velocity
  is already world-frame.
- `include/basalt/utils/imu_types.h` defines `constants::g=(0,0,-9.81)`; the VIT
  tracker passes it to the estimator factory.
- `thirdparty/basalt-headers/include/basalt/imu/preintegration.h` rotates body
  specific force using a midpoint SO(3) rotation and adds world gravity in state
  prediction. It subtracts estimated accelerometer/gyro biases before integrating.
- Prepared event payloads already contain accelerometer m/s² followed by gyro rad/s
  in the Basalt body basis. Do not apply the raw-device axis transform again.
- The supplied prepared calibration's fixed accelerometer/gyro calibration arrays
  are all zero, representing identity correction. The diagnostic verifies this and
  refuses a nonidentity calibration instead of silently ignoring it. These fixed
  arrays are **not the optimized per-pose biases**, which the saved VIT CSV omits.

## Diagnostic and tests

`tools/tracking/imu-propagation-consistency.py` starts from each recorded VIO IMU
pose and world velocity, then integrates incoming IMU through 20,35,65,100 ms.
For each interval it uses only the latest already-available IMU sample (left hold),
updates orientation on SO(3), rotates specific force at the interval midpoint and
adds gravity. It does not consult any IMU sample later than the target. Seed or
integration gaps over 10 ms are rejected. No head-origin lever arm, display-frame
alignment, reset/reacquisition behavior, optical model or runtime presentation is
included. There is no smoothing or optimization using future trajectory.

This models propagation of a **delayed VIO state up to now using IMU already received
through now**, not knowledge of future acceleration beyond received measurements.
The initial VIO state is the published estimate, not an assertion of independent
truth. All optimized IMU biases are assumed zero; they cannot be recovered from
these CSVs without additional estimator telemetry.

The future **same-estimator** VIO pose is used only for scoring, with linear position
interpolation and shortest-path quaternion interpolation between finite endpoints
no more than 100 ms apart. That reference can contain estimator corrections, drift
and interpolation error. It is circular evidence because Basalt itself uses this
IMU. A smaller discrepancy cannot establish actual tracking accuracy or wearer
comfort. Constant velocity and held-position controls use the same seed and target.

`test-imu-propagation-consistency.py` has six passing tests: stationary/tilted gravity
cancellation, constant world acceleration, stationary-position rotating IMU with
changing body gravity, no access to future IMU values, gap/nonfinite rejection, and
reference interpolation including quaternion sign. The rotating test uses 100 us
samples and explicitly tolerates the tiny left-hold discretization error rather
than assuming exact cancellation.

## Inputs and completion

Used the root agent's deterministic `static1.csv` and `marked1.csv` from private
`deterministic-audit-dl0hpbpi`, paired with their original prepared events and
calibration. Only IMU records were extracted for transfer; no image payloads were
copied. Event header/payload bytes were preserved unchanged for each retained IMU.
Quality subsets use the controlled threshold-5 quality outputs whose pose CSVs
match the root baseline. All seeds before 3 seconds are excluded from scoring.

- Static: 451 poses, 17262 IMU samples. Valid scored seeds by horizon:
  360,359,359,358. All exclusions are targets beyond the final VIO reference.
  No ready-seed subset exists: this recording never passed the quality gate.
- Marked: 1798 poses, 62952 IMU samples. Valid scored seeds:
  1707,1706,1706,1705; ready-seed subset:1568,1567,1567,1566.
  Again, exclusions are only unavailable final VIO endpoints, not IMU gap failures.

All analysis completed. These counts are analysis windows, not fresh device frames
or a realtime throughput benchmark.

## Position consistency results

Errors are Euclidean millimetres relative to interpolated future VIO. Each entry
shows **median / 95th percentile** over all eligible seeds after 3 seconds.

| Clip/horizon | Hold position | Constant velocity | IMU propagation |
| --- | --- | --- | --- |
| Static 20 ms | 0.667 / 3.031 | 0.635 / 3.006 | 0.641 / 3.011 |
| Static 35 ms | 1.145 / 4.862 | 1.098 / 5.026 | 1.102 / 5.049 |
| Static 65 ms | 2.052 / 7.138 | 1.966 / 7.872 | 1.969 / 7.896 |
| Static 100 ms | 2.904 / 9.695 | 2.874 / 10.856 | 3.008 / 11.026 |
| Marked 20 ms | 0.566 / 2.791 | 0.410 / 2.244 | 0.411 / 2.208 |
| Marked 35 ms | 0.978 / 4.762 | 0.715 / 3.804 | 0.716 / 3.716 |
| Marked 65 ms | 1.751 / 8.078 | 1.386 / 5.774 | 1.432 / 5.785 |
| Marked 100 ms | 2.550 / 11.729 | 2.186 / 7.711 | 2.356 / 8.143 |

IMU propagation beats constant velocity on only 41%,38%,36%,32% of marked windows
at the four horizons. The tiny 20/35 ms tail improvements coexist with slightly
worse medians. Static tail errors worsen at every horizon. Restricting marked seeds
to quality-ready does not rescue 65/100 ms: constant-velocity versus IMU p95 is
5.276→5.412 mm and 6.973→7.401 mm, respectively.

A held static reference can look better than either propagator because the future
VIO reference itself wanders; this is another reason not to interpret the table as
physical tracking accuracy. The 20/35 ms tail gains are not sufficient evidence to
survive missing-bias assumptions, finite reference interpolation and circularity.

## Attitude and bias qualifications

IMU rotation predicts marked future VIO attitude better than simply holding the
seed attitude: p95 held→propagated is 0.2515→0.0694 degrees at 20 ms and
1.0307→0.2710 degrees at 100 ms. Static attitude consistency instead worsens:
0.0420→0.0492 and 0.1422→0.1962 degrees. **This is not a comparison against the
headset's current Monado orientation fusion**, which already follows the IMU.
No orientation runtime change follows from this result.

As an illustrative sensitivity bound, an unknown constant accelerometer bias of
0.1 m/s² can contribute up to `0.5*b*h²`: 0.020,0.061,0.211,0.500 mm across these
horizons. A 0.01 rad/s gyro bias under initially correct gravity alignment gives a
small-angle gravity position term approximately `g*b*h³/6`:0.00013,0.00070,
0.00449,0.01635 mm. These are assumed example magnitudes, **not measured biases or
confidence intervals**; initial attitude error and dynamic acceleration introduce
additional terms. The accelerometer example is already comparable with the small
apparent tail improvements. Missing estimated biases therefore matter even at these
short horizons.

## Reproduction and next evidence

The tool requires Python with NumPy (the checked host interpreter was
`/Users/<user>/.pyenv/versions/3.13.1/bin/python3`):

```sh
python3 -B tools/tracking/test-imu-propagation-consistency.py
python3 -B tools/tracking/imu-propagation-consistency.py --poses POSES.csv --imu-events EVENTS.bin --calibration CALIBRATION.json --quality QUALITY.csv --output RESULT.json
```

`--quality` is optional; it adds a ready-seed subset rather than changing integration.
Full prepared events or IMU-only extracted events are supported. The calibration
file is mandatory. Private inputs, manifests, and complete JSON metrics are at
`/Users/<user>/work/quest-pmos-bringup/camera-work/imu-propagation-audit-9kq7wbss`;
extraction provenance also exists in the same named Kali directory under
`/home/<user>/quest-camera-work`. Output JSON hashes the actual pose, IMU,
calibration and quality inputs. No private calibration content was added to git.

Before runtime integration, capture each published VIO state's optimized accel/gyro
bias, clock basis and validity alongside position/velocity/orientation, and retain
the matching IMU stream through display time. Re-evaluate correction jumps, stops,
rotation/translation and loss/reacquisition with actual state propagation rather
than assuming zero bias. Independent movement/reference evidence and a later
wearer test remain necessary; this audit is not a reason to re-enable r6 prediction.


<a id="archived-timing-audit"></a>

## Archived investigation: timing-audit (2026-10-01)

Historical audit, retained verbatim. The final checkpoint supersedes active/pending wording.

# Camera / IMU timing audit — 2026-09-30

The current evidence does not establish an exposure-time correction to apply.
Keep the accepted clock mapping and rejected prediction disabled. The measured
clock-rate discrepancy is small over the marked recording, but **this does not
exclude a fixed camera/IMU offset**. No device was contacted or changed for this
audit; work used repository source and read-only access to saved Kali recordings.

## Reproducible measurements

New `tools/tracking/audit-capture-timing.py` reads QVRREC01 without retaining images
and emits aggregate timing. Run with Python + NumPy against the private source:

```
/home/<user>/quest-camera-work/vio-motion-04-marked.bin
```

The raw clip has 1,798 camera groups and 62,952 IMU samples spanning 61.997644 s
(IMU recording extends approximately two seconds beyond the camera capture).
The camera ABI offsets are derived from `record-sensors.py` and `camera-feed.h`:
eight-byte recorder arrival precedes the unchanged 128-byte feed header. Camera
times at payload offset32, exposure at80, publication at128, pixels at136.
`test-capture-timing.py` verifies these offsets with known packed data and verifies
a synthetic100 ppm clock. Seven tests pass with the explicit interpreter below,
including rejection of insufficient camera/IMU data, too few lower-envelope fit
points, malformed camera ABI headers, unknown record types and truncated headers.

Verified Mac invocation (independent of shell working directory):

```sh
/Users/<user>/.pyenv/versions/3.13.1/bin/python3 tools/tracking/test-capture-timing.py
```

The initial local verification ran from `/Users/<user>/work`, whose `python3`
resolved via pyenv to that Python3.13.1 interpreter. In the repository directory,
the shell instead selects Xcode Python3.9 (`/usr/bin/python3`), while the inherited
`PYTHONPATH` points at Python3.13.0 site-packages. That mismatched path exposes an
incomplete `numpy` namespace (`__file__` is `None`, no `array` attribute). This is an
interpreter/environment mismatch, not a repo `numpy.py` shadow. No global packages,
shell configuration or environment files were modified. The audit now detects
incomplete NumPy at startup and reports a clear dependency error. Other machines
need a Python interpreter with a compatible, complete NumPy installation. The
actual raw-recording audit ran on Kali using its existing `python3`+NumPy; its
aggregate results above were not produced by the mismatched Xcode interpreter.

| Measurement | Result |
|---|---:|
| MCU sample interval median / min / max | 0.985 / 0.984 / 0.985 ms |
| Host arrival above global minimum host−MCU offset, median / p95 / max | 1.653 / 2.611 / 9.258 ms |
| First-second minimum offset minus global minimum | 0.071804 ms |
| Later samples mapped after host arrival using that first-second minimum | 2 |
| Lower-envelope affine rate difference | +26.521 ppm |
| Accumulated fitted rate difference across this recording | 1.644 ms |
| Camera interval median / maximum | 33.334 / 99.999 ms |
| Intercamera driver-timestamp skew median / maximum | 0.015 / 0.041 ms |
| Driver timestamp to publication median / p95 / maximum | 2.259 / 3.612 / 6.873 ms |
| Publication to recorder arrival median / p95 / maximum | 4.238 / 6.985 / 26.589 ms |
| Exposure, all four cameras, throughout clip | 12,008 μs |

The affine fit reproduces `prepare-replay.py --fit-imu-clock`: fit relative
host/device times, select the lowest10% residuals, refit. It is a transport-based
estimate, not a calibrated oscillator measurement: changing transport delay can
bias its slope. The first-second comparison emulates the live formula on this
saved capture; it is **not** the actual warmup from the rejected wearer trial.
Exposure here is older than the current automatic-exposure and prefilter changes.
No claim about current low-light photon timing follows from this clip.

## What the code actually does

- `capture-imu.c` and `live-vit.cpp` assign the same host read time to every sample
  in a received SyncBoss packet; embedded MCU ticks remain per-sample. Live mapping
  estimates `min(host − tick*1000)` over one second and then fixes that offset.
- Default `prepare-replay.py` instead uses the minimum over the **entire recording**.
  It offers an affine-fit mode. Consequently live and replay are not identical
  timing pipelines, although the start/global discrepancy above is only72 μs here.
- `stock-probe.c` copies library buffer seconds/nanoseconds into the feed, with no
  exposure midpoint adjustment. Live/replay average near-synchronous camera
  timestamps into the one timestamp required by VIT. The measured group skew is
  tiny compared with the exposure duration.
- Replay calibration writes `cam_time_offset_ns=0`. Live pushes buffer times and
  mapped IMU times directly to VIT. The VIT bridge inspected in Kali Basalt
  `src/vit/vit_tracker.cpp` assigns input timestamps directly to its samples.
  Zero is currently an assumption, not proof of calibrated camera/IMU synchrony.
- Available kernel source at `quest-camera-work/kernel/drivers/media/platform/msm/
  camera_v2/isp/msm_isp_util.c`, `msm_isp_get_timestamp`, uses `ktime_get_ts` for
  buffer time unless `vt_enable` selects the AV timer. This supports auditing the
  driver path but does **not** yet prove the exact active path from sensor exposure
  through the proprietary camera library to our returned buffer.

If buffer time denotes exposure end, midpoint would be6.004 ms earlier for this
clip; if it denotes start, midpoint would be6.004 ms later. Neither convention is
proven, and readout/IRQ/library timing may add other terms. Do not implement either
sign from this conditional arithmetic.

## Existing visual/gyro check does not calibrate delay

Private `visual-gyro-marked.json` from `audit-visual-gyro.py` has only nine usable
rotation pairs. Identity gyro axes have median error0.96443° at zero delay and
0.96856° at−30 ms, while p90 favors−30 ms (1.586° versus2.152°). That broad/ambiguous
result supports the axis choice; it does not establish a precise temporal offset.

The diagnostic uses stride6 images (~200 ms intervals), essential-matrix rotation,
component-wise trapezoidal gyro integration, a fixed camera-axis transform, and
five coarse delay choices. Translation/low parallax, extrinsics and noncommuting
rotation can confound a delay ranking. Keep it an axis diagnostic, not an automatic
calibration tool. A denser numerical delay sweep alone would not resolve those
model limitations.

## Proposed next change

Add opt-in raw timing telemetry to a future authorized live capture: MCU tick,
host packet read time, mapped time, four original buffer times, publication time,
exposure metadata, and the exact warmup offset. Preserve the originals rather than
silently correcting them. Then replay the **same** capture with controlled timing
variants and compare residuals and tracking, using the same quality gate.

Before any fixed camera-time correction, trace the active buffer timestamp path
and/or perform a validated rotation/exposure alignment analysis with camera
extrinsics, SO(3) gyro integration and interval-end interpolation. A future physical
capture may help that calibration, but there are no physical prompts tonight.
Transport drift alone is not a supported explanation for the rejected prediction
trial, and the unmeasured constant offset remains unresolved.

## Buffer timestamp provenance traced offline — 2026-10-01

The saved stock libraries establish that our returned timestamp is the V4L2
**dequeued buffer timestamp**, copied without exposure correction. The available
kernel's ordinary completion path supplies a **VFE interrupt timestamp associated
with AXI ping/pong buffer completion**, rather than calculating exposure midpoint.
This substantially narrows the unknown epoch. It does not establish an exact
photon-time offset or justify subtracting half the exposure duration alone.

All following paths are on private Kali, under
`/home/<user>/quest-camera-work`. Addresses are ELF virtual addresses from
`llvm-objdump -d`, not runtime ASLR addresses. No binaries or source were modified.

### Exact userspace chain

1. `stock/vendor/lib64/libqcameraoculushal.so`,
   `qcamera_dequeue_nonblocking` at `0x4944`: calls
   `control_dqbuf_nonblocking` at `0x4960`. Helper at `0x49a8` receives the returned
   internal frame. Instructions `0x49e4` and `0x49f4` copy its16 bytes at offset8
   into the allocated wrapper at offset8; `0x49f0` returns a pointer to that offset.
   Thus our returned `raw[0]` and `raw[1]` are exactly that timestamp pair.
2. `stock/vendor/lib64/libqcameradriver.so`,
   `control_dqbuf_nonblocking` at `0xa2a4`: calls `mm_stream_read_msm_frame` at
   `0xa2dc` and returns the frame pointer from its output at `0xa314`.
3. Same library, `mm_stream_read_msm_frame` at `0xc528`: calls `ioctl` at `0xc594`
   with request `0xc0585611` (AArch64 `VIDIOC_DQBUF`) and a V4L2 buffer at `sp`.
   Its timestamp fields are `sp+0x18` seconds and `sp+0x20` microseconds.
   `0xc608–0xc610` copy seconds to internal frame offset8;
   `0xc614–0xc620` multiply microseconds by1000 and store nanoseconds at offset16.
   There is no clock read, exposure subtraction or timestamp reconstruction on
   this successful dequeue/copy path. Metadata exposure is interpreted later by
   our capture preload, independently of this timestamp.

Library identities (SHA-256):

```
libqcameraoculushal.so 109b418cec3182c069d20bda7e659666ce9202d731d1253c3302c4d5ff30dd86
libqcameradriver.so   567349ac6c00ab50a3aa45038b75bc63580b899ce5090bb820a2579eceb6eb89
```

### Available kernel chain

Prefix for the following source references:
`kernel/drivers/media/platform/msm/camera_v2/`.

- `isp/msm_isp_util.c:206`, `msm_isp_get_timestamp`: `buf_time` uses monotonic
  `ktime_get_ts`, truncated to microseconds, unless `vt_enable` selects AV timer.
- `isp/msm_isp_util.c:2113`: `msm_isp_enqueue_tasklet_cmd` captures this timestamp
  while enqueuing VFE interrupt status and ping/pong status. The IRQ handler calls
  this enqueue routine; the tasklet copies `queue_cmd->ts` and at line2227 passes
  it to `process_axi_irq`. It also passes the same IRQ timestamp to other event
  handlers; the timestamp alone does not encode which IRQ bits fired.
- `isp/msm_isp_axi_util.c:4502`, `msm_isp_process_axi_irq`: dispatches write-master
  and composite completion masks to `msm_isp_process_axi_irq_stream`.
- `isp/msm_isp_axi_util.c:4285`, `msm_isp_process_axi_irq_stream`: selects
  `ts->buf_time` on the normal monotonic path (line4309), resolves the completed
  ping/pong buffer, and at line4496 passes that time to `msm_isp_process_done_buf`.
- `isp/msm_isp_axi_util.c:2276`: normal non-diverted completion passes this time
  through the buffer manager's `buf_done` callback.
- `isp/msm_buf_mgr.c:740`, `msm_isp_buf_done`: HAL dequeued buffers pass the same
  `tv` to `vb2_ops->buf_done` at line769.
- `msm_vb2/msm_vb2.c:451`, `msm_vb2_buf_done`: assigns
  `vb2_v4l2_buf->timestamp = *ts` immediately before `vb2_buffer_done`.

Source snapshot identities (SHA-256, since this extracted directory has no Git metadata):

```
isp/msm_isp_util.c     f43e5257c2e738d002465b3c10686d35b01d27c2a3cb99a51ce207166fd0c934
isp/msm_isp_axi_util.c 0f3e754ebd4bae1a19a98437b6869a395f92e481cb0adfe97c1862d61252d5d9
isp/msm_buf_mgr.c      162b3c8c4cfdaf348e914e340a70d3d258e8a460b9241f56902cfb52eecf6c27
msm_vb2/msm_vb2.c      3818887714db44be44745aa2235c4e72213b76277f54f67da9ac0c9ee404ccb6
```

### Remaining limits and next action

This proves the saved library's copy path and the available source's normal buffer
completion path. It does not independently identify every deployed kernel byte,
confirm stream flags, or eliminate alternate paths. In particular `vt_enable`
changes the time source; controllable-output/deferred completion and native diverted
buffers have other dispatch paths. No live inspection was performed to resolve
those choices. Normal timestamp alignment with host monotonic in the saved recording
is consistent with the ordinary path but is not proof of its exact configuration.

Even on the traced ordinary path, IRQ completion includes sensor readout and
transport/interrupt timing after light integration. Exposure midpoint is not simply
“buffer timestamp minus half exposure” unless those additional intervals are known.
The earlier start/end arithmetic is a conditional illustration, superseded by this
stronger provenance evidence; no correction sign or magnitude was deployed.

Next offline work can identify the exact kernel source/build provenance and stock
sensor readout sequence before constructing controlled replay offsets. Preserve
raw buffer timestamps and exposure metadata in any diagnostic experiment; do not
relabel corrected estimates as measured photon timestamps.

## Separate SO(3) visual/gyro diagnostic — 2026-10-01

`tools/tracking/audit-visual-gyro-so3.py` leaves the older axis audit unchanged.
It integrates body-frame angular velocity as ordered right-multiplied rotations,
using adjacent gyro averages and interpolating angular velocity at requested
interval endpoints. It compares the geodesic rotation error, instead of subtracting
rotation vectors. This is a piecewise integration approximation at the IMU sample
rate, not an exact continuous-time solution for arbitrary angular acceleration.

For each prepared camera it reads the actual pinhole intrinsics and rotation from
`T_imu_cam`. OpenCV's recovered rotation maps first-camera coordinates to second;
the audit inverts and conjugates it by the calibrated camera-to-IMU rotation to
match gyro integration's convention. All four cameras are checked. The currently
prepared marked calibration happens to contain the same180° X rotation for each
virtual camera, but the audit does not hardcode that assumption.

Six synthetic tests in `test-visual-gyro-so3.py` cover noncommuting rotation order,
fractional interval endpoints, arbitrary camera extrinsics, recovering a known
positive25 ms offset, rejecting integration outside IMU coverage and producing no
estimate for absent visual pairs. All six pass on Kali's existing Python/NumPy/SciPy.
The Mac interpreter documented for the capture audit lacks SciPy; no packages were
installed. For reproducible execution, use a Python environment with NumPy, SciPy
and OpenCV; the synthetic tests need NumPy/SciPy only. They were run without remote
file writes by sending the audit source as a temporary in-memory Python module and
then executing the test definitions through SSH standard input.

Real-data command (executed via SSH stdin, with stdout saved privately on the Mac):

```
python3 audit-visual-gyro-so3.py \
 /home/<user>/quest-camera-work/vio-motion-reordered-04/events.bin \
 /home/<user>/quest-camera-work/vio-motion-reordered-04/calibration.json
```

Default stride3 produces approximately100 ms image pairs; delays range−80..+80 ms
at5 ms steps. Positive delay means **images at t compared with IMU at t+delay**.
The same visual pairs must have IMU support at every tested delay. A diagnostic
reports all-pair and per-camera median/p90 error curves plus scatter of each pair's
individual best delay. These scatter quantiles are not confidence intervals:
synchronized cameras share motion and adjacent pairs share frames, while essential
matrix recovery remains vulnerable to parallax/translation and degeneracy. Gyro
bias is not estimated by this audit. Curve minima alone are not calibration.

The stride3 marked scan yielded600 sampled frames and92 accepted visual pairs
across all cameras. Private output:
`/Users/<user>/work/quest-pmos-bringup/camera-work/timing-so3-marked.json`.

| Logical camera | Pairs | Minimum median-error delay | Median error there | Median error at zero |
|---|---:|---:|---:|---:|
| All | 92 | −20 ms | 1.188° | 1.316° |
| 0 | 15 | −10 ms | 0.846° | 1.184° |
| 1 | 18 | −15 ms | 0.975° | 1.198° |
| 2 | 20 | −20 ms | 1.193° | 1.348° |
| 3 | 39 | −10 ms | 1.342° | 1.355° |

The aggregate individual-pair optimal delays have min/quartiles/max
`[−80, −45, −15, +21.25, +80] ms`; both search boundaries are hit. Camera3 contributes
42% of accepted pairs and has a nearly flat median-error curve from−20 to+20 ms.
Aggregate p90 improves from2.001° at zero to1.878° at−20 ms, but camera1's p90 instead
worsens from2.280° at zero to2.806° at its median-optimal−15 ms. Thus this scan provides
a weak preference for earlier IMU intervals, not an identified common clock offset.
Visual model error/degeneracy or unmodeled gyro bias remain viable explanations.
No timestamp correction should be selected solely from the aggregate minimum.

Verified analysis runtime on Kali: `/usr/bin/python3`, NumPy2.3.5, SciPy1.16.3,
OpenCV4.10.0; OpenCV restricted to one thread and RNG seed2026 in the audit.

A stride6 sensitivity check (~200 ms intervals) used300 sampled frames and78
accepted pairs; private output `camera-work/timing-so3-marked-stride6.json` under
the same bring-up workspace. Aggregate minimum moved to−5 ms with median1.1664°,
only0.0022° better than zero (1.1686°). Per-camera minima were−10/−25/−30/−35 ms,
with21/17/9/31 pairs respectively. Individual optima still spanned−80..+80 ms.
This change in aggregate optimum with pair spacing reinforces that these images
and this visual model do **not identify a reliable common temporal correction**.
Keep the zero-offset baseline; the results support better timestamp provenance and
controlled timing telemetry rather than deploying a visually fitted delay.

## Controlled replay timestamp hypotheses

The root investigator requested testing−10/−20 ms camera-time shifts as bounded
hypotheses, without accepting the ambiguous visual scan as calibration. New
`tools/tracking/shift-replay-camera-time.py` exclusively creates an output file,
changes only C-event header timestamps, and re-sorts by shifted timestamp and
original event order. All camera pixels, IMU timestamps and IMU payloads are
preserved. Two standard-library tests pass: exact payload/IMU preservation with
reordering and exclusive-create protection; timestamp underflow rejection before
output creation.

Private Kali directory: `/home/<user>/quest-camera-work/timing-shifts-0haG5IGf`.
Both inputs retain1798camera groups and62952IMU records. Unchanged IMU-record SHA256:
`961992ae04411c8df635805f8e9f292def24c92475f27134deab6bd799ef2d7e`.
Each camera has a later IMU bracket. The first camera precedes the first IMU in
both variants, as it already does in the baseline (baseline≈−2.386 ms;
variants−12.386/−22.386 ms). No asymmetric frame trimming was performed; conclusions
exclude treating the startup boundary as calibrated timing.

Replay controls match the separate threshold investigation: existing
`fast2-deterministic-hcg9szpv/marked-fast5.conf`, saved0005+0006-only library
`basalt-epipolar-only`, `replay-vit-bracket`, `QUEST_REPLAY_BRACKET_IMU=1`,
`QUEST_REPLAY_QUALITY` enabled. Neither realtime nor timing extension enabled;
no feature CSV argument. External timeout180seconds per run. The root verified
this IMU-bracketing replay scheduler gives byte-identical repeats for the baseline.

Both shifted runs completed1798/1798 poses. Comparison uses the same relative-time
landmarks and support analysis as the threshold experiment; paths/logs, CSVs,
`analysis.json`, analysis script and executable/library/config/input hashes are
saved in the Mac `camera-work/timing-shifts-0haG5IGf` directory. The baseline is
`fast2-deterministic-hcg9szpv/marked-fast5-*`, which matches the root's deterministic
baseline pose hash. Shift variants were each run once; no repeated-run claim is
made for them.

| Metric | Baseline | Camera−10 ms | Camera−20 ms |
|---|---:|---:|---:|
| Ready /1798 | 1569 | 1574 | 1267 |
| 20→40 s displacement | 0.526 m | 0.540 m | 0.353 m |
| 20 s→end displacement | 0.110 m | 0.119 m | 0.183 m |
| 3 s→end displacement | 0.456 m | 0.483 m | 0.397 m |
| Largest excursion from3 s | 0.787 m | 0.793 m | 0.646 m |
| Pose step p95 | 4.848 mm | 5.043 mm | 4.798 mm |
| Largest pose step | 43.253 mm | 39.121 mm | 45.171 mm |
| Longest unsupported interval after3 s | 4.367 s | 4.367 s | 14.667 s |
| Median positive features, logical0/1/2/3 | 33/9/2/8 | 34/8/4/9 | 37/9/2/4 |
| Median occupied coverage cells, logical0/1/2/3 | 9/6/2/4 | 10/5/3/4 | 10/5/1/3 |
| Median shared features | 5 | 5 | 5 |

Reject both as deployment candidates. At−10 ms metrics are mixed: five more ready
poses and smaller maximum step accompany a worse p95 step and larger endpoint
separations. At−20 ms support declines, the unsupported interval more than triples,
and the intended roughly half-metre movement shrinks to0.353 m while return
separation grows. The smaller3 s→end norm therefore is not improved tracking.
These marks are approximate user motion, not motion-capture ground truth, but the
combined observations do not justify changing the zero-offset baseline. The
SO(3) curve's−20 ms aggregate minimum demonstrably was not an estimator improvement
on the same saved recording. No runtime/calibration/firmware/device change was made.

## Corrected HAL buffer-length interpretation — 2026-10-01 bench review

The first raw-capture bench attempt safely rejected `raw[7]` low32=15. My earlier
review suggestion to treat this field as byte length was incorrect: the timestamp
trace established the copy offset, but did not establish that copied field's
meaning. Do not infer struct field semantics from proximity or expected layout.
No capture was written by that rejected attempt.

For saved HAL SHA109b418cec3182c069d20bda7e659666ce9202d731d1253c3302c4d5ff30dd86:

- `0x49c4–0x49c8` allocate80zeroed wrapper bytes. `0x49d0` stores the internal frame
  pointer at wrapper offset0. `0x49f0` returns wrapper+8.
- `0x4a08–0x4a0c` copy internal frame+0x228 into wrapper+0x40 as a **32-bit field**.
  This is returned `raw[7]` low32. There is no store to wrapper+0x44; high32 remains
  zero. Moving the assumed length extraction to high32 would also be wrong.
- Allocation callback `0x4e7c–0x4e80` copies allocation descriptor+0x10 into internal
  frame+0x228. Allocator `0x36cc` passes that descriptor field's origin as `mmap` fd,
  and `0x370c` stores it at descriptor+0x10. Therefore this field is the **fd**.
- Callback `0x4e88–0x4e8c` copies descriptor+8 into internal frame+0x238 as u32.
  Allocator `0x36c8` uses the corresponding value as `mmap` length and `0x3710`
  stores mapped pointer and length at descriptor offsets0 and8. Thus the actual
  mapped byte bound is **internal frame+0x238**, not a field copied into the public
  wrapper.
- Callback `0x4e94–0x4ea0` copies descriptor pointer0 into internal frame+0x230;
  `0x4ea8` stores the allocation descriptor pointer at internal+0x240.
- Allocator `0x3678–0x3680` rounds the requested length up to4096bytes. For307840
  bytes of metadata+pixels, the mapping is311296bytes. A real bound check must
  permit this padding while requiring at least307840bytes.

Any diagnostic relying on these private structures must be limited to the traced
library layout, verify internal/public data pointer and fd consistency, and fail
closed on mismatch. The fixed raw format still contains exactly307840bytes per
camera, with no allocation padding. This correction changes buffer-size reasoning;
the earlier timestamp-copy provenance remains valid.


## Stock Monterey exposure-bank defaults and tags — 2026-10-01

Read-only audit of the saved stock binaries, with no device interaction or source
changes. Binary virtual addresses below belong to these exact private files:

- Kali `/home/<user>/quest-camera-work/sensors-service`, SHA256
  `f2549a8c1f433385ea98faf068cfe0e66a907f1400f8c19072010a338c7c7f43`.
- Kali `/home/<user>/quest-camera-work/stock/vendor/lib64/libsyncboss.so`, SHA256
  `bd479d113eb7b37a678556461fea3197de9ee98edcdecce038494325bfa88075`.

The provider constructor copies 0xc0 bytes from VA `0x1cc40` into its config
vector (`0x65118–0x65160`). Its log tag at VA `0x160d7` is
`MontereyCameraProvider`. The entries are 64 bytes each. Parsing through ELF
PT_LOAD virtual-address mappings, rather than assuming VA equals file offset,
gives:

| Config VA | Internal type | Exposure microseconds | Gain Q4 | Tag | Camera count |
|---|---:|---:|---:|---:|---:|
| 0x1cc40 | 2 | 5000 | 48 (3x) | 1 | 4 |
| 0x1cc80 | 3 | 38 | 48 (3x) | 2 | 4 |
| 0x1ccc0 | 4 | 5000 | 48 (3x) | 4 | 4 |

Type is config+0; default six-byte tuple at config+0xc is little-endian u16
exposure, u16 gain, u8 tag and one padding byte. Count is u64 at config+0x28.
The type3 tuple is `26 00 30 00 02 00`.

The constructor iterates configs in 0x40-byte steps at `0x654a8–0x654c0`, through
`0x30fc0` and `0x312c8`; the latter copies the config at `0x31928–0x31934` and
passes it as x6 to constructor `0x38358` at `0x3199c`. That constructor retains
x6 in x21, copies the full config into object+0x30, saves its type at object+0x70
and count at object+0x78 (`0x38400–0x3841c`). The loop at
`0x38474–0x384a0` copies the six-byte default tuple once per camera into the
settings vector at object+0x140.

These are command units, not guessed metadata units: the service's exposure
setter multiplies seconds by the double **1000000.0** stored at VA `0x1c5f0`
before converting to u16 (`0x3a4e8–0x3a4f0`). Gain conversion uses
`fcvtzs ... #4` (`0x3a500–0x3a508`), establishing Q4. The setter updates only
exposure and gain, preserving each tag. The libsyncboss serializer at
`0x65a0–0x65fc` reads exposure at tuple+0, gain at +2, and tag byte at +4, with
six-byte stride. The normal entry point `0x6574` supplies bank selector0;
controller entry point `0x66b4` supplies selector1.

The service's initial settings call loads type at `0x3aef0`, compares it to2 at
`0x3af00`, and routes type2 to `syncboss_camera_set_exposure_gain_tag` at
`0x3af08`; other types use `syncboss_camera_set_controller_exposure_gain_tag`
at `0x3af10`. The update path repeats this distinction at `0x3a554–0x3a56c`
and `0x3a740`. The receive side independently corroborates the tag assignments:
`0x64ea4–0x64ee0` maps decoded low tags1,2,4 to types2,3,4 respectively.

This establishes a stock-supported short-bank initialization of **bank1:
38 microseconds, gain48, tag2 on all four cameras**, paired with scene bank0
tag1. It supports an opt-in candidate that restores that short bank once, then
updates only scene-bank exposure/gain while retaining the established scene
filter. It does **not** establish that stock always keeps bank1 short: the same
provider has the type4/tag4 long-exposure alternative, whose activation policy
was not traced in this bounded audit. Nor does it justify using frame parity as
a permanent bank classifier. Commands and observed metadata must still agree
in a bounded hardware test managed by the parent task.

The current project's historical commands set both banks to tag1, so tag1 alone
in an existing recording cannot establish which bank produced a frame. The
parent's distinct-exposure bank bench provides separate runtime evidence; that
bench is not part of this read-only source audit.


### Independent review of opt-in scene-bank candidate

Reviewed `tools/camera/stock-probe.c` SHA256
`42de077497b97baea74a7cdce14f9556195782662af9ae3ea226ea8cf68df41c`,
`run-stock-camera.sh` SHA256
`e164d587b6f3281e16721e4d4e0181ab5e2dfa90129931ca012426846ea66845`,
`camera-scene-bank.h` SHA256
`88a956213e37756ae99e157df0346661de1c8431cd80a2ebcbac5914fbd51c87`, and
`camera-feed.h` SHA256
`0969a20474c6ee2217eda2ca9c514b987940ea85fb175506e64b0e1fadb96968`.
No blocking source issue remains for the parent's bounded stationary nonraw bench.

- New behavior defaults off. Candidate restores bank1 38/48/tag2 once after a
  successful local write; subsequent exposure requests update only bank0/tag1.
  A write success is not a MCU application acknowledgement. Actual metadata must
  verify the controller tuple on all four cameras.
- Candidate command write failure and private-buffer guard failure take the
  existing cleanup path. The launcher verifies the traced HAL hash before launch.
- Candidate accepts low metadata class1 only, with metadata0x50 bit1 clear.
  Class2, class4 and other classes are rejected; it does not use frame parity or
  an exposure threshold for scene identity.
- Candidate startup/autoexposure uses a coherent quartet within1ms, each timestamp
  after the command watermark and within100ms of now. The existing publication
  threshold remains two raw sequence numbers; alternating scene/controller at
  60Hz therefore permits scene publication at30Hz without another decimation.
  Actual cadence remains a bench measurement, not a software-test result.
- RAW_CAPTURE plus SCENE_BANK_ONLY is rejected by both launcher and constructor:
  the existing raw artifact semantics reject sub1000us frames, whereas class1
  selection can legitimately accept988us quantized scene exposure.

Independent host `cc -std=c11 -Wall -Wextra -Werror` runs passed
`test-camera-scene-bank.c`, `test-camera-control-cohort.c`,
`test-camera-cohort.c`, and `test-camera-bank-diagnostic.c`; launcher `sh -n`
also passed. This review did not deploy or access the headset.


### Independent review of all-frame settled readback counters

Reviewed updated `stock-probe.c` SHA256
`9bd79f14b972bcd9a2c9f1eee3d16a813c648900728d9e778e464ec3d43a9627`
and `camera-scene-bank.h` SHA256
`6f4537fe4b1050066c6c866a921e466ef08f4068d407d6ff8fee2dc846d0ade2`.
The new diagnostic compares every selected class1 frame before publication
selection with requested bank0 settings. A capture timestamp must be strictly
more than100ms after the last command watermark; old queued frames and the
100ms boundary are excluded. Exposure tolerance is inclusive ±19us and gain
must match exactly. Timestamp and exposure subtractions are ordered to avoid
unsigned underflow. The matching work runs only with scene-bank mode and
diagnostics enabled; it adds fixed counters, not image work or per-frame logging.

The host scene-bank and control-cohort suites pass independently under
`cc -std=c11 -Wall -Wextra -Werror`. No blocking source finding. Hardware
interpretation still requires enough eligible frames from both observed scene
sequence phases: aggregate counts catch mismatches from either phase, but do not
themselves report phase coverage. Local writes are not application acknowledgements.
No device interaction or code changes were made for this review.


### Independent review of target acknowledgement controller

Reviewed `stock-probe.c` SHA256
`7f7c6486394f7e920348a5f2e6ccb0ada04b8c248a66e9cd9ea721ce7888795f`,
`exposure-ack.h` SHA256
`377258553b463f1c32b6adf6490d8ed9ec7f9264c437d186c83e3d283651b52b`, and
`test-exposure-ack.c` SHA256
`989c9a31b96366368b97f09adc403a3684893fa51ed930a7b1f171fb840e37aa`.
No remaining source blocker for the parent's bounded bench.

The opt-in controller holds its entire requested four-camera target until two
fresh matching cohorts are observed after100ms settling. Each camera must advance
exactly two raw sequence values for the second support, covering alternating
scene phases; a +4 gap starts support at one again. Duplicate or out-of-order
quartets cannot confirm or trigger another policy evaluation. Cohorts must be
within1ms and each capture timestamp within100ms of now. Stale input expires
confirmation. Backward host-time polls do not mutate state.

Retries resend the same target at500ms intervals; a10s deadline stops automatic
writes without stopping capture. Retry completion does not extend that deadline.
Two later matching cohorts can recover confirmation. Policy evaluation requires
new accepted input and500ms cadence; an unchanged target causes no write.
Integration polls even empty/partial feeds in scene-bank mode, so absent input
cannot suppress its retry/timeout mechanism. Default mode retains its existing
control path, and raw-capture incompatibility remains enforced.

Independent host Werror tests passed for exposure acknowledgement, scene-bank
packets/readback, control cohorts, and exposure policy. The helper author's
sanitizer validation is separate evidence. These tests establish controller
logic, not that hardware applies every command; actual phase/readback stability
remains a hardware acceptance condition. No device access or code edits were
performed for this review.


### Failed target-ack hardware bench: controller application is unverified

Read-only audit of private Mac
`/Users/<user>/work/quest-pmos-bringup/camera-work/scene-ack-01.log`, SHA256
`daf8899269627acbd14b143a1f189e55545586afcc882a4601feceb3d9e51fbc`.
The initial command trace reports successful local writes for bank0
12000us/gain16/tag1 and bank1 38us/gain48/tag2, followed by bank0-only retries.
This does not establish command application by the MCU.

Camera2 does not maintain a normal alternating long/short stream: of597 bounded
trace frames,595 report38us/gain48/class1. Only one startup frame reports
12008us/gain16/class1 and one class0. Cameras0/1 each show298 long class1 and298
short class2 frames plus one startup transition. Camera3 similarly alternates
297 long class1 and297 short class2 plus two startup transitions. Thus the
camera2 failure is more than simply both phases sharing a tag; its traced stream
stays short. The cause is not established by these observations.

At final diagnostics, camera2 has5343 retained class1 frames, zero controller
stock frames, zero settled target matches and5224 settled mismatches. The other
three cameras each have2613 settled scene matches and zero mismatches, plus
2670–2671 stock controller frames. Publication remained blocked by the cohort
condition, and scene target acknowledgement timed out rather than retargeting.

Proposed next diagnostic requires actual fresh class2/38us/gain48 evidence from
all four cameras before starting scene automatic control or publishing its feed.
A bounded controller-first retry can hold the same stock bank1 tuple at500ms
cadence, stop writes after10s without stopping observation, and only advance on
valid readback. Do not relabel a short class1 frame as controller to satisfy this
gate. Sequential controller verification before scene-target initialization also
avoids the initial nearly back-to-back bank writes, but this is a controlled
hypothesis test, not an established explanation of the failure. Helper/integration
review is separate from this log audit; no device was accessed here.


### Controller-first staged candidate review

Reviewed final `stock-probe.c` SHA256
`dcc17c56047d70fe0996dd066b0077298a27d0c37d23786ecd21c8c7673f1e51`
and `controller-ack.h` SHA256
`f89e34a1fd6c3aa4bd74cb54709a7c92183742240a3a43814d2869762e59ef5a`.
The earlier independently tested helper SHA794c64a3ab79f49542563b5aff1be5e5bf9d8931bc34faa48f80d31ffaf96656
has the same functional code; the later edit documents strict gap behavior.

The scene-only initial command writes bank1 alone. All four cameras must provide
two fresh exact38us/gain48/class2 observations with advancing timestamps and
sequence delta2 before any bank0 command. Ordinary intervening class1 frames do
not erase controller support. Retries are limited to the same bank1 tuple,
500ms command spacing and a10s deadline; later valid readback can still recover.
Default mode retains its two-bank request path.

Publication waits for controller confirmation and initial/recovery scene target
confirmation. That scene latch stays set through ordinary subsequent AE target
transitions, avoiding unnecessary publication holes. Class1 is independently
mandatory, and scene metadata must be within981..12019us and gain16..240 before
copying pixels into the retained feed. This includes988us quantization of a1000us
request while excluding the observed mis-tagged38us stream.

Controller loss immediately unlinks the published feed, clears retained frame
sequences/timestamps, resets scene acknowledgement and its startup latch, and
quarantines publication. Controller recovery initializes a new scene request;
old image data cannot be republished through the cleared feed metadata. Initial
quarantine also unlinks an old feed artifact.

Independent Werror suites passed for controller acknowledgement, exposure
acknowledgement, scene-bank metadata/packets and control cohorts. No source blocker
for a bounded stationary bench. Known conservative limitation: a controller
sequence gap of4 drops confirmation even if prior evidence is younger than100ms,
which can interrupt publication on a dropped frame; the parent explicitly
accepted this for the initial bench. This review made no device calls or source
edits.


## DMA/cache coherency audit — saved stock binaries, 2026-10-01

This is a read-only source/binary investigation prompted by frozen metadata
bytes during the staged bench. It establishes a different CPU-access path in
stock; it does not yet prove cache incoherency is the failure's cause or validate
an ioctl-based remedy. All addresses are virtual addresses in exact saved ELFs.
The previously recorded HAL/driver hashes apply. Additional private Kali binaries:

- `stock/vendor/lib64/libqcamerahal.so`, SHA256
  `fc8c296d93d71c3871111523572722555108ef62c39f3f691ae76fa6b5747ff2`.
- `stock/vendor/lib64/libcamerahal.so`, SHA256
  `662555e7a907951570f711f862da0122e9e62f7b0800b5f08bd8f53d0106a880`.

### Allocation and dequeue evidence

The low-level HAL allocator at0x3638 rounds to4096bytes and supplies an allocation
request with length, alignment4096, heap mask0x02000000, and flags1. The last two
u32 values come from the eight bytes at VA0x1408 (`0x3670–0x3688`). It calls
ioctl0xc0204900 at0x36a0, obtains a shared fd using0xc0084904 at0x36c0, then mmap
with PROT_READ|PROT_WRITE and MAP_SHARED at0x36e0. The saved source subset lacks
the ION UAPI headers, so the numeric flags are primary evidence; assigning names
or choosing replacement flags requires the corresponding ABI verification.

The driver's `mm_stream_read_msm_frame` calls VIDIOC_DQBUF at0xc594, copies driver
sequence/timestamps into its frame descriptor, resets descriptor+0x24c to0, and
calls `mm_stream_handle_cache_ops` at0xc63c with argument2=1. Thus the dequeue path
**does contain cache-operation handling**; absence of a top-level call is not the
problem established by this audit.

The cache handler at0xc6a8 loads callbacks from stream+0x258/+0x260/+0x268. It
reads mode from the stream-info pointer at stream+0x10, offset0x2c0. Mode0 clears
the frame's cache-operation flags and reaches the no-operation return
(`0xc6f0–0xc6f8`, `0xc790–0xc798`, `0xc7ec`). Mode2 requests operation flags3 on
its dequeue path; other modes consult existing frame flags. Callback dispatch is
at0xc7a8–0xc7b0.

The HAL stream setup supplies all three callbacks unconditionally:
0x5110,0x5190,0x5210 (`0x44dc–0x4550`). Driver `mm_stream_config`
0xbd7c copies them into the corresponding stream callback fields. The HAL clears
its712-byte stream-info structure at0x453c and initializes selected fields but
leaves offset0x2c0 zero in this setup path. Runtime mutation of this mapped
stream-info or additional kernel cache handling has not been excluded.

The HAL callback wrappers look up the allocation descriptor by buffer index,
then call0x39dc/0x3a84/0x3a90, respectively. Those feed embedded command values
0xc0184d01/0xc0184d02/0xc0184d00 to the common routine0x39e8. That routine constructs
a descriptor containing allocation handle, fd, virtual address, offset0 and full
mapped length, and calls ioctl0xc0104906 at0x3a48. These exact existing binary
operations are evidence, **not authorization or a recommendation to issue them**;
operation naming and ABI semantics still need matching headers/backend review.

### The NULL start argument and stock CPU access

Probe `qcamera_start_sensor(...,4,NULL,1)` does not omit a cache callback. The
fifth argument x4 is retained at0x4108, passed through0x4150, and used by the shared
setup at0x4578–0x4598 as an optional array copied with count*4 bytes into
stream-info+0x1a0, with presence flag+0x198. Its non-null path imports external
allocation fds; NULL selects HAL-owned allocations. The final argument's low bit
increments the high32 dimension word at0x411c–0x4128; it is not a cache mode.

Stock's higher `libqcamerahal.so` creates ImageBuffer/GraphicBuffer allocations
at0x5ecc, obtains their native-buffer fd at0x5ed8–0x5ef8, and passes its fd array
as x4 to qcamera_start_sensor at0x6604–0x661c. Its streaming thread dequeues at
0x6a24, finds the returned fd in its buffer map, then invokes its consumer at
0x6ac8. The saved `libcamerahal.so` CPU consumer locks a GraphicBuffer with usage
0x33 at0x737c, passes the returned CPU pointer into a callback at0x7424, unlocks
at0x7454 and returns the buffer through another callback at0x746c.

Therefore stock has an explicit GraphicBuffer CPU lock/unlock boundary that the
probe's direct raw mapping access lacks. This makes coherency a supported next
investigation, especially alongside frozen second-cache-line metadata reported
by the separate log audit. It does not yet identify the precise missing cache
operation: saved libimagebuffer/libui/gralloc and matching ION headers were not
present in the inspected private subset. Trace that backend or matching kernel
ABI before selecting a bounded CPU-access experiment. Do not alter bank policy
again based on possibly stale metadata alone, and do not assume sequence modulo4
identifies a particular allocated fd without logging that identity.


### Follow-up: owner gralloc and exact kernel confirm invalidate semantics

The parent subsequently supplied read-only copies of the owner's additional
libraries under private Kali `cache-audit-owner-libs/`. The limitation above
about unavailable backend/header evidence is superseded by this follow-up.
`gralloc.msm8998.so` SHA256 is
`0da3b6944c2dd98a2b422f3769b56fc5fa14bab1664deccec9d0c3ca4130b0fe`.
Other supplied libraries: libimagebuffer
`705f66b2220207d0bc20cc40b4635cbe57048940b5c61f9b894c5acdf4ca3157`,
libion `71c81c8313450928185e5a992bf78c5fc309592205606d7477ff56af19bd8426`,
and libui `33788a7bd262c904b8013ccd4eb5767dab24b7c00ecca061dea132b8a8d61141`.
These remain private binary artifacts, not redistributable repository content.

Owner gralloc `BufferManager::LockBuffer` at0xbb08 checks CPU consumer usage and
handle flags masked with0x288 at0xbb9c–0xbbb4. On its eligible cached CPU-read
branch it calls `Allocator::CleanBuffer` with operation2 at0xbbd4.
`IonAlloc::CleanBuffer` at0xa050 maps operation2 to embedded command0xc0184d01
at0xa0dc/0xa100, then invokes outer ioctl0xc0104906 at0xa120. This is the same
operation encoded by low-level HAL callback0x5110. Unlock uses operation1 on
buffers marked CPU-written (0xbd30–0xbd48), which maps to0xc0184d00; it is not
interchangeable with pre-read invalidation.

The exact project kernel source, independently located by the lowlight agent,
was extracted privately to
`/home/<user>/quest-camera-work/ion-kernel-audit/oculus-linux-kernel-6929f734ce0e602018790ff3a52dc7bad646af60`.
Its `drivers/staging/android/uapi/msm_ion.h:153` defines the24-byte AArch64
ion_flush_data layout: handleu32, fdint, vaddrpointer, offsetu32, lengthu32.
Lines190–208 name embedded operations4d00 CLEAN,4d01 INVALIDATE,4d02 CLEAN_INVALIDATE.
`drivers/staging/android/ion/msm/msm_ion.c:218–219`,246–247 and319–320 route
INVALIDATE to `dmac_inv_range`. `drivers/staging/android/ion/ion.c:566` forcibly
adds ION_FLAG_CACHED_NEEDS_SYNC at allocation; its lines127–128 exclude that
flagged case from the automatic fault-user-mapping path. This supports the
manual CPU synchronization requirement for the HAL's cached allocation path.
The parent verified the running kernel release as4.4.205-perf; any earlier3.18
assumption must not be used to choose this ABI.

Conclusion: stock's CPU lock layer performs a verified invalidate operation on
eligible cached buffers, and the probe's raw mapping access omits that boundary.
A narrowly bounded pre-read invalidation experiment using the verified buffer
identity, full mapped range, exact ABI and fail-closed return handling is now
supported by source evidence. It must precede all metadata and image reads.
This is still a proposed experiment: neither successful cache maintenance nor
correction of frozen metadata/image data has been demonstrated by this audit.
Generic ION_SYNC is not a substitute inferred from its name; the separate exact
kernel audit found its operation directed toward device synchronization.
