# Current status, work in progress and TODO next

Checkpoint: 2026-09-30. This page supersedes older status statements in chronological
logs. Evidence is from one owner's Quest 1; a fresh-owner end-to-end tutorial run has
**not** been completed. No claim of a finished consumer OS or 6DoF runtime.

## Demonstrated on the headset

| Area | Result | Boundary |
|---|---|---|
| Boot | Linux/OpenRC,4096-byte filesystem blocks, USB SSH | Manual boot finalization remains |
| Wi-Fi | Scan, WPA2, DHCP, internet/DNS, SSH, reboot startup | Owner stock firmware/runtime needed |
| VR scene | Direct framebuffer, corrected stock lens mesh, accepted orientation motion | CPU rendering, diagnostic scene |
| Recovery | Trigger-renewable five-minute guard and HUD, independent hardware watchdog | Expiry returns to fastboot |
| Autostart | Cold-boot Wi-Fi and scene verified | Slot retry accounting/charger mode unfinished |
| Cameras | All four monochrome streams and world-space panels | Later camera startup inconsistency |
| Drawing | Wearer-confirmed head-aimed3D ink, saved strokes | Not tracked-controller drawing |
| Passthrough | Real composite/effects implemented | Stitching rejected; not accepted stereo |

## Current in progress

- **Physical movement tracking:** recorded Wi-Fi camera/IMU replays, rectification,
  correspondence diagnostics and initialization analysis. The 60-second marked capture
  is already saved; do not repeat solely because an older note asks for readiness.
  Startup drift remains unresolved; no live positional pose deployed.
- **Native Basalt:** ARM musl library/replay built; 3-frame real-data replay passes in
  an emulated buildroot. Headset dependency staging, performance and integration remain.
- **Passthrough floor panel:** requested to replace the oversized composite; source
  exists, final wearer acceptance still needed.
- **Tutorial audit:** corrected missing VR/camera/control work and unsupported claims.
  Clean-machine reproduction is still pending.

Tracking experimentation is paused while this documentation audit is performed.
The accepted scene remains the baseline. A separate clarification about whether the
headset was moved again near the end of the marked clip remains unresolved; do not
attribute all endpoint displacement to estimator drift without that history.

## TODO next — in order

1. Diagnose stationary startup drift using existing recordings. Validate units/axes,
   gravity/bias initialization, feature support, extrinsics and clock alignment. Keep
   original captures and avoid declaring a fix from reduced stationary drift alone.
2. Establish measured translation scale, return error and stationary stability across
   recordings. Report initialization exclusions and reference-pose choice explicitly.
3. Audit/bundle ARM dependencies privately, then run a bounded native trial alongside
   the accepted scene. Integrate with Monado only with nonblocking sensor queues,
   coordinate conversion and tracking-loss handling.
4. Validate the small floor composite; repair stitching/parallax and retain the four
   separate camera stations. Work on controller aiming and physical controls.
5. Extend camera-data drawing/effects: optical flow, particles, silhouettes and trails.
6. Integrate optics and rendering into a real native OpenXR compositor/VR shell.
   Investigate isolated KGSL GL further if useful; hardware acceleration is unverified.
7. Finish boot-success/slot retry handling, charger-only mode and camera autostart
   reliability. Preserve both recovery mechanisms during changes.
8. Produce a complete clean-build/package/install workflow: boot finalizer, owner
   asset extraction manifest, exact dependency versions, guard/scene/camera packages,
   rollback and verification. Test it from an independent checkout/device.
9. Audio and other unvalidated peripherals need separate bring-up and acceptance.

## Finish criteria for a device session

Save an actual full framebuffer screenshot, distinguish wearer checks from automated
checks, preserve drawings and working boot defaults, record the next actionable step.
At the agreed stopping point use orderly shutdown. USB-connected poweroff has caused
a charger reboot; an unplugged Wi-Fi shutdown is needed to leave this device off.

## Reproduction gaps the tutorial must not hide

- `prepare-monterey-boot` is referenced by historical scripts but **not included in
  this repository**. The hard-coded `prepare-4k-boot.py` is a historical recipe.
- pmbootstrap initialization was pre-seeded on the original build host. Exact clean
  setup, pinned dependencies and all private asset extraction steps need packaging.
- Stock code/firmware, per-unit calibration, lens assets and backups are different
  inputs. Calibration is not all located in a stock system image.
- Guard, scene and camera runtime sources exist, but are not all installed by the
  base device package. Copying `packages/*` does not reproduce the entire headset.
- Never apply the whole Basalt patch directory:0002 is an unsuccessful experiment.

[Boot](../boot-bringup.md) · [Wi-Fi](../adsp-wifi.md) ·
[Autostart](../TEST-SCENE-AUTOSTART.md) · [Cameras](../CAMERA-ROADMAP.md) ·
[Tracking](../basalt-positional-tracking.md) · [Index](README.md)
