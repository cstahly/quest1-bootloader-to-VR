# Camera / passthrough / playground roadmap

## Owner correction: physical movement only

The owner explicitly rejected joystick locomotion. Its unshipped code has been
removed. Priority is camera/IMU positional tracking for actual leaning, crouching,
and walking. `scene_position` remains a rendering input for a future measured pose;
it must never be driven by thumbsticks. Current on-device build remains the accepted
drawing/camera scene, with rotation-only tracking. No joystick build was deployed.

## Current priorities — owner feedback

The owner enjoyed drawing and reported that the composite passthrough is incorrectly
stitched. The fixed-depth immersive view is **not accepted**. No physical question
is pending. A saved drawing (11,868 bytes) was backed up before further changes.

1. **In progress:** move the composite to a small floor panel beside/in front of the
   cube. Use one combined image for both eyes on that panel, eliminating divergent
   per-eye camera selection. Keep the four separate camera stations available.
2. **Open:** correct stitching and near-object parallax. Factory calibration and a
   fixed three-metre projection are not sufficient evidence of correct stereo.
3. **In progress:** Touch controller aiming and usable controls; investigate actual
   controller motion data. Current drawing aims with the head, not a tracked hand.
4. **Priority:** physical camera/IMU positional tracking. No joystick locomotion.
5. **Open:** camera-assisted positional tracking (6DoF), depth-aware stereo, optical
   flow/particles and other camera-driven effects.

Completed foundations: all four live camera feeds, camera boot service, VR camera
panels, head-aimed saved 3D strokes, edges/motion-trail effects, watchdog controls,
and real framebuffer captures. Hold-to-peek was implemented but its visual result
needs replacement/correction per the feedback above.

The chronological checkpoints below retain earlier results and superseded plans.

Owner authorized the complete roadmap on 2026-09-30. Start with item1, but preserve
all items across context compaction. The headset is the owner's only device.

- [ ] **1. Live camera window in VR.** Establish actual capture from an outward-facing
  monochrome camera; identify formats, resolution, timestamps, exposure and cadence.
  Present a camera panel alongside the known-good scene. Measure capture/render rate,
  dropped frames and latency where measurable; do not invent sensor-to-display latency
  from render-loop time. Save an actual framebuffer screenshot.
- [ ] **2. Hold-to-peek passthrough.** A separate control shows the room temporarily;
  preserve index-trigger hold2s recovery renewal. Handle release, disconnected input,
  stale frames and capture failure visibly, without leaving a misleading frozen view.
- [ ] **3. Calibrated stereo passthrough.** Map camera intrinsics/extrinsics, pair and
  synchronize camera frames with IMU timing, correct camera distortion and display
  optics, and account for camera-to-eye offsets. Human comfort/alignment confirmation
  requires the wearer; screenshots alone cannot establish it.
- [ ] **4. Camera-assisted positional tracking.** Explore visual-inertial tracking
  against the existing orientation-only baseline. Verify real translation, drift,
  scale, relocalization and frame conventions; do not call3DoF rotation6DoF tracking.
- [ ] **5. Camera-driven playground effects.** Let the owner draw weird shit from
  camera data: edges, brightness, motion/optical flow, trails, particles, silhouettes,
  and experimental mappings. Provide a reusable frame/metadata input interface so
  individual effects can change without rebuilding camera bring-up.

Additional future possibilities discussed: QR/marker interactions and optical hand
tracking. They are not substitutes for completing the five requested items above.

## Execution and finish requirements

- Device work authorized once reconnected. Prior power-off restriction is superseded
  by the owner's explicit request to plug back in and continue this roadmap.
- Keep hardware watchdog and the trigger-renewable software recovery guard active.
  Preserve Wi-Fi and the accepted scene; experimental capture must be reversible.
- No desktop, bootloader/splash/modem/NV/factory-calibration flashing. Calibration
  sources may be read and private copies analyzed; never overwrite factory data.
- Capture a screenshot of the actual resulting scene, report tested vs untested
  behavior accurately, leave a working boot default, and shut down at stopping point.
- USB-attached poweroff causes a firmware charger boot, currently normal pmOS startup.
  Final full shutdown requires unplugging USB and issuing poweroff over Wi-Fi.
  Do not loop poweroff/reboot or silently claim the device is off while it restarted.
- Physical questions require real replies. Preserve the scene while the owner checks.

## Initial state

Camera capture has NOT been verified in this port. Existing mdev service notes that
camera drivers probe, but that is not evidence of usable frame capture. First work
is read-only device/sysfs/media inventory plus inspection of kernel and owner stock
camera libraries/configuration. Quest1 sensors are monochrome; RGB passthrough is
not available from them. Stock Meta modern camera APIs are not a pmOS camera driver.

Known-good boot services: oculus-wifi, oculus-recovery-guard, oculus-test-scene.
Scene/HUD and trigger renewal verified; details in TEST-SCENE-AUTOSTART.md.
Private working artifacts: /Users/<user>/work/quest-pmos-bringup.
Build host: <user>@<BUILD_HOST_IP>, ARM rootfs under /home/<user>/pmos/work.
No camera changes deployed yet. Headset absent from USB at the initial check.

## Bring-up checkpoint: all four sensors queried

Normal boot exposes four `msm-sensor` nodes (`video3`–`video6`). Charging boot has
`androidboot.mode=charger`; the stock kernel deliberately skips camera probing in
`syncboss_camera.c`. A normal guarded reboot resolves that without changing firmware.

The small Bionic preload `tools/camera/stock-probe.c` successfully loaded the owner's
`libqcameraoculushal.so`, opened its HAL, opened/query/closed each of four sensors,
and closed the HAL cleanly. All four report640×480. This is **not yet frame capture**.
Probe is run with15s alarm/20s outer timeout inside `/run/quest-camera-root`: stock
code/proc/sys read-only, data in RAM, explicit media/video/ION device nodes, no block
devices or factory/NV partitions. `prepare-stock-root.sh` builds this diagnostic root.
Private evidence: `camera-work/probe-first.log`, `normal-query.log`, stock library
archive and disassembly under `/Users/<user>/work/quest-pmos-bringup`.

Recovery guard now has explicit root `--renew` (SIGUSR1), renewing to five minutes
at an active maintenance checkpoint. No automatic keepalive. Expiry wins over late
renewal; worker failure still recovers. Process tests pass. Control requests require
the exact same executable inode as the live guard, so upgrades cannot accidentally
signal an older process lacking its handler. Verified refusal against the old live
guard. New binary installed atomically; a fresh normal boot is in progress to activate.

### Actual capture checkpoint

Fresh-boot guard `--renew` verified on device, watchdog disable remains0. All four
OV7251 sensors now deliver640×480 8-bit frames simultaneously:181 frames/camera
over approximately3seconds, with matching sequence numbers. Cameras0/1 share a
timestamp,2/3 share one; pairs differed about13–17µs in these samples. These are
driver buffer timestamps, not a verified exposure-time calibration.

Stock runtime camera commands reconstructed from owner's libsyncboss and sensor
service (no proprietary source committed): probe40→response70 mask0x0f; set property
command3 with property140/bpp8; init46 payload camera COUNT4; stream44 count4;
stop45 count4→status125; deinit47 count4→status125; release41→status125.
Query command2 property74 reports OV7251 parameters. Buffer is640×481, first row
sensor metadata; image data follows at+640. All start/stop/release acknowledgements
succeeded. Captures in private camera-work/{probe-four.log,four-frames.tar}.

Exposure default metadata was2lines×19µs, hence initially tiny. Legacy exposure
command42 did not change it in our test. Current stock tagged command54 (four u16
exposure-us, four Q4 gains, four tags, controller flag), after setting frame-tag mode
property141=1, successfully changed exposure to211lines≈4009µs. Images remain near
black level4; asked owner whether cameras face a lit room. Await reply; don't claim
usable passthrough yet. Current private evidence probe-tag.log / four-frames-tag.tar.

Bionic preload MUST link explicit dependencies `-lc -ldl` against owner's stock
Bionic libraries. Without DT_NEEDED libc, its constructor initially saw no environment,
which skipped MCU setup even though later code saw flags after dlopen. Corrected and
verified. Probe remains a bounded RAM-only diagnostic; camera autostart not installed.

### Live-feed / world-panel checkpoint (2026-09-30)

