# Quest 1 tracking, optics, and performance — PAUSED handoff

> **Deployment update — 2026-09-30, after owner resumed with “can you push the framerate fixes to device”:** Optimized bundle is now persistently installed at `/root/quest-mesh-fast/` on the headset. Includes optimized renderer, stock mesh, ARM test, baseline renderer, and `run-head-mesh-fast.sh` (manual launch only, no autostart). No partition flashing or watchdog changes. Booted existing slot B (retry count6; no reset needed this turn). ARM tests pass: 6144 inverse round trips, max .000908pixel; cached max deviation .0294pixel; cache startup1.158s; 100k exact lookups .0213s vs cached .0041s. **Full static renderer still measures19.5fps** (367frames over about18.8 render seconds after cache startup); matched old baseline234frames in12s (~19.5fps). Thus installed optimization has NOT fixed overall framerate; further profiling of rendering/display costs needed. Baseline first lacked executable permission; chmod755 then rerun successfully. Optimized binary SHA256 50f7017430f3532a0a285d788ecc09834d6e3387af296cb7cc5e288f40d68199. Tests ended cleanly; no tracking scene or Monado service started this turn and no wearer question pending. Last device uptime about2min; watchdog remains active. Logs in local mesh-fast/device-validation.log, device-benchmark.log, device-baseline.log and archive head-tracking/performance-deployed-20260930/. Historical “not transferred/tested” statements below are superseded.

Written 2026-09-30 at the owner's explicit request to pause and preserve context for another agent to sort/commit. THIS DOCUMENT is the latest state. Earlier dated blocks in MONTEREY-PMOS-HANDOFF.md are chronological history and contain superseded “pending” statements.

## Immediate state and next objective

The owner confirmed correct tracking directions AND correct stock-mesh optics. Current issue is poor frame rate in the diagnostic scene. An optimized renderer is built, but NOT yet tested on the headset. Do not claim the performance fix works on-device.

Latest owner observation, verbatim: “wait wait it was just behind me. hang on. yes, taht now looks correct now but the framerate TANKED”. The previous report of a black screen was because the scene was behind the wearer, NOT a confirmed rendering failure.

Then owner explicitly requested: “pause and write all of this down so we don't lose anything in context compaction like just happened. i will givr it to another agent to sort/commit/etc”. All device work is paused. No benchmark, new visual test, or reboot was performed after that request. No new commit was made.

Last read-only device check just before pause: SSH worked, `uptime` reported about 3 minutes, no monado-service or demo process appeared (only the search shell), and /tmp/distortion-mesh.bin did not exist. A fresh boot had been started after resetting the same slot B retry count. The normal watchdog remains active and should return it to fastboot; CURRENT live state must be checked when resuming. No optimized artifacts have been transferred on this boot. Do not assume it is still running Linux.

## Safety and interaction constraints

- This is the user's ONLY headset. Preserve stock recovery and all backups.
- Do not touch boot-chain, NV, modem firmware, controller firmware, factory calibration, or unrelated partitions. Current work needs only userspace files and runtime processes.
- Keep the existing recovery watchdog: 300 seconds from real-root mounting (roughly 330 seconds total boot uptime). NEVER disable/kill it to extend a test.
- Slot B test boots consume retry count. `set_active b` resets the SAME test slot if retry-count:b is zero/unbootable; it is not a request to flash anything. `fastboot continue` previously returned LoadError; use ordinary reboot after checking state.
- The owner is frustrated by rushed physical questions. When asking them to move/wear/check the device, WAIT for their actual reply and leave the scene unchanged. A “please wait” means wait. Do not infer readiness from elapsed time.
- For a visual scene, boot and calibrate first; then ask wearer to put it on and look forward, wait for “ready”, and launch immediately. Earlier forward direction was captured while the headset was on a table, making the cube appear behind them.
- Preserve the owner's requested note: “It's like being at the fucking eye doctor.” This referred to the iterative stereo alignment test, already recorded in the main handoff.
- Do not spawn additional agents without explicit authorization. Next agent is to receive this handoff from the owner.

## Locations and access

Local workspace: /Users/<user>/work/quest-pmos-bringup
Main historical handoff: /Volumes/vela/Backups/quest1-recovery/pmos/MONTEREY-PMOS-HANDOFF.md
WiFi/ADSP handoff: /Volumes/vela/Backups/quest1-recovery/pmos/ADSP-WIFI-HANDOFF.md
Archived head-tracking artifacts: /Volumes/vela/Backups/quest1-recovery/pmos/head-tracking/
Stock partition backups: /Volumes/vela/Backups/quest1-recovery/<SERIAL>/
Mac staging: /private/tmp/claude-501/quest-pmos/

