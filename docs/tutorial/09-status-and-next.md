# Current status, work in progress and TODO next

Checkpoint: 2026-10-01. This page supersedes older status statements in chronological
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
| Cameras | All four monochrome streams and world-space panels | Stale OpenRC cache fixed; normal reboot verified |
| Drawing | Wearer-confirmed head-aimed3D ink, saved strokes | Not tracked-controller drawing |
| Passthrough | Real composite/effects implemented | Stitching rejected; not accepted stereo |

## Final checkpoint — 2026-10-01

Work stopped at the owner's request. Headset verified in fastboot/USB Update Mode
(serial `<SERIAL>`), not powered off. No experimental firmware or bootloader
writes. The installed scene remains the orientation-only fallback; prediction is
disabled. Latest camera/Monado experiments were RAM-only and baseline services were
restored. No new wearer test is pending.

- Four-core tracking enabled substantial walking in full light, but remained laggy
  and unreliable in low light. The wearer rejected velocity prediction as worse.
- Experimental r8 fixes prediction geometry and HOLD query behavior; 22 package
  tests passed, but the package is not installed or wearer-accepted.
- Saved-input replays are now deterministic. Lower FAST thresholds, guessed timing
  offsets and zero-bias IMU propagation did not establish improvements.
- Frozen camera metadata while HAL frame counters advance makes exposure readback
  unreliable. Earlier claims of physical bank-command failure are unproven.
- The exact stock CPU cache-invalidation path is traced. An opt-in implementation
  builds and passes host tests, but **has not run on the headset**. This is the next
  bounded experiment, not a demonstrated tracking fix.
- Display now uses one composite wall left of the cube. Duplicate standing panels
  and floor panel were removed. Stitching still needs correction; this is not
  accepted stereo passthrough.

Details and historical evidence: [tracking log](../basalt-positional-tracking.md),
[camera log](../CAMERA-ROADMAP.md). Private assets: [location and SHA256 inventory](../assets-inventory.md).

## TODO next — in order

1. Run the prepared 90-second RAM-only CPU-invalidation test using original exposure
   settings. Compare all-frame metadata counters and image freshness; retain both
   recovery mechanisms and restore baseline. Do not tune exposure against stale data.
2. If coherency is established, reassess scene/controller-bank selection and exposure
   acknowledgments. Test normal and low light using existing recordings first.
3. Reduce positional latency and tracking loss, then validate translation scale,
   return error and stationary stability. Prediction stays disabled until useful.
4. Correct composite geometry; add controller aiming and camera-driven drawing/effects.
5. Integrate optics/rendering into a native OpenXR compositor and VR shell. Hardware
   acceleration remains unverified; no desktop or joystick locomotion target.
6. Finish boot-success/slot retry handling and charger-only shutdown, package the full
   reproducible install, and validate audio/remaining peripherals separately.

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
- Never apply the whole Basalt patch directory: 0002, 0004 and 0007 are unsuccessful or unaccepted experiments; 0005/0006 have supporting tests.

[Boot](../boot-bringup.md) · [Wi-Fi](../adsp-wifi.md) ·
[Autostart](../TEST-SCENE-AUTOSTART.md) · [Cameras](../CAMERA-ROADMAP.md) ·
[Tracking](../basalt-positional-tracking.md) · [Index](README.md)