All four cameras passed a60-second simultaneous capture/clean-stop test, about3575
frames each. Owner repositioned headset and switched on lights; real room images
are now visible. Tagged exposure12000us reads back12008us, gainQ4=240. Short38us
controller frames sometimes remain interleaved; the preview producer now rejects
frames with exposure<1000us rather than displaying alternating dark images. Raw
accepted values remain unchanged; gamma/black-level adjustment is display-only.

`camera-feed.h` defines an atomic307328-byte, four-camera320x240 raw-preview feed
with per-camera driver timestamps, counters, exposure, gain, mean and publication.
The renderer rejects malformed or stale feeds (>500ms), replacing old views with
blank grey/red frames. Driver timestamp age is not calibrated photon latency.

A real full-framebuffer capture showed the first live four-camera mosaic in both
eyes, grid/cube/HUD and Wi-Fi. Private artifact: camera-work/live-composite.png.
That version was56fps, so do not claim the camera renderer retains72fps. The grey
squares previously displayed were explicitly synthetic layout QA, not camera data.

Owner now requests camera views as furniture around an arc LEFT of the cube,
not attached to the HUD. Implemented four separate3m-radius stations at-35,-67,
-99,-131 degrees, with legs to the floor and1–4 identifying top ticks. Same tracked
world transform as cube; camera orientation/extrinsics still uncalibrated. Current
new renderer uses a12x8 tessellated plane per sensor, perspective-correct sampling,
stock RGB lens warp,2x2 software pixel blocks. Build/test/device validation pending.
This supersedes the head-locked camera mosaic. It remains3DoF, not6DoF.

The new OpenRC camera service is staged in source; not yet installed/enabled.
No bootloader, factory calibration, NV, firmware, or camera kernel changes made.
All remaining roadmap items (hold-to-peek, stereo rectification, positional tracking,
creative camera effects) remain open. Finish the world-panel deployment first.

### Factory calibration and spatial playground (2026-09-30)

Android's actual `/persist` data lives on the **private** partition, not the GPT
partition named `persist`. A read-only 64 MiB copy is saved with mode 0600 at
`camera-work/owner-private-readonly.img`. Do not commit or upload it. Targeted
read-only extraction found `/calibration/camera_calibration_v2.json` and the legacy
camera calibration. The four OV7251 cameras have global shutters, 640 × 480 images,
PinholeSymmetric projection, Fisheye62 distortion (six radial and two tangential
coefficients), and full `DeviceFromCamera` transforms. All rotation matrices passed
orthonormality and determinant checks. Factory calibration was never modified.

Fisheye62 equations were checked against Meta's primary documentation:
https://facebookresearch.github.io/projectaria_tools/docs/tech_insights/camera_intrinsic_models

`preview-calibration.py` produces an offline panorama spanning 240 × 140 degrees
at a chosen depth of three metres. The real captured room appears with correctly
aligned camera orientations, but nearby objects have obvious parallax seams.
This is **not depth-aware stitching**. The actual preview shown to the owner is
`camera-work/factory-panorama.png` in the private working directory.

`build-passthrough-map.py` combines camera calibration with the stock display lens
mesh, accepted optical centres, and 63.5 mm virtual IPD. The private lookup uses
4 × 4 display-pixel blocks and three source indices per block. Its 24-byte header
contains magic 0x51504153, version 1, width 2880, height 1600, step 4, and channels 3.
The remaining 400 × 720 × 3 unsigned 32-bit indices address four 320 × 240 images;
UINT_MAX means no coverage. This is a fixed three-metre depth approximation.
The generated file is installed at
`/var/lib/monado/monterey/passthrough-map.bin` with mode 0600.

World-panel rendering measured about 45 fps initially, 60 fps after incremental
barycentric interpolation and reciprocal reuse, and 62 fps with the two eyes drawn
concurrently on performance cores 4–7. The GPU is not used. Textured geometry clips
at the cached optical boundary; existing scene wires retain the original fallback.
All four stations passed both-eye visibility tests. Actual camera composites were
shown separately from synthetic test images.

`oculus-camera` is installed and enabled in the default OpenRC runlevel. It requires
local mounts, the recovery guard, and Wi-Fi's read-only stock mount; it starts before
the test scene. Its restricted runtime root remains in RAM. Clean stop/start passed
with successful MCU acknowledgements. Response waits now use a real one-second
deadline: counting ten records had timed out early in a busy IMU stream. Live capture
requires all four sensors to start. No firmware or hardware-watchdog changes.

The owner requested drawing in 3D. New playground controls and implementation:

- `spatial-ink.h`: head-aimed laser pen; hold A/X to draw, release to save a stroke.
  A/X uses BTN_LEFT only when the separate physical BTN_TRIGGER is clear. The stick
  changes drawing depth from 0.4 to 8 metres. A blue crosshair becomes pink while
  drawing. Up to 2,048 points are saved atomically on release or clean exit to
  `/var/lib/monado/monterey/spatial-ink.bin`. Coordinates use the cube's world frame.
  This does not claim tracked hand position or headset positional tracking.
- `camera-controls.h`: independent read-only evdev reader, without exclusive grabs.
  Index-trigger watchdog renewal is unchanged. Hold grip/B to peek; click a stick
  to cycle raw camera images, edges, and motion trails on the four panels.
- `camera-peek.h`: a combined view from the real cameras using the private fixed-depth
  stereo lookup. Stale frames become black instead of freezing the room image.
- Effects leave the raw feed unchanged. Motion trails use temporal brightness
  differences; they are not optical flow or positional tracking.

Validation: build passed with warnings as errors; tests passed for all four stations
in both eyes, stale/truncated feeds, full-panel lookup bounds and source selection,
world-coordinate pen strokes, stroke separation, and save/reload. The final renderer is now deployed. Drawing, peek, and effects are installed.
The runtime reports about 84 fps with the cameras active; the last two 240-frame
submission windows were 83.8 fps, p95 about 22.3 ms, no intervals over 25 ms. This
is a stationary measurement, not yet a wearer motion/smoothness verdict.

Still open: wearer verification of drawing/peek controls, depth-aware stereo and
seam handling, camera-assisted positional tracking, broader creative effects,
performance beyond roughly 62 fps, final screenshot and requested orderly shutdown.

The owner has been asked to try A/X drawing. **Answered:** owner enjoyed drawing but rejected composite stitching. It is currently running with the guard explicitly renewed.
Actual new composite saved privately as `camera-work/playground-composite.png`.
No drawn strokes were saved at the instant of that capture. Do not claim controller
interaction is verified until the owner replies or actual saved strokes appear.


<a id="archived-lowlight-audit"></a>

## Archived investigation: lowlight-audit (2026-10-01)

Historical audit, retained verbatim. The final checkpoint supersedes active/pending wording.

# Low-light capture and feature audit — 2026-09-30

This is an offline source/evidence audit, not an accepted tracking fix. No headset
connection, deployment, camera command, firmware change, or shared Kali source edit
was performed. Optional diagnostic source changes were added afterward as described
below; nothing was deployed. The owner rejected r6 GFdfKL; retain four-core r5 as the better tested
candidate. GFdfKL changed both positional prediction and camera prefilter, so it
cannot isolate either variable.

## Findings and evidence

1. **Shared shutter can remain controlled by a dark/covered camera.**
   `tools/camera/exposure-policy.h` computes each camera's desired exposure product,
   chooses the largest requested shutter, then clips individual gains to 16–240.
   With all shutters at 8000 us, camera 0 dark at gain 240, and other cameras clipped
   at gain 16, twenty policy iterations leave every setting unchanged. The bright
   cameras cannot reduce gain further. This is a demonstrable dynamic-range tradeoff,
   not evidence that this exact state occurred in a wearer trial. A covered or
   textureless dark camera should not necessarily set the shutter for all useful
   cameras. This cannot always be solved within the current hardware limits: for a
   desired exposure product P, a feasible common shutter t must satisfy
   P/240 <= t <= P/16, intersected with 1000..8000 us. In the characterization,
   the dark camera asks for capped P=1920000, requiring t>=8000; bright cameras
   ask for P=96000 after the bounded 0.75 dimming step, requiring t<=6000.
   The feasible intersection is empty. A replacement must explicitly balance loss
   across views and retain useful dark-camera support, rather than silently
   sacrificing every dark view. This is a constrained policy choice, not an
   arithmetic bug. Independent shutters are not a safe immediate workaround: earlier native
   evidence recorded roughly 18 ms cohort skew when shutters differed.