Quest serial: <SERIAL>; unlocked; test slot B.
Fastboot executable: /Users/<user>/Library/Application Support/QuestStack/platform-tools/37.0.1/osx-universal/fastboot
Local ./quest-ssh wrapper uses root@172.16.42.1 with local id_ed25519 and known_hosts. Do not copy/commit the private SSH key. USB NCM interface typically en15, host address 172.16.42.2. SSH/X usually ready 65–80 seconds after reboot; connection refused earlier is expected.
No scp/SFTP server on headset. Transfer with `cat | ./quest-ssh 'cat > /tmp/file'` or tar piped to `./quest-ssh 'tar -C /tmp -xf -'`.
Debug fallback: QUEST_DEBUG_PORT=2324 python3 debug-shell.py 'COMMAND' (real root); port2323 is initramfs.
Xorg DISPLAY=:0 accessible by root; XDG_RUNTIME_DIR=/run/user/10000.

Kali build host: <user>@<BUILD_HOST_IP>, SSH key auth, sudo -n. Sometimes temporarily unreachable; retry before assuming it is off.
pmbootstrap: /home/<user>/pmos/pmbootstrap/pmbootstrap.py
Port: /home/<user>/pmos/pmaports/device/testing/monado-oculus-monterey/
Rootfs chroot: /home/<user>/pmos/work/chroot_rootfs_oculus-monterey
Build log: /home/<user>/pmos/work/log.txt
Build runtime: `python3 pmbootstrap.py build --lax monado-oculus-monterey` from pmbootstrap directory. --lax preserves build dirs; strict mode wiped them and wasted rebuild time. Full build takes minutes; 22 test targets.
Small C compilation: `python3 pmbootstrap.py chroot -r --output stdout -- cc ...` (lowercase stdout). Rootfs has gcc/build-base, libx11-dev, openxr-dev.
Do not interfere with unrelated op25 processes on Kali.

## System baseline and unfinished broader goal

Kernel 4.4.205-perf, real framebuffer 2880x1600; Xorg fbdev rotates UD, software labwc nested X11. Upright readable desktop confirmed. Both controllers move pointers; BTN_LEFT observed, UI click behavior not fully confirmed. Controller helper stream fix removed --observe and uses --print-every-ms16 --stream ID. Stock runtime mount helper selects system_a read-only for pmOS system_b root; /dev/sda6 mounted ro,noexec,noload.

Signed patched Monado r1 is persistently installed. Desktop service autostarts. Head-tracking runtime does NOT autostart. Diagnostic demos and mesh are normally copied into /tmp and disappear on reboot. GPU acceleration has NOT been verified; Mesa Lavapipe/null compositor used for diagnostic OpenXR service. WiFi still unfinished; modem PIL disabled; audio, positional tracking, passthrough, and complete OS integration unfinished. See ADSP-WIFI-HANDOFF.md for earlier WiFi specifics.

Guarded boot image: /private/tmp/claude-501/quest-pmos/pmos-boot-4k-rootready-final.img
SHA256: 621190ea2880728264953fbb559719e91a6bcbd1d3b36d7cb73262c094d675fb
Desktop sparse image: /Volumes/vela/Backups/quest1-recovery/pmos/4k-bringup/pmos-system-desktop.sparse.img
No reflash is required for the next step.

## Tracking fix: implemented, tested, installed, wearer CONFIRMED

Source: local monado-source/, fork Block-Flock/monado-oculus-monterey pinned e160863ad52c0e1c29c8bb7df3e47facdb02288d.
Patch: 0001-monterey-head-frame-startup.patch
APKBUILD: monado-APKBUILD (original saved separately)
Installed package: monado-oculus-monterey-25.1.0_git20260822-r1.apk
SHA256: c82cfd16ff2d20c71bc02b389076048257e6956b09688ab5aec505c91157c49d

Raw IMU type0x50: u64 timestamp, accel3 floats in g, gyro3 floats degrees/sec, metadata. /dev/syncboss_stream0 read-only; IMU enable/disable commands 6e0000/6f0000 on /dev/syncboss0. Probe streams were closed cleanly.
Measured right-handed transform: head=(-sensor.y,-sensor.x,-sensor.z). Head +Y up, -Z forward. Both accel and gyro mapped identically.
- Flat level capture: 5971 samples, acceleration mean (-9.851389,0.022999,0.744476)m/s²; gyro mean (0.012934,-0.006936,0.003589)rad/s.
- Controlled front/visor edge lifted 30–45 degrees while back edge stayed down; wearer replied “dne”. Held accel (-7.74027,0.18207,6.07913)m/s².
- Accel tilt change31.146 degrees, integrated gyro pitch31.385 degrees using assumed microsecond ticks; ratio1.0077 supports normal gyro scale.

