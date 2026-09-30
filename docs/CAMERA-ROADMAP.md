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