2. **The exposure objective is brightness, not tracking information.**
   The 70th percentile over a central sampled rectangle excludes fisheye borders and
   isolated lamps, but ignores clipping in up to roughly 30% of ROI samples. A host
   test with 29% saturated ROI samples and the remainder at 80 produces no change.
   Conversely, a uniformly dark wall can request maximum gain/shutter without adding
   usable corners. This may waste dynamic range and increase blur for other cameras.
   It does not justify replacing the policy with ordinary global auto exposure.
   A candidate policy needs per-camera clipping fraction, spatial gradient/feature
   distribution, and evidence of occlusion, plus a common-shutter compromise. Keep
   current support gates; brightness and corner count alone are not tracking truth.

3. **Scene-frame classification has a quantization boundary to verify.**
   `stock-probe.c` discards exposure metadata below 1000 us as controller frames.
   Metadata is decoded in 19 us units, while the policy can request exactly 1000 us.
   Floor quantization would yield 988 us and discard those scene frames; ceiling
   quantization yields 1007 us and preserves them. The MCU rounding convention is
   NOT established by this audit. The 1000 us minimum also sits on the exposure
   validator boundary. Verify actual command/readback mapping before changing the
   classifier: accepting controller frames would corrupt capture. A conservative
   policy minimum with at least one quantization-step margin is a testable candidate,
   not a deployed correction. Startup exposure verification similarly waits forever
   if requested exposure cannot be matched within 19 us; it currently logs retries
   but has no distinct published "auto exposure has not started" state.

4. **Prefilter changes optics sampling without shifting the nominal map center.**
   `camera-resample.h` uses a centred 3x3 binomial kernel at the previous (2x,2y)
   sample positions. Interior coordinate centers are preserved; boundaries clamp.
   The filter suppresses alias/noise but also weak high-frequency contrast. The live
   VIO path then performs the existing bilinear rectification, another interpolation.
   Calibration-center tests therefore do not demonstrate equal feature retention or
   equal motion tracking. A full-resolution recording is required to reproduce both
   prefilter choices faithfully; filtering a previously decimated 320x240 recording
   is not an equivalent A/B test. The reported additional capture cost (~0.12 CPU
   core) is not by itself a tracking-quality result.

5. **The detector still requires absolute intensity contrast.**
   Read-only inspection of Kali Basalt `src/utils/keypoints.cpp` shows conversion
   from internal uint16 back to 8-bit (`>>8`) and FAST thresholds 40,20,10,5 under
   current settings. `optical_flow/patch.h` normalizes patch intensity by mean for
   alignment; that normalization does not recover absent detections. Thus low-light
   contrast/noise affects detection separately from photometric patch matching.
   Lowering FAST thresholds or applying contrast gain might produce more noise
   tracks, so compare persistent geometric inliers, spatial coverage, motion scale,
   drift and processing tails—not merely feature count or LIVE fraction. Existing
   0007 upper-pair seeding changed motion scale and produced long native stalls;
   this audit does not accept it.

## Private artifact inspection

The Mac `camera-work/filtered-feed.bin` contains 244 text bytes followed by a valid
307328-byte camera feed. Validate magic, dimensions and exact remaining length before
parsing; reading the whole file as a feed produces nonsense. Its actual metadata is
12008 us / gain 240 on all four cameras, and sampled ROI p70 is 216,255,172,124.
The four whole-image saturated fractions are approximately 12.5%,27.6%,4.9%,16.7%.
The text prefix reports sequence 3, so this is an early capture, not proof of the
settled automatic policy or GFdfKL exposure. In particular, do not use this artifact
as evidence that the current final feed is underexposed or that auto exposure failed.
No raw images or calibration were copied into the repository.

## Reproducible host characterization

`tools/camera/test-lowlight-policy-audit.c` documents the three policy edge cases
above and passes with:

```sh
cc -std=c11 -O2 -Wall -Wextra -Werror tools/camera/test-lowlight-policy-audit.c -o /tmp/quest-test-lowlight-policy-audit
/tmp/quest-test-lowlight-policy-audit
```

These are characterization assertions of current limits, not a claim that hardware
passed. A deliberate policy correction should update these assertions and add its
own desired-behavior regressions. Exposure-policy behavior is unchanged.

## Next controlled work

- Capture telemetry is now implemented below for exposure, startup state, p70,
  clipping, rejected short frames, and publication counters. Gradient coverage remains
  future work. Record these alongside estimator support and latency on a later run.
- Before another wearer comparison, explicitly hold capture filter, camera rate,
  library, CPU affinity and scene fixed. Compare prediction disabled/enabled only;
  separately compare prefilter OFF/ON with prediction disabled. Do not infer success
  from the rejected r6 trial's high ready count.
- Offline: exercise a candidate common-shutter policy on dark/covered, mixed-brightness,
  clipped and quantized synthetic inputs. Preserve bounded shutter/gain, synchronize
  all four shutters, and avoid letting an uninformative covered view dominate.
- When hardware testing is appropriate again, verify applied short-exposure metadata
  and the scene/controller classification boundary without a wearer. Save full-resolution
  cohorts plus metadata for reproducible prefilter comparisons. Never change MCU
  firmware or loosen camera-cohort/estimator quality gates to make a metric pass.

Actual low-light motion accuracy remains unresolved. No physical prompt is needed
tonight for the offline work above.


## Optional capture telemetry added offline

`CAMERA_DIAGNOSTICS=1` enables `stock-probe.c` diagnostics; the default is 0. It
emits at most one five-line `CAMERA_DIAG` group each second, with no catch-up bursts.
No statistics are computed and no new diagnostic lines are emitted when disabled.
It does not change exposure decisions, frame classification, camera-feed ABI, or
publication selection. This adds observability, not a tracking fix.

The summary records monotonic time, exposure control/auto flags, startup verification,
last request time, candidate publication sequence, successful atomic-publication
count, and prefilter flag. Four camera lines record requested and retained-scene
exposure/gain, latest raw metadata exposure/gain (including rejected frames), scene
sequence/timestamp, central ROI p70, exactly-saturated sample count and denominator,
and cumulative rejected-short-frame counts. The last raw metadata is included so a
short-frame rejection problem remains visible even when retained scene data stops
updating. Scene sequence 0 means there is not yet a retained scene; its statistics
are not meaningful. `request_ns=0` means the configured requested settings have not
yet been sent. Request time means a send was attempted, not MCU acknowledgement;
readback metadata remains the evidence for application. Metadata from the latest
raw frame can be an ordinary interleaved controller frame, so interpret the counters
and retained-scene timestamps together rather than treating one short sample as loss.
Counters are cumulative for the process and useful as interval deltas.

`camera-statistics.h` uses the same 10800-sample ROI as exposure metering. Host test
`test-camera-statistics.c` checks all-dark/all-clipped, exact 70th-percentile boundary,
ROI exclusions, clipping counts and rate limiting including stalled/backward time.
Build and run with the same flags as the characterization test above. It passes.
The updated preload also compiles with `-O2 -Wall -Wextra -Werror` for Bionic ARM64
in isolated Kali directory `/home/<user>/quest-camera-work/diagnostics-audit.c0G2ne`.
Its `stock-camera-diagnostics.so` was **not deployed or run**. No image/calibration
output is added by these diagnostics; existing unrelated debug output is unchanged.

## Offline common-shutter compromise experiment — 2026-10-01

Added `tools/camera/experimental-exposure-common-simulator.py`. It compiles a
**temporary host wrapper around the actual C exposure policy**, so comparison does
not depend on a hand-reimplemented baseline. The separate experimental Python
candidate is not imported by capture and has not been deployed. Its only inputs
are simulated observed pixels and applied exposure/gain; simulated scene labels,
lighting and radiance are inaccessible to its decision function.

For each view, the existing individual policy supplies a desired exposure product
P. Candidate selects a common shutter t and integer gain g_i in 16..240 to minimize
`sum(w_i * log(t*g_i/P_i)^2)` plus a small preference for the existing nominal 4 ms
operating point. It searches 19 us shutter steps within 1026..7980 us, with bounded
0.75..1.33 shutter changes. The 1026 us minimum is a conservative experimental
quantization margin, not a verified MCU requirement. Weights are 0.25..1 according
to observable spatial contrast coverage; no view is disabled, and darkness is never
labeled as proven occlusion. The contrast proxy is the fraction of sampled ROI tiles
whose 80th-minus-20th percentile is at least 12 and median is 8..245. All camera
indices are treated symmetrically; permutation invariance is tested.