Driver changes:
1. Sensor-to-head transform.
2. Continuous stationary startup window 0.75s, >=100 samples; gyro norm<.08rad/s; accel8.5..11m/s²; gyro variance<.008²/axis and accel variance<.01.
3. Seed orientation from mean gravity to world+Y, estimate gyro bias in RAM.
4. Return world-space angular velocity by rotating debiased head-space gyro for xrt relation/prediction.
5. Reject nonfinite IMU data; stale after100ms; gaps>100ms reset/recalibrate; fail startup after10s without stable window.
No calibration or firmware data written to device NV.

All 22 test targets pass. New coverage includes tilted/inverted starts, invalid gravity, noisy/moving calibration, five-second stationary fusion with bias, axis basis, yaw/prediction, lifecycle, stale samples. test-heading-recenter.c independently validates heading-only recenter. Patch generation scripts are NOT idempotent; do not rerun blindly: patch-tracking-startup.py, add-tracking-calibration.py, add-tracking-tests.py. Original source files saved adjacent as .original.

Device stationary result: 0.1495deg change over14.396s; head-frame bias about(.006810,-.012350,-.003433)rad/s. Logs tracking-patched-stationary*.log, tracking-test-results.log, monado-tracking-build-final.log.
Wearer: “the orientation and tilt and everything looks fine, but the field of view is off. like lines are moving too fast”. Directions/tilt are confirmed; do not undo them.

## Stereo and lens optics: wearer CONFIRMED in diagnostic renderer

Initial apparent blur was binocular misalignment: one eye alone saw one clear chart. Two eyes saw overlapping charts (~25% overlap). Shifting each eye100 native pixels toward logical screen middle fixed alignment. Centers left820 and right-local620 (=global2060). Owner requested another100, but that build NEVER ran; then said the last view before reboot was perfect. Confirmed offset is100, NOT200.

Unwarped FOV90→105 was worse;90→75 also worse. Wearer reported lines curved when tilting. Stop guessing FOV to compensate for missing lens correction.

Read-only extraction from stock system_b.img using /opt/homebrew/opt/e2fsprogs/sbin/debugfs found /system/etc/calibration/distortion-mesh.bin. Local stock-optics/distortion-mesh.bin, 52368bytes. xrs-hmdconfig.capnp.bin also saved but not used.
Mesh layout inferred and tested:
- magic0x56347807, 96-byte header, 32x32 blocks, panel2880x1600.
- offset8 u64device0x101;16 u64headset0x103;24/28 block counts32.
- offset32 lens separation .053428;40/44 panel dimensions .1188/.066 meters.
- offset48/52 panel2880/1600;56/60 eye texture1216/1344.
- offset64 floats FOV left(up47,down53,left52,right42), right(up47,down53,left42,right52).
- Rows0..32, then eyes0/1, columns0..32, RGB channels0..2, each float pair ray tangent(x,y). Rows bottom to top: X11y=(32-gy)*H/32.
- Finite/shape/fold guards; positive Jacobian; near mirror error max2.9e-5 tangent.
- Inverse bilinear Newton maps target ray to grid. Green ray0 centers left792.39/right647.61 local; Y734.06. Add ±27.61 to retain confirmed820/620 centers.
- 6144 direct inverse round trips passed Mac AND aarch64, worst .000908pixel.

Known visually correct SLOW renderer: monterey-head-mesh.c + quest-lens-mesh.c/.h, binary monterey-head-mesh, wrapper run-head-mesh.sh; logs head-stock-mesh-visual.log and mesh-demo-build.log.
It draws each world line in24 segments and RGB channels separately with Xlib GXor. Mesh replaces guessed FOV projection. Heading-only recenter preserves gravity; R/Space/controller ButtonPress recenter, Escape exits. Default180s alarm; --pose-only15s, --static skips Monado. App verifies real Quest HMD and quaternion validity.
run-head-mesh.sh starts its own null-compositor service, sleeps12s for calibration, runs sibling monterey-head-mesh, kills/waits owned service on exit; refuses existing service. This wrapper has NOT been changed to launch the new fast binary.

Wearer initially said black, then corrected: scene behind them, and optics now looked correct; frame rate tanked. Rough slow observations ~19fps facing geometry and~27fps behind it. Not a controlled matched benchmark. Old loop also sleeps33ms AFTER CPU work, imposing <=30fps before rendering overhead.

CRITICAL SCOPE: optical correction currently exists ONLY in the standalone X11 diagnostic renderer. Monado's device view/distortion configuration still has provisional90-degree/no-distortion setup. Do NOT claim systemwide OpenXR optics are fixed. Tracking corrections are in the real runtime; optics integration remains future work.

## Performance fix: local build ready, headset testing NOT DONE

Separate mesh-fast/ preserves the wearer-confirmed slow baseline. Files:
- monterey-head-mesh.c (optimized source)
- quest-lens-mesh.c/.h (optimized mapping)
- test-lens-mesh.c
- monterey-head-mesh-fast (aarch64 executable)
- test-lens-mesh (aarch64 executable; replaced the earlier Mac test executable here)
- build.log