The toy image model has deterministic synthetic textured/constant scenes, exposure
scaling, clipping, and signal/gain-dependent noise. It has **no measured sensor
response, black-level model, lens PSF, vignetting, motion blur, temporal VIO, or
tracking ground truth**. One policy update is modeled each 0.5 seconds, for 48
updates. An optional 6802 us applied cap is a sensitivity case based on prior
metadata, not a claim that the hardware always caps there.

Command (requires a Python with NumPy plus a host C compiler):

```sh
python3 tools/camera/experimental-exposure-common-simulator.py --output /tmp/quest-exposure-common-results.json --trace
```

The checked run used `/Users/<user>/.pyenv/versions/3.13.1/bin/python3`; this repo's
shell-selected system Python had an unusable NumPy namespace. No environment was
changed. Host policy wrapper compiled with warnings-as-errors. Bounds, common
shutter, minimum per-view weight, quantization margin and camera-permutation tests
passed. Full numeric traces are reproducible with the fixed RNG seed in the script.

### Results: useful tradeoff evidence, candidate not accepted

All values below are synthetic; p70/clipping/contrast are averaged over the final
eight updates. Default scenario rounds applied exposure down in 19 us units.

| Scenario | Current policy | Experimental compromise | Interpretation |
| --- | --- | --- | --- |
| Normal textured views | 3990 us, p70 78..86, no clipping | 3705 us, p70 78..87, no clipping | Both settle; no tracking accuracy conclusion. |
| Mixed two dark/two bright textured views | 7999 us; dark p70 53/71; bright clipping 96%/100% | 4085 us; dark p70 29/68; bright clipping 18%/82% | Preserves more bright contrast by sacrificing dark signal; not a universal win. |
| One covered, three normal views | 7999 us | 7676 us | Retained minimum weight still lets a covered view demand long shutter. No robust occlusion inference. |
| Uniform dark, no true texture | 7999 us; contrast proxy about 62% | 7980 us; contrast proxy about 62% | Noise alone fools the observable contrast proxy. It is NOT reliable feature evidence. |
| All four dark textured | 7999 us; p70 36..68 | 7980 us; p70 36..69 | Both saturate gain/shutter; no real low-light sensitivity improvement shown. |
| One useful dark view, three bright featureless views | Useful-view p70 72; other views clipped | Useful-view p70 65; other clipping ~0.14% | Even symmetric minimum-weight objective spends dark-view signal to improve useless bright views. This is a reason not to deploy. |
| Lights darken then brighten | Returns to 3990 us | Returns to 3705 us | Both recover in this idealized brightness model. |

In the mixed case, the candidate has **26 shutter-direction reversals** over the
24-second simulation and a final-window shutter span of 152 us. Bounded step size
does not prevent oscillation; a temporal confidence/hysteresis model would need
independent tests. Uniform-dark noise contamination is a more fundamental problem
than oscillation. A textured-but-dim view must not lose its weight merely because
its present signal is weak. This candidate remains an offline rejected experiment,
not the next device build.

The all-bright scenario reaches the current policy's 1000 us request and the model's
988 us floor readback four times. **Real capture would discard those frames and
retain old scene state; this simulator continues evaluating fresh images only to
flag the boundary, so its subsequent current-policy oscillation is not a prediction
of the real controller loop.** The candidate's 1026 us minimum avoids that synthetic
rejection under floor/ceil/nearest quantization, but the simulated scene is still
too bright for the allowed gain/shutter. Mixed-scene ceiling/nearest rounding and a
6802 us applied cap preserve the qualitative clipping/dark-signal tradeoff; none
establish actual MCU rounding.

### Evidence needed before policy changes

Obtain synchronized full-resolution raw sequences with applied exposure/gain,
verified scene/controller classification, timestamps and estimator support in
normal, mixed and low lighting. Measure noise/black level versus gain and exposure
on genuinely static detail and uniform regions. Evaluate temporal feature survival
and geometric consistency, not single-frame histogram spread, so a bright flat
surface and gain-amplified noise do not masquerade as useful tracking evidence.
Require preservation of useful dark-camera tracks when balancing clipping elsewhere.
Until those observations exist, retain the actual common-shutter policy and record
its diagnostics. No camera control or physical assistance was needed for this
experiment, and no production exposure-policy source was changed.

## Controlled recorded-input FAST threshold test — 2026-10-01

**Minimum FAST threshold 2 is not accepted.** A single config-only change from 5 to
2 gives no improvement on the weak static clip and slightly more ready poses on the
marked clip while increasing trajectory steps and return discrepancy. No library,
production setting, calibration, recorded event stream, or device was changed.

All four runs used the newly verified deterministic bracket-IMU replay scheduler,
`deterministic=1`, two threads, the saved 0005+0006 library, and quality feature
extension **ON for both thresholds**. Individual replays ran sequentially with
external 180-second deadlines. Other build/replay work ran concurrently, so no wall
time is used as a performance result. The only settings JSON difference is
`config.optical_flow_detection_min_threshold: 5 -> 2`; max threshold remains 40.
FAST therefore gains one additional fallback threshold 2 after 40,20,10,5.

Every run exited 0: static 451/451 groups and 451 poses at either threshold;
marked 1798/1798 groups and 1798 poses at either threshold. Static omitted 2037 and
marked omitted 2063 trailing IMU samples after the last camera cohort, as explicitly
reported by this scheduler. The quality gate evaluates recorded geometry with input
assumed fresh; it cannot validate native processing age or display smoothness.

| Recorded metric | Threshold 5 | Threshold 2 |
| --- | --- | --- |
| Static ready poses | 0/451 | 0/451 |
| Static positive-depth feature medians, cameras 0..3 | 7,2,1,1 | 7,2,1,1 |
| Static displacement from 3 s to end | 0.03996 m | 0.03996 m |
| Static maximum excursion after 3 s | 0.08184 m | 0.08184 m |
| Static maximum pose step | 0.03563 m | 0.03563 m |
| Marked ready poses | 1569/1798 | 1574/1798 |
| Marked positive-depth feature medians | 33,9,2,8 | 36,9,2,7 |
| Marked spatial coverage medians (of 16 cells/view) | 9,6,2,4 | 10,6,2,4 |
| Marked shared-feature median | 5 | 5 |
| Marked 20 s to 40 s displacement | 0.52628 m | 0.56606 m |
| Marked 20 s to end displacement | 0.10996 m | 0.13419 m |
| Marked 95th-percentile pose step | 0.00485 m | 0.00529 m |
| Marked maximum pose step | 0.04325 m | 0.06468 m |
| Marked ready-to-unready edges | 4 | 3 |

The static pose and quality CSVs are **byte-identical** across thresholds. The
marked threshold-5 pose CSV also matches the root agent's independently repeated
baseline (even with its quality output disabled), SHA-256
`99cd6e5edc2c35e5d68275eea2b9195bf3795a79e03c577b0ac55a028b078fb7`.
Threshold-2 marked pose SHA is
`1d1ecf796c6df63a8bf63df1505604bd81a1c282590eebce8cb6386a72575c86`.
This avoids attributing scheduling nondeterminism to the threshold change.

Marked movement was approximately 50 cm with an approximate return, not external
motion ground truth. The larger 20–40 s displacement and endpoint difference are
therefore a regression warning, not a calibrated error estimate. The 50% increase
in maximum update step and lack of static support improvement provide no reason to
trade the existing setting for five extra ready poses. The saved clips do not cover
all possible low-light motion; this rejects the candidate based on current evidence,
not a claim that lower thresholds can never help.

Private evidence directory on Kali:
`/home/<user>/quest-camera-work/fast2-deterministic-hcg9szpv`.
Mac copy and analysis script/results:
`/Users/<user>/work/quest-pmos-bringup/camera-work/fast2-deterministic-hcg9szpv`.
The directories hold exact per-run configs, the two settings JSONs, run commands,
complete logs, pose/quality CSVs, exit statuses and a provenance manifest hashing
executable, library, inputs, calibration and settings. Mac `analyze.py` records the
reference conventions: relative timestamps; first sample at or after 3 s for static
displacement; nearest samples to 20/40 s for marked movement; Euclidean position
steps; medians across all poses. Neither raw images nor calibration contents were
added to the repository.

Run environment in the saved script:
`LD_LIBRARY_PATH=/home/<user>/quest-camera-work/basalt-epipolar-only`,
`QUEST_REPLAY_BRACKET_IMU=1`, unique `QUEST_REPLAY_QUALITY` output path; executable
`/home/<user>/quest-camera-work/replay-vit-bracket`. No REALTIME or TIMING flag.
Executable SHA-256 is
`a49753f728534c828133673d204ba6e1446697d277d25c2c58cd6a7291266286`;
library SHA-256 is
`d12884c4d194259d7bf82dd6802972f21f28eafa2960c74ca96bf0ee3a8ecf96`.

## Bench exposure discrepancy: phase correlation, not a proven clamp

Reviewed private `camera-work/raw-bench-first-camera.log` from the root agent's
approximately 50-second bench, without contacting the headset. There are 49 complete
one-second diagnostic groups for each camera. All four cameras share the following
correlation after the initial exposure ramp:

| Retained metadata exposure | Raw sequence parity | Diagnostic samples/camera |
| --- | --- | --- |
| 5339 us | Odd only | 25 |
| 7106 us | Even only | 12 |
| 7999 us | Even only | 8 |
| 4009 us (initial ramp) | Two odd, two even | 4 |

Of the 25 odd/5339 samples, one accompanies the initial 5332 us request. The other
24 accompany 7101 or 8000 us requests. All 20 even samples at 7106/7999 match those
requests within one 19 us unit. Raw sequence advances 56→2940 across 48.067 seconds;
one-second sampling drifts between long runs of even and odd parity. Every
`rejected_short` counter stays zero. Camera0's logged gain always equals request;
cameras1–3 have one early logged mismatch (requested101, metadata43 at odd seq177).

This strongly supports alternating capture phase/bank behavior or phase-dependent
readback. **A uniform 5339 us hardware clamp is contradicted by the 7999 us readbacks**;
startup also verified 12000 us on all four cameras. One-second diagnostics alone
cannot establish bank identity, command-application lag, every-frame alternation,
or whether publication consistently retains the intended phase. Do not change
shutter limits on the basis of those undersampled mismatches. The current
`scene_us` diagnostic name means retained-by-our-filter, not proven stock frame class.

### Decoder and command semantics independently checked

Inspected the owner's `libsyncboss.so` disassembly, SHA-256
`bd479d113eb7b37a678556461fea3197de9ee98edcdecce038494325bfa88075`, and the saved
Kali `sensors-service` binary. No proprietary source or binary was added to git.

For the selected 8-bit camera mode, `syncboss_camera_decode_metadata` at0x64b0
reads big-endian metadata bytes6/7 and multiplies by the sensor-parameter u16 at
+6; the bench parameter reply has 19 there. Gain comes from byte3. The stock
consumer subsequently scales that gain by1/16. The current numerical exposure/gain
decoding therefore matches the checked stock path for this mode. The first16-byte
bench metadata dump `... 20 20 20 00 d3 ...` contains gain32 and211lines→4009us;
it cannot reveal the remaining frame-class fields.

The stock decoder also reads byte0x59 into a separate output byte. Byte0x50 bit1
causes output class/tag nibble0; otherwise byte0x52 is split into its low/high
nibbles. The stock service branches on the low nibble:1→internaltype2,
2→internaltype3,4→internaltype4; other values take an invalid-frame path. The high
nibble is compared against rolling frame state. These are disassembly-observed
relationships; the exact names of the byte0x50 flag, byte0x59 value and high-nibble
state are not yet established. **Stock classification is not simply exposure<1000.**
The first16 metadata bytes previously logged omit all three of these offsets.

The two named stock setters enter a common command54 serializer with final byte0
(normal setter at0x6574) or1 (controller setter at0x66b4). It serializes four u16
exposures, four u16 gains, four tag bytes and that final selector, matching our
packet layout. Stock service call sites choose a setter by internal client/frame
class. Our `request_exposure` deliberately sends BOTH selectors with the same
settings/tags. Programming both exposure banks can make both phases exceed1000us;
zero rejected-short frames therefore does not demonstrate absence of interleaving.
The actual bank responsible for the odd5339 phase needs per-frame metadata, not a
new guessed classification rule.

### Bounded metadata-only trace added, not yet hardware-validated by this agent

`CAMERA_FRAME_TRACE=1` now opts into `stock-probe.c` trace, default0. It stops after
10 seconds or4096 total raw-frame records, and logs at most64 command records.
After expiration it disables its own per-frame timing overhead. No image bytes are
logged. Frame trace runs before the exposure filter and every-two-frame publication,
recording camera, raw sequence, driver timestamp, host dequeue timestamp, request
ID/current requested exposure/gain, and decimal values of metadata offsets
0x03/0x06/0x07/0x50/0x52/0x59. Field names `m50`, `m52`, `m59` designate hex offsets;
their values are decimal. No unverified semantic labels are assigned.

Each command54 record contains bank selector, request ID, before/after host time,
return code, four requested exposures/gains and tags. Return0 means local command
write success, not MCU acknowledgement. Camera policy, both writes, tags, frame
selection and camera-feed ABI are unchanged. The trace is independent of optional
raw-image capture. It uses the newly reviewed private HAL capacity/pointer/fd guard
before reading extended metadata; the launcher now requires the same known owner
HAL SHA for either raw capture OR frame trace. Unknown HALs fail closed for these
opt-in diagnostics; the existing default path remains unchanged.

`camera-frame-trace.h` and `test-camera-frame-trace.c` verify defaultoff, exact
10-second boundary, backward-time rejection,4096-frame cap and64-command cap.
Host test and shell syntax check pass. Bionic ARM64 compilation with
`-O2 -Wall -Wextra -Werror` passes in isolated Kali directory
`/home/<user>/quest-camera-work/frame-trace-audit.ZC3w5A`;
`stock-camera-frame-trace.so` SHA-256 is
`90f8e88100653d4770c0bb116a339d915d33995910e8fe6d91caff527d8ff42c`.
The root agent owns any RAM-only bench launch and restoration; this audit made no
device changes. Use that all-frame trace to map class/parity/request-response timing
before selecting a phase or modifying exposure policy.

## Complete per-frame trace kbgcpj: alternating readbacks confirmed

Root's private `camera-work/raw-trace-kbgcpj.log` contains2384 raw-frame records:
596 contiguous sequences1..596 for EACH camera,40 command54 records (20 requests,
both banks), and a normal trace-end marker after10.0008seconds. All command return
codes are0 (write success only). The trace did not hit either record cap.

Unlike the earlier bench, this restart has **odd long/even shorter exposure**:
startup request12000/16 eventually yields odd12008/16 and even5339/16. Requests
4000 with gains28,37,49,65,87,116,155,207 subsequently update BOTH phases, as do
4599/240→4598 and6115/240→6118. Once8000/240 is requested, odd frames become7999
while even frames remain6118 for the rest of the trace despite nine paired8000
requests. For camera0 from request12 onward:132 odd7999,134 even6118, and one
odd6118 transition. All four cameras have the same exposure distributions.

This confirms every-frame alternation, not merely a1Hz alias. It is consistent
with **one exposure bank retaining its last accepted setting when a larger request
is not applied**, rather than clamping all cameras to one exposure. Its retained
value changes between runs (5339 previously,6118 here), while the long phase follows
the higher request. Command54's controller selector is a plausible explanation,
but simultaneous writes to both banks do not identify which one owns which phase.
**Never hardcode odd/even identity:** it flipped across these two restarts.

Accepted changes appear in camera0 about38–71ms after local command write in this
trace (requests2..11, separately tracking both parities). This is a command-to-
observed-buffer latency, not exposure timestamp calibration or a universal bound.
The8000 mismatch persists far longer than that ordinary transition delay.

Extended metadata findings:

- Byte0x50 is128 on all2384 records; its bit1 is clear.
- Low nibble of0x52 is1 on595/596 frames per camera; only firstseq1 is0.
  Both exposure phases therefore carry the SAME low-nibble class under our forced
  tag1 commands. This does not provide a reliable phase classifier as currently set.
- Byte0x59 equals rawsequence modulo256 on every frame; it behaves as an8-bit frame
  counter in this capture, not a bank discriminator. High nibble of0x52 cycles but
  does not independently label the two exposure durations.