Changes:
1. Precompute 2x3x513x513x2 float inverse LUT (~12.6MB) at startup, ray range[-4,4], spacing8/512. Invalid samples NAN. Bilinear four-neighbor lookup; invalid neighborhood falls back to exact inverse. Float index clamp prevents edge overflow.
2. Exact inverse rejects |ray|>4 and breaks when clamped Newton step stagnates.
3. Rotate world line endpoints once, interpolate rotated points instead of repeated quaternion rotations per subdivision/channel.
4. Preserve24 segments, RGB correction, all established optics alignment.
5. Remove fixed33ms sleep; target16.666667ms frame budget (~60fps), sleep remainder only. XSync after XCopyArea accounts for X processing. Prints measured cumulative render loop fps each120 frames and cache startup time.

Mac measurements from actual C test, prior to replacing local test binary with ARM build:
- Cache startup0.429s.
- Worst cached-vs-exact deviation0.0294pixels over6144 cases (assert<.25).
- 100k lookups: exact .0045s, cached .0005s (~9x lookup improvement).
- Exact inverse roundtrip max .000908pixel; mirror/bounds tests pass.
These are NOT device frame rate results and do not establish full-renderer speedup.

ARM build completed cleanly with -O3 -Wall -Wextra at logged02:49:16 (build.log). Kali source copy /home/<user>/pmos/mesh-fast; chroot /tmp/mesh-fast. Commands in chroot:
cc -O3 -Wall -Wextra monterey-head-mesh.c quest-lens-mesh.c -o monterey-head-mesh-fast -lX11 -lopenxr_loader -lm
cc -O3 -Wall -Wextra test-lens-mesh.c quest-lens-mesh.c -o test-lens-mesh -lm
Both ARM executables fetched locally successfully. No test on ARM of this NEW cached version yet. No new wearer validation yet.

## Concrete resume sequence (only after owner resumes)

1. Check USB/fastboot/SSH, remaining watchdog uptime. If retry count exhausted reset SAME slotB and reboot, do not flash. Wait for SSH/X.
2. Transfer fast executable, fast test executable, old baseline executable, and stock distortion-mesh.bin to /tmp using tar/SSH. Both renderers look for distortion-mesh.bin beside their executable by default.
3. Run `/tmp/test-lens-mesh /tmp/distortion-mesh.bin` on the headset; save results.
4. Matched static baseline/optimized benchmark: `DISPLAY=:0 timeout 12 /tmp/monterey-head-mesh --static`, then `DISPLAY=:0 timeout 20 /tmp/monterey-head-mesh-fast --static`. --static needs no Monado/sensor ownership. Save logs; startup cache is excluded from internal measured loop fps. These show a static scene, no physical wearer question needed.
5. If still slow, profile actual draw CPU/XSync costs; preserve known-good lens geometry. Lookup9x does not imply total renderer9x. No hardware-GPU claim.
6. For visual tracking, start one monado-service with DISPLAY=:0, XDG_RUNTIME_DIR=/run/user/10000, XR_RUNTIME_JSON=/usr/share/openxr/1/openxr_monado.json, XRT_COMPOSITOR_NULL=1, XRT_NO_STDIN=1, MONTEREY_LOG=info. Capture PID/log. Check stable calibration (gravity initialization) and real Quest presence. Do not start a second runtime.
7. Once calibrated/boot ready, ask wearer to put headset on/look forward, WAIT for reply. Launch /tmp/monterey-head-mesh-fast immediately after actual readiness, using the running service, so heading points forward. Clean up only owned runtime when done.
8. Ask whether motion is smoother while lines still look correct, then WAIT without changing scene. Account for watchdog budget; don't demand a response within it or silently change test.
9. Save device logs, actual frame-rate result and wearer observations. Frame loop fps is not proven panel refresh or end-to-end motion-to-photon latency.
10. Integrate optics into runtime and broader pmOS/GPU/WiFi work later; don't confuse this diagnostic scene with finished OS support.

## Sorting/committing guidance

No commit was made in this paused turn. Preserve slow baseline and new optimized version separately until on-device validation. Do not blindly git-add the whole directory: contains private SSH key, raw captures, APK/runtime archives, framebuffer dumps, image files, and source archives. Review repository boundaries and existing status before sorting/committing. Patch/APKBUILD, relevant C/header/tests/wrappers and human-readable notes are the likely code artifacts; keep binaries/logs/backups as evidence separately. Do not delete historical evidence to tidy the workspace.

New handoff and a copy of mesh-fast plus checksum manifest are archived under head-tracking/performance-pause-20260930/. Historical head-tracking archive and original workspace retain the other sources and logs. This handoff is mirrored in the workspace for convenient reading.