- Since publication advances every two raw sequences, selection can remain on the
  shorter phase. The root's saved raw cohort660 is even and thus6118 in this run.
  A visually dark retained feed need not represent the requested8000us scene phase.

The narrow next experiment is asymmetric, low accepted-range bank requests with
auto exposure OFF: bank0=4000us, bank1=2000us, gain16 for both, all four cameras
equal within each bank. Keeping tag bytes unchanged avoids inventing stock class
semantics. Per-frame readbacks then identify bank ownership without raising exposure
limits, guessing phase parity, or changing production selection. Stock caller tag
assignment/controller defaults are being audited separately before any correction.

## Isolated bank-identification diagnostic implemented

Two new explicit environment values opt in:
`CAMERA_DIAGNOSTIC_BANK0_US=4000` and `CAMERA_DIAGNOSTIC_BANK1_US=2000`.
Both must be present and distinct, each2000..4000us; diagnostic gain is fixed16.
It fails before opening cameras unless `CAMERA_FRAME_TRACE=1`, auto exposure is0,
and MCU/exposure/all-camera/stream control is enabled. The launcher additionally
rejects an override without trace/auto-off and the trace path enforces the verified
owner HAL hash. Defaults keep the existing production policy and both-bank settings.
Root owns the external approximately20-second RAM-only bench deadline and restoration;
this agent has not run it on hardware.

Actual override values are serialized per bank and logged in command records.
Frame records now contain **both** `bank0_us/bank0_gain` and `bank1_us/bank1_gain`,
not an invented single requested value for an as-yet-unidentified frame phase.
Startup diagnostic verification waits until both numerical settings have appeared
on every camera, with19us readback tolerance; it does not call a phase bank0/1
merely from parity. Automatic policy remains off for the experiment. Optional1Hz
diagnostics mark `diagnostic_banks=1`, list both bank exposures in the summary and
use requested_us/gain0 in the per-frame line to indicate that a single nominal
request is inapplicable. This0 is NOT sent to hardware.

`camera-bank-diagnostic.h` and its host test cover missing fields, unsafe values,
equal values, missing trace/control, auto-on rejection, normal default passthrough,
fixed diagnostic gain, and both-value readback masks. Host tests, shell syntax and
Bionic ARM64 `-Werror` compilation pass. Binary:
`/home/<user>/quest-camera-work/bank-diagnostic-audit.YkAPed/stock-camera-bank-diagnostic.so`,
SHA-256`ceb9c38db8edd3a489e7da1e97e1c522ddd7378f19af76a2510b854a7d20e33e`.
No production frame classifier or exposure policy has been changed.

### Distinct-bank bench01 completed; identity established within this run

Root's complete private `camera-work/bank-bench-01.log` has2384 trace records,
596 contiguous sequences/camera, two successful command writes, and normal trace
end. Startup confirms both requested numerical settings on all four cameras.
For settled seq10..596, EVERY camera has294 even frames at4009us/gain16 and293
odd frames at1995us/gain16. Thus **in this run bank0=even, bank1=odd**. No exposure
exceptions occur after seq10. Low metadata tag nibble remains1 for both phases.
Raw driver intervals are16.308..17.027ms; same-bank intervals33.307..33.358ms.

This supplies bank identity only for the current start. It must not be projected
back onto previous runs, whose raw sequence parity changed. The next bounded
experiment identifies phase and then steps exposure in the SAME uninterrupted
stream to determine which bank retains its old setting.

`CAMERA_DIAGNOSTIC_BANK_STEP=1` adds that optional one-shot step to the existing
valid4000/2000 diagnostic. It requires both diagnostic bank fields, trace, auto-off
and the same verified HAL gate. After at least3seconds from the first exposure
request, and only after both initial values were seen on every camera while trace
is still active, it sends bank0=bank1=8000us/gain16 once. The initial verification
latch remains set, so the ordinary startup retry path does not restore4000/2000.
The step marker plus command/frame logs record actual values. Without this flag,
no step occurs; no production policy or frame selection changes.

Tests cover exact3second boundary, unverified/backward-time rejection, one-shot
completion and actual8000/16 selection for both banks. Bionic build passes in
`/home/<user>/quest-camera-work/bank-step-audit.7B3Cwv/stock-camera-bank-step.so`,
SHA-256`95cea251fa6b28d1babe1ecc35fb3b5b537454db3edba7c48dde85aaaea648f8`.
The parent owns the20-second RAM-only run and restoration. This records an
implemented diagnostic, not an already-observed step result.

## Scene-bank selection candidate and completed RAM bench

The uninterrupted `bank-step-01.log` establishes which bank retains its old
setting: after initial bank0=4000/bank1=2000 identifies the phases, requesting8000
on both makes bank0 report7999 while bank1 stays1995. Local command write success
is not proof of MCU application. Parent counted209 odd1995 and208 even7999 after
request2 in that run; parity remains a within-run observation only.

Stock caller evidence now supports actual class assignment. Monterey
`sensors-service` SHA256
`f2549a8c1f433385ea98faf068cfe0e66a907f1400f8c19072010a338c7c7f43`
has default table atVA0x1cc40: type2 uses5000us/Q4gain48/tag1 and normal bank0;
type3 at0x1cc80 uses38us/Q4gain48/tag2 and controller bank1. Constructor0x38358
copies six-byte tuples. Service0x3a4e8 converts exposure seconds to integerus;
0x3a500 converts gain toQ4. Setter dispatch0x3af08/0x3af10 selects normal for
type2 and controller otherwise. Metadata consumer0x64ea4 maps low tag1/2/4 to
internal type2/3/4. The separate type4 default is not evidence that controller
bank can never carry that class. Timing audit independently verified this chain.

`CAMERA_SCENE_BANK_ONLY=1` is an explicit, default-off candidate. It restores
bank1 to38us/Q4gain48/tag2 once, latching that only after successful local write;
later auto-exposure commands update bank0/tag1 only. Any candidate command write
failure stops capture through existing HAL cleanup. The launcher requires the
verified owner HAL hash. Constructor and launcher reject combining it with bank
identification/step overrides or raw capture (raw artifact exposure semantics
still reject legitimate sub1000us scene values).

Selection uses metadata0x52 low nibble1 with metadata0x50 bit1 clear. Unknown,
controller and alternate classes are rejected; no fixed parity or1000us cutoff
is used in this candidate. Before exposure control, all four retained samples
must form a synchronized, fresh cohort captured after the last command write.
That additional gate is limited to the new mode. Existing defaults, feed ABI,
common-shutter policy and publication logic are unchanged. Counters expose
retained scene, rejected classes and class2 frames with exact38us/gain48 readback.
Tests cover packet bytes, stock controller values, all low tag classes/high
nibbles, invalid flag handling and coherent control timestamps. Host suites,
shell syntax and Bionic ARM64 `-Werror` build pass; timing audit independently
reviewed final source and launcher before the parent staged it.

Artifact: `/home/<user>/quest-camera-work/scene-bank-audit.jA45Uv/stock-camera-scene-bank.so`,
SHA256`2f425038915171f627e1e9ba5770acc109ce274a0379246619adc906a888c429`.
This agent performed no hardware deployment.

### Scene-bank bench01: classification fixed, another gain phase exposed

Complete private `camera-work/scene-bank-01.log` closes cleanly, status0, after
1776 raw frames per camera. Bounded trace has2389 records (598 oncamera0,597 on
others, ending at its time limit),20 bank0 writes and exactlyone bank1 write.
All trace records classify as1 or2. Every class2 frame fromseq3 onward reports
38us/gain48 on all four cameras; firstseq1 still has startup12008us/gain16.
Controller frames are excluded even during that first numerical transition.

Scene timestamps have median33.334ms spacing, approximately30.005Hz. Early
transition interval range29.324..34.321ms is not a steady-state performance bound.
Publication count rises29→869 across28.023seconds,29.975Hz; every consecutive1Hz
sample advances30 and successful publication count matches. Thus the new filter
retains the expected scene cadence without selecting a fixed startup parity.

Camera0–2 have13 trace scene readback mismatches each, clustered immediately after
requests. Camera3 has126: a second phase exists WITHIN class1. For requests5–9,
scene sequence modulo4==0 retainsgain113 while modulo4==2 follows151/180/102.
Request10 gain76 reaches both phases. For requests11–20, modulo4==0 retains76
while modulo4==2 follows101/134. Both carry tag1 and7999us after the exposure
settles. These are within-run phase observations, not a new selection rule.

The command itself alternates camera3 gain101/134 every500ms for requests13–20.
The1Hz p70=5 diagnostics repeatedly sample134, hiding that cycle. A stale76
readback naturally yields101 under the existing1.33 increase, then101 yields134;
catching stale76 again yields101. Therefore declining requested gain does not
require bright pixels or prove brightness oscillation. The metadata gain
retention is directly observed; its MCU/sensor cause remains unresolved. After
15–16seconds the1Hz requested/readback becomes237→240 and stays240 through29s,
but the per-frame trace has already ended, so uniformity of both scene phases
at that later time is unproven. A repeat and command-time policy input telemetry
can resolve this without guessing from1Hz samples.

This bench validates bank restoration, stock class filtering and publication
cadence. It does not establish low-light tracking improvement, uniform applied
scene gain, or readiness to silently change defaults. Keep promotion separate
from that narrower evidence.

### Scene-bank bench02 repeat: no recurring gain split

Complete private `scene-bank-02.log` also closes cleanly status0. It contains2386
trace frames,20 bank0 writes andone bank1 write. Each camera has exactlynine scene
request/readback mismatches, one at each early command transition2–10. Thereafter
all trace scene exposure/gain reports7999/240 as requested. All four command gains
follow16→64→85→113→151→201→240 together; no alternating101/134 commands occur.
Thus bench01 does not establish a universal camera3 gain limit above113.

Controller readbacks settle to38/48 on all four cameras. Initial unknown class0
records are rejected. Within this single run, cameras0/1 scene frames are EVEN
and cameras2/3 are ODD: class-based selection handles this, whereas a shared raw
parity rule would reject valid scene frames. Scene median interval33.334ms;
publication27→869 gives30.020Hz across the diagnostic interval. The different
starting count reflects startup and does not imply a steady rate above30Hz.

This repeat supports the bank/class correction but leaves bench01's intermittent
secondary gain phase unexplained. A fixed8000us/gain48 auto-off run can test stable
readback without feedback changes; it cannot by itself prove arbitrary later
commands always update every sensor phase.

### Fixed scene-bank bench and aggregate readback diagnostic

Parent's complete private `scene-fixed-01.log` uses8000us/gain48 with auto-off.
Afterseq10 all four cameras report controller38/48 on293 frames each and scene
7999/48 on293/293/293/292 frames respectively, without exceptions. Exactlyone
write per bank was sufficient for that run. Services were restored by the parent.
This argues against an unconditional need to repeat every command, but does not
explain the intermittent bench01 gain phase during automatic changes.

A further default-off diagnostic adds counters to existing1Hz lines, accumulating
**every** selected scene frame so1Hz alias cannot hide alternating readback:
`scene_match`, `scene_mismatch`, `scene_transition`, and `last_bad_seq/us/gain`.
They run only when scene-bank selection AND CAMERA_DIAGNOSTICS are enabled.
The helper excludes frames captured at or before100ms after the last command;
capture time rather than dequeue time also excludes delayed old buffers. Eligible
frames match when exposure differs by at most19us and gain is exact. These are
setting-consistency counters, not image-quality or estimator-success metrics.
Repeated AE commands restart the100ms exclusion even if requested values repeat;
thus the count intentionally does not claim coverage of every scene frame.

Pure tests cover exact100ms boundary, backward/zero timestamps, both19us
boundaries,20us rejection, gain mismatch and988us quantized scene acceptance.
Host tests and Bionic `-Werror` compile pass. New binary:
`/home/<user>/quest-camera-work/scene-readback-audit.mIL16P/stock-camera-scene-readback.so`,
SHA256`2662ee1f1bf6f1076bb2ef71b66eb770fd6fef80f87683be7c0d14c690c8afc4`.
No exposure policy, class selection, packet behavior or launcher changed.

### Ninety-second aggregate bench confirms eventual settling, not clean ramp

Complete private `scene-readback-01.log` closes cleanly. Last diagnostic near89s
has scene mismatch counts0/12/18/136 for cameras0..3 and eligible match counts
2136/2124/2118/2000. Each excludes535 transition frames. Last mismatched sequence
is0/297/265/855 respectively: camera3 stops adding mismatches around14s and all
four remain stable through the end. Thus the partial-application effect is not
unique to camera3, although that camera is worst in this run. Camera1 briefly
retains5947us; cameras2/3 retain old gain/exposure tuples during the ramp.

The existing control gate proves a fresh synchronized cohort, not that its
settings equal the latest desired target. Feeding partial application back into
policy can compute another target before the earlier one is applied. A separate
candidate acknowledgment state machine is therefore under development: retain
one desired four-camera target, require two successive fresh matching cohorts,
retry that same target when unconfirmed, and avoid rewriting an unchanged target.
This proposal does not change the common-shutter brightness policy and must be
bench-tested independently before promotion.

### Target acknowledgment candidate integrated (not yet hardware-validated)

`exposure-ack.h` from the prediction audit now governs only scene-bank mode.
After writing a desired four-camera tuple, it requires two distinct consecutive
scene cohorts (raw sequence increments2 per camera), captured more than100ms
after write completion and no older than100ms. Every exposure must be within19us
of desired and every gain exact. Repeated/stale/out-of-order cohorts cannot
confirm a target or trigger policy evaluation. Until confirmed, the caller
retries the SAME desired tuple at500ms cadence; it cannot derive the next target
from partially applied settings. Once confirmed, policy evaluation remains at
500ms cadence and an unchanged target causes no command write. Subsequent fresh
mismatch removes confirmation. A10-second unconfirmed interval stops retries and
logs once; capture continues, with timeout exposed explicitly rather than a
fallback class/exposure policy. Timers still run if a camera never supplies its
first accepted scene frame. Normal/default capture retains its original path.

Existing1Hz diagnostics now include `target_confirmed`, `target_streak`, and
`target_timeout`. Existing per-frame setting counters remain independent. The
startup `verified` flag latches initial confirmation and must not be confused
with the current target confirmation field. No additional per-frame logging.

Host helper tests include alternating gain101/134 sensor behavior, target hold,
partial/stale feeds, both phase coverage, settle boundaries, no-op evaluation
cadence, retries, timeout/recovery and out-of-order rejection. They pass with
`-Werror`; independent integration review and Bionic ARM64 compilation also pass.
Artifact: `/home/<user>/quest-camera-work/scene-ack-audit.fIau5v/stock-camera-scene-ack.so`,
SHA256`3d89cb06b491c1c284b72a515937d5a5fbd875e97fc5c61dc3ca9c9446be4451`.
Source SHA256`7f7c6486394f7e920348a5f2e6ccb0ada04b8c248a66e9cd9ea721ce7888795f`;
helper SHA256`377258553b463f1c32b6adf6490d8ed9ec7f9264c437d186c83e3d283651b52b`.
This is a controlled candidate, not a claim that MCU application or tracking is
fixed. Parent owns the next bounded RAM bench and restoration.

### Acknowledgment bench01 exposes unverified controller initialization

Private `scene-ack-01.log` shows a separate startup failure. Afterseq10, camera2
has587 traced raw frames ALL carrying tag1,38us,gain48: there is no visible long
scene phase on that camera. Other cameras alternate valid class2/38/48 controller
and class1/12008/16 scene frames. The initial bank1 packet requested tag2 on all
four cameras, but local write success did not establish that it took effect.
Repeated bank0 startup requests did not correct camera2. Scene target times out
explicitly and publication remains0; this is not a tracking-quality result.

The previous once-after-local-write bank1 latch is insufficient. A bounded
controller readback acknowledgment is being designed before further benching:
require fresh class2/38/48 evidence on every camera, retry the identical stock
controller tuple with a deadline, and prevent scene startup/control/publication
until that evidence exists. Do not silently relabel the anomalous short tag1
frames as scene or controller. The evidence demonstrates missing expected output,
not the internal MCU cause or which register/phase failed.

### Controller-first staged candidate implemented

The opt-in path now starts with **bank1 only**, stock38us/Q4gain48/tag2. A pure
`controller-ack.h` observer examines every raw frame before classification and
requires two successive matching controller sightings on EACH camera, sequence
increment2, captured after100ms settling and fresh within100ms. Global sequence
parity is never assumed. Wrong class/tuple on an established controller phase
breaks support; ordinary intervening scene frames do not. Bank1 retries preserve
the identical tuple, no faster than500ms since either bank's last command, with
one explicit timeout after10s. Missing/partial camera input still advances the
deadline. Late valid readback can recover; no guessed frame classification.

Only after controller confirmation does the caller write bank0 and start a fresh
scene acknowledgment episode. Midrun controller loss withdraws the known RAM
`/tmp/camera-feed` artifact, clears feed frame/timestamps and scene acknowledgment,
and quarantines publication. Restoring controller proof resends the cached scene
target with a fresh scene-ack clock. Publication resumes after that initial or
recovery scene target is confirmed. Later ordinary AE target transitions do NOT
pause publication, avoiding new periodic gaps during the exposure ramp.

A separate scene metadata envelope rejects impossible class1 values outside
981..12019us orQ4gain16..240. Class1 remains mandatory; this does not relabel a
short frame as controller. Legitimate1000us→988us quantization is accepted, while
anomalous tag1/38us frames cannot enter the scene feed. Diagnostics add quarantined
scene count, invalid-scene-metadata count and controller confirmed/timeout state.
Scene command, controller command and global command clocks remain separate.
No default-mode packet policy, feed ABI or production brightness rule changed.

Controller tests cover class/flags/tuple identity, distinct successive sightings,
intervening scene frames, missing/stale/out-of-order data, bounded retry spacing,
exact timeout and late recovery. Scene envelope boundary tests and existing
scene-ack tests pass. One deliberately conservative limitation: a dropped
controller frame causing sequence+4 breaks confirmation and can quarantine the
feed. This first bounded diagnostic prioritizes evidence; its effect on tracking
continuity must be measured, not assumed harmless.

Final staged Bionic build:
`/home/<user>/quest-camera-work/scene-staged-audit.b44rXh/stock-camera-scene-staged.so`,
SHA256`d2feb73efde49d158bd98b8449adb54387d1ade459fee5dae40e342ded9c7424`;
source SHA256`dcc17c56047d70fe0996dd066b0077298a27d0c37d23786ecd21c8c7673f1e51`.
This also removes the original back-to-back bank writes separated by only about9us.
That is a controlled sequencing change, NOT proof that close command spacing
caused the observed failure. Parent owns the hardware bench and restoration.

## Correction: stale metadata undermines physical partial-application inference

The complete private `scene-staged-01.log` closes cleanly but never confirms the
4000us/gain85 target: camera3 appears to alternate gain64/85 despite repeated
identical commands. Crucially, raw metadata byte0x59 no longer follows advancing
HAL sequence numbers on the affected phase. Camera3 sequence modulo4==0 freezes
m59=64 and m52=241 fromseq68 through596. There are131 modulo256 counter mismatches;
periodic equality afterwrap is merely an alias, not evidence of refreshed data.
Camera2 sequence modulo4==0 freezes m59=140/m52=81 fromseq144 through596, giving113
counter mismatches. Camera0/1 counters remain consistent throughout the trace.
All four HAL timestamps and raw sequences continue advancing.

Camera2 gain byte0x03 eventually changes64→85 while its m52/m59 remain frozen.
Those fields lie in different portions of the header. This is consistent with
stale memory/cache lines or another partial buffer-refresh problem and is NOT
proof that the physical sensor retained an old gain. Camera3's frozen phase
always reportsgain64. Initial logs expose only the first three data pointers,
not the later failing phase's buffer index/fd; available evidence cannot establish
which allocated DMA buffer owns it. No addresses or guessed fd association are
needed to establish the metadata-counter inconsistency.

Cross-check of existing bounded traces, m59 mismatches by camera0..3:

| Private trace | Counter mismatches |
| --- | --- |
| scene-bank-01 |47 /47 /47 /78 |
| scene-bank-02 |0 /0 /0 /0 |
| scene-fixed-01 |0 /0 /0 /0 |
| scene-readback-01 |0 /0 /64 /30 |
| scene-ack-01 |17 /52 /0 /25 |
| scene-staged-01 |0 /0 /113 /131 |

Frozen values repeat at particular raw sequence residues, making a rotating-buffer
problem plausible. However, ack01's anomalous camera2 has ZERO m59 mismatches while
allframes reporttag1/38/48. Counter consistency alone therefore does not establish
that every other header field or image byte is fresh, nor does one stale-counter
mechanism yet explain every observed failure.

**Earlier “partial application,” “retention,” and bank-setting observations in
this audit describe CPU-visible metadata, not proven physical sensor state.**
The suggested MCU/sensor explanations remain hypotheses. Aggregate matching
counters and acknowledgment state machines consume the same potentially stale
metadata and cannot validate physical application until buffer coherency is
established. Prior bank/class experiments remain useful observations but are not
an independent hardware truth source. Stop further AE policy changes and audit
the stock DMA/cache synchronization path first. Parent restored baseline; this
agent made no additional source changes for this finding.

## Exact deployed-kernel ION audit (read-only)

Parent verified the running kernel is `4.4.205-perf`, not the earlier3.18
assumption. The package uses sourcecommit
`6929f734ce0e602018790ff3a52dc7bad646af60`; private cached archive is
`/home/<user>/pmos/work/cache_distfiles/linux-oculus-monterey-6929f734ce0e602018790ff3a52dc7bad646af60.tar.gz`.
Relevant source was extracted into private `camera-work/ion-kernel-audit/`.

Primary source establishes manual synchronization requirements:

- `drivers/staging/android/uapi/ion.h:61` defines CACHED=1 and
  CACHED_NEEDS_SYNC=2. `ion/ion.c:566` unconditionally adds NEEDS_SYNC during
  allocation, explaining why an allocator passing flags1 still requires manual
  cache management on this kernel.
- `uapi/msm_ion.h:165` defines `ion_flush_data`:32-bit handle,32-bit fd, pointer
  vaddr,32-bit offset and32-bit length; LP64 size24.
- `msm_ion.h:200` defines INV as `_IOWR('M',1,struct ion_flush_data)`, yielding
  `0xc0184d01` on this ABI. CLEAN is4d00; CLEAN_INV is4d02. These operations are
  different and should not be substituted casually.
- `ion/msm/msm_ion.c:732` dispatches the exact command. Handle<=0 imports the
  supplied DMA-buffer fd; a positive handle is resolved in that ION client.
  It checks the userspace mapping bounds, performs the operation, then releases
  the temporary imported handle.
- `ion_pages_cache_ops` chooses `dmac_inv_range` for INV and applies the requested
  offset/length over the backing scatterlist. `ion_do_cache_op:352` skips uncached
  or secure buffers. Thus no-op success on an uncached buffer is intentional.

The owner's `libion.so` SHA256
`71c81c8313450928185e5a992bf78c5fc309592205606d7477ff56af19bd8426`
exports open/close/import/share/map/alloc/query and `ion_sync_fd`, but no dedicated
CPU-invalidate wrapper. Disassembly `ion_sync_fd:0x2160` emits`0xc0084907`;
kernel `ion.c:1720` routes that generic ION_IOC_SYNC to
`dma_sync_sg_for_device(...,DMA_BIDIRECTIONAL)`. It is not the direct CPU
invalidation operation audited above.

The verified candidate route is the owner's exact custom operation:
outer`ION_IOC_CUSTOM=0xc0104906`, with innercmd`0xc0184d01` and pointer to the
24-byte flush descriptor. A fresh ION client can use handle0 and the actual
shared DMA fd, mapping address, offset0 and validated full mapped length. This
matches timing audit's owner-HAL callback packet and stock gralloc CPU lock:
LockBuffer0xbb08 calls CleanBuffer with op2; IonAlloc::CleanBuffer0xa050 maps op2
to INV0xc0184d01 and sends the same outer ioctl. The naming `CleanBuffer` alone
would have been misleading; its actual opcode proves invalidation.

For this HAL, use only the already-verified private buffer guard to obtain the
fd/address/capacity; raw[7] is fd plus padding, NEVER a length. Invalidate while
holding the dequeued frame, before ANY CPU metadata/image read. The complete
mapped buffer is311296bytes, containing307840 logical bytes. Stock uses full
mapped length. Whether this resolves the observed stale data still needs a
bounded hardware experiment; no operation was executed during this audit.
