# Oculus Quest 1 (monterey) → postmarketOS: full bring-up handoff

> **Owner confirms improvement; VR-only requirement:** Corrected submitted-frame scene is visible and definitely better, still not fully smooth. Owner never wants desktop mode. Desktop default-runlevel link removed, service stopped, Xorg/labwc exit verified. Direct launcher no longer restarts desktop. SSH/controller/recovery services preserved. Next work is frame pacing/presentation analysis and native VR compositor integration; diagnostic lens/performance path is not systemwide yet. Repo docs/tracking-optics-performance.md is the current full handoff.

> **Corrected display pipeline latest:** First direct framebuffer trial was black: missing MDSS frame submission. Fixed with FBIOPAN_DISPLAY and a two-CPU-buffer submission worker. Now measured59.7fps render loop AND59.8fps MDSS driver during static and live-tracking trials. Source /Volumes/vela/src/quest1-nura-port/renderer/fast; details in repo docs/tracking-optics-performance.md. Current corrected visual test running, wearer confirmation pending. Archive head-tracking/direct-framebuffer-20260930/ updated.

> **Latest work now in Git repo /Volumes/vela/src/quest1-nura-port:** See its docs/tracking-optics-performance.md. Direct framebuffer diagnostic benchmarks59.7fps versus X11 19.5fps. Installed device binary /root/quest-mesh-fast/monterey-head-mesh-fb. First tracked retry could not calibrate while worn; requested headset resting still, validation pending. Sources/evidence archived in head-tracking/direct-framebuffer-20260930/. Watchdog unchanged.

> **Deployment update — 2026-09-30, after owner resumed with “can you push the framerate fixes to device”:** Optimized bundle is now persistently installed at `/root/quest-mesh-fast/` on the headset. Includes optimized renderer, stock mesh, ARM test, baseline renderer, and `run-head-mesh-fast.sh` (manual launch only, no autostart). No partition flashing or watchdog changes. Booted existing slot B (retry count6; no reset needed this turn). ARM tests pass: 6144 inverse round trips, max .000908pixel; cached max deviation .0294pixel; cache startup1.158s; 100k exact lookups .0213s vs cached .0041s. **Full static renderer still measures19.5fps** (367frames over about18.8 render seconds after cache startup); matched old baseline234frames in12s (~19.5fps). Thus installed optimization has NOT fixed overall framerate; further profiling of rendering/display costs needed. Baseline first lacked executable permission; chmod755 then rerun successfully. Optimized binary SHA256 50f7017430f3532a0a285d788ecc09834d6e3387af296cb7cc5e288f40d68199. Tests ended cleanly; no tracking scene or Monado service started this turn and no wearer question pending. Last device uptime about2min; watchdog remains active. Logs in local mesh-fast/device-validation.log, device-benchmark.log, device-baseline.log and archive head-tracking/performance-deployed-20260930/. Historical “not transferred/tested” statements below are superseded.

> **PAUSED at owner request — latest state 2026-09-30:** Read [TRACKING-OPTICS-PERFORMANCE-HANDOFF.md](TRACKING-OPTICS-PERFORMANCE-HANDOFF.md) FIRST. Wearer confirmed the stock lens mesh now looks correct; apparent black screen was scene behind them. Frame rate tanked. Optimized cached renderer built in local `mesh-fast/`, with Mac numeric tests passing, but **NOT yet transferred or tested on headset**. Device work is paused for handoff; watchdog remains intact. New code/evidence/checksums archived in `head-tracking/performance-pause-20260930/`. Earlier “mesh validation pending” and other status blocks below are historical and superseded. No commit or new flash in this paused turn.

> **Lens warp diagnosis and stock mesh found — 2026-09-30:** Both plain-FOV
> changes were rejected: 105 degrees worse; 75 degrees also worse. Owner says
> “when i tilt my head the lines become curved”. Stop FOV guessing. Existing
> direct X11 rendering had no optical pre-warp.
> Read-only debugfs extraction from the verified stock system_b.img found
> /system/etc/calibration/distortion-mesh.bin (52368 bytes). Header magic
> 0x56347807, 32x32 blocks, panel2880x1600. Header FOV tuples
> (up47,down53,left52,right42), mirrored for right eye.
> Inferred data layout verified by size/monotonicity/near-mirror symmetry:
> 96-byte header, rows then eyes then 33 columns, three float-pair ray channels.
> Built quest-lens-mesh.c inverse-bilinear decoder with finite/shape/fold guards.
> 6144 inverse round trips pass on Mac and aarch64, max error0.000908pixel.
> Center rays map to left792.39/right647.61 local pixels and Y734.06 (X11);
> adds +27.61/-27.61 pixels to retain wearer-confirmed centers820/620.
> New monterey-head-mesh renders subdivided curves per RGB channel with GXor
> compositing, retains r1 tracking/recenter. Stock mesh rows are interpreted as
> bottom-to-top; wearer validation still pending. Binary/source/mesh/wrapper
> archived in head-tracking/. Waiting for wearer readiness before boot/test.
> No lens factory data or NV was written; everything is userspace diagnostics.

> **Tracking directions VERIFIED by wearer — 2026-09-30:** Installed signed
> monado-oculus-monterey r1. All 22 test targets passed including new regression
> coverage; standalone heading-recenter math test passed. On-device stationary
> calibration succeeds (head-frame gyro bias about 0.00681,-0.01235,-0.00343 rad/s).
> Orientation change 0.1495 degrees over 14.396 seconds at rest. Wearer: “the
> orientation and tilt and everything looks fine, but the field of view is off.
> like lines are moving too fast”. Thus directions/tilt are confirmed, preserve
> them. Remaining task is projection angular scale and eventual lens distortion.
> Controlled front-lift capture: accelerometer tilt change 31.146 degrees and
> integrated gyro pitch 31.385 degrees (device ticks assumed microseconds), ratio
> 1.0077, supporting normal gyro gain. Original demo HFOV 90 degrees; prepared
> monterey-head-fov.c default 105 degrees, configurable 70..130 via
> MONTEREY_DEMO_HFOV_DEG. FOV version building; not visually tested yet.
> +100 pixel per-eye inward center offset remains unchanged.

> **Tracking correction in progress — 2026-09-30:** Owner explicitly requests
> fixing axes/startup and volunteered that headset is horizontally flat on level
> surface. Stationary capture 5971 samples: acceleration mean
> (-9.851389, 0.022999, 0.744476) m/s²; gyro mean
> (0.012934, -0.006936, 0.003589) rad/s. Gyro stddev ~0.0015–0.0022 rad/s.
> Controlled front/visor-edge-up tilt, owner confirmed done: acceleration while
> held (-7.74027, 0.18207, 6.07913) m/s². Both sensor streams were closed cleanly.
> Thus right-handed sensor-to-head basis proposed/tested in code is
> head=(-sensor.y, -sensor.x, -sensor.z). Head +Y up, -Z forward.
>
> Patch 0001-monterey-head-frame-startup.patch and APKBUILD r1 prepared on Kali:
> apply mapping to gyro+accel; require continuous 0.75s low-motion window for
> initial gravity orientation and gyro bias; refuse startup after 10s without
> stability; invalidate stale tracking after 100ms and recalibrate after gaps;
> rotate debiased gyro to world frame for xrt_space_relation/prediction.
> Regression tests cover orientation starts including inverted, invalid data,
> calibration movement/noise, five seconds stationary fusion, sensor lifecycle,
> stale tracking, axis basis and prediction after yaw. Full build running;
> nothing from this patch has yet been installed or visually validated.
>
> New demo monterey-head-tracking.c preserves +100 px inward alignment,
> recenters heading only (keeps gravity up); trigger/click, R or Space recenters,
> Escape exits. Build/visual verification pending. Do not claim axes fixed until
> wearer confirms. Logs monado-tracking-build-final.log and Kali work/log.txt.

> **Corrected head-tracked scene verified — 2026-09-30:** monterey-head-optics.c
> built cleanly with -Wall -Wextra and ran on headset via run-head-optics.sh.
> Applies 100 logical pixels inward per eye, plus per-eye clipping. Owner:
> “okay, much clearer. now i can see that the axes are not moving correctly
> with my head” and asks about “this is up” calibration. Preserve display
> correction; motion axes/reference orientation still unvalidated and wrong
> by wearer report. Stock Meta runtime corrections are not provided by pmOS.
> Review sensor-to-head basis AND initial recenter/fusion alignment separately.
> Driver feeds raw accelerometer/gyro axes directly into m_imu_3dof, starts
> identity, and gradually corrects gravity. Demo snapshots origin at startup
> after only 0.5 seconds, so later gravity settling is another possible issue;
> this is a hypothesis, not a verified axis fault. Do not infer mapping from
> earlier lying-down captures or change axes blindly.
> Corrected test completed cleanly after 5345 frames; process check confirmed
> both the demo and owned Monado service stopped. Whole boot retains its
> 300-second recovery watchdog. All source, binary, wrapper and logs archived
> in head-tracking/. No tracking-axis changes were deployed.

> **Confirmed chart alignment — 2026-09-30:** Owner subsequently says the chart
> was “perfect” before the recovery reboot. The last ACTUALLY DISPLAYED setting
> was +100 logical pixels inward per eye (left center x=820, right center x=2060
> on X11 2880x1600). +200 was NOT compiled/deployed because Kali SSH went offline.
> Use +100 as the wearer-confirmed central chart alignment, not +200.
> This is an empirical screen-center correction, not a measured IPD or full lens
> calibration. Head-tracked scene with this correction still needs visual validation.
> Prepared monterey-head-optics.c adds this correction and per-eye clipping;
> original demo source is preserved. Build currently awaiting Kali connectivity.

> **Optics wearer observations — 2026-09-30:** Native stationary chart is sharp
> in each eye separately. Both eyes see two charts overlapping about 25% or more.
> Moving each chart 100 pixels toward the logical screen middle noticeably
> improves alignment. Owner requests another identical 100-pixel step.
> Owner explicitly asks notes to record: “It’s like being at the fucking eye doctor.”
> Do not call this optical blur or sensor-axis failure; binocular alignment is
> the current demonstrated issue. +200 pixels per eye is requested but not yet tested.

> **Visual head-pose milestone and optics follow-up — 2026-09-30:** Monado
> 25.1.0_git20260822-r0 built successfully; all 22 tests passed. Installed runtime
> and Mesa software Vulkan on the headset with signatures verified. Excluded
> already-installed eudev and xkeyboard-config APKs carrying an older local key.
> Root usage afterward ~740 MiB with ~999 MiB available. No runtime autostart added.
>
> XRT_COMPOSITOR_NULL=1 service and XR_MND_headless OpenXR client recognized
> Oculus Quest 1 (Monterey) and returned changing orientation. The 60-second
> X11 stereo wireframe completed cleanly; owner explicitly confirmed seeing it.
> Owner reports blur that did not feel like physical focus. The sideways view
> was because owner was lying down: NOT evidence of an axis mapping fault.
> Service was stopped with SIGTERM after the test. Recovery watchdog remains.
>
> Visual test bypasses Monado display rendering and has NO lens distortion
> correction, guessed eye spacing/projection, native 2880x1600 X11 rendering.
> Do not claim calibrated stereo, validated axis response, or positional tracking.
> Next prepared diagnostic: monterey-optics-check (C/Xlib), identical stationary
> central charts with native pixel patterns, no sensor access, 180-second limit.
> Built successfully with -Wall -Wextra. Not yet run on headset. Waiting for
> owner readiness before booting/showing it; wait for physical-test replies
> before changing scenes or drawing conclusions.
>
> Repeated guarded boots exhausted slot B retry count; fastboot reported B
> unbootable with retry count 0. set_active b then reboot restored normal guarded
> boot. No partition images were flashed in this head-tracking work.
> Latest observed headset state before optics test: fastboot.

> **Head tracking now targeted — 2026-09-30:** Owner explicitly selected
> headset head tracking as the next goal (rotation first; no positional tracking
> claimed). Pinned Monado fork e160863ad52c0e1c29c8bb7df3e47facdb02288d source
> audited: exact IMU enable 6e 00 00, disable 6f 00 00 on /dev/syncboss0;
> reads /dev/syncboss_stream0, requires type 0x50 samples before device creation,
> uses m_imu_3dof and provisional optics. No modem or firmware-update operations.
>
> Bounded probe enabled/read/disabled IMU successfully. Long capture contains
> 49,910 valid samples; gyro peaks 1.303,3.728,1.187 rad/s during owner-confirmed
> head movement, median device timestamp delta 985 (units presumed microseconds,
> not yet independently calibrated), all parsed numeric values finite. This is
> verified physical HMD sensor input, not yet visual head tracking.
>
> Monado package building on Kali via pmbootstrap; log local
> quest-pmos-bringup/monado-build.log and Kali ~/pmos/work/log.txt. Standalone
> monterey-head-demo.c prepared to link actual Monado Monterey device/fusion
> and render an X11 stereo wireframe for 60 seconds, with signal cleanup and no
> simulated pose fallback. Build/display/orientation validation remains pending.
> Headset returned to fastboot normally under watchdog while build runs.

> **Controller stream fix — 2026-09-30:** After a fresh guarded boot, a
> single stock-tool owner reported BOTH controllers connected, battery 100%,
> with zero bad magic/checksum/rejected transaction counters. Earlier packet
> errors are therefore not a proven persistent compatibility fault.
>
> Direct non-observer `--stream` produced live thumbstick and trigger samples;
> observer readers produced none. Changing only the gateway's stream case from
> `run_tool --observe --print-every-ms 16 --stream "$2"` to
> `run_tool --print-every-ms 16 --stream "$2"` and restarting the bridge produced
> 68 Linux input events: REL_X and REL_Y movements plus BTN_LEFT press/release.
> This verifies the input path for one active controller; second-controller and
> visible pointer confirmation are pending. Both controllers remain paired.
> Original daemon mode `--list-watch-and-enum` is retained. A stream-per owner
> experiment did not fix input and was REVERTED before this successful test.
> No reset, re-pairing, calibration writes, or controller firmware updates used.
>
> `4k-bringup/desktop-trial-fixes.tar.gz` now also includes the fixed
> /usr/sbin/oculus-controller, alongside slot helper and desktop startup service.
> `controller-active-events.raw` is the captured aarch64 input_event stream
> (24 bytes/record); `oculus-controller-fixed` is the exact tested gateway.
> Last observed: guarded desktop running at ~140s uptime, automatic fastboot
> recovery still armed. Owner confirms visible desktop upright and readable.

> **Wearer confirmation and input diagnosis — 2026-09-30:** Owner confirms
> desktop terminal is upright and readable through the lenses. Both controller
> uinput devices exist and Xorg loads libinput for both, but a 15-second evdev
> capture produced zero bytes. Kernel logs show SyncBoss RX magic 0xcacacaca
> instead of expected 0xDEFEC8ED and rejected transactions. This does not yet
> identify the cause. Missing PROX_PS_* calibration and observer FIFO-full logs
> also occur. Do not infer controller firmware damage or attempt firmware updates.
> Watchdog subsequently returned device to fastboot as expected. A further guarded
> boot is starting for a single-owner input test. Logs: controller-no-events.log.

> **Desktop milestone — 2026-09-30:** Kali reconnected. Built a 2400 MiB
> system image from the verified SSH snapshot, expanded root to ~1.9 GB with
> resize2fs, retained UUIDs/4096-byte blocks and byte-identical embedded boot FS.
> Installed labwc, Xorg/fbdev, fonts, xterm, dbus and the port's no-VT launchers.
> Image passed e2fsck -fn and GPT validation; sparse decode matched raw SHA256.
> Flashed only system_b; existing guarded boot_b unchanged. Source-built kernel
> now runs Xorg + labwc + xterm. Direct framebuffer capture shows the terminal;
> wearer confirmation remains pending. No claim of stereo/VR/acceleration.
>
> Second boot VERIFIED automatic startup of oculus-desktop-trial,
> oculus-stock-runtime, and oculus-controller. USB SSH works; watchdog remains
> armed for 300 seconds. Last observed state: running desktop trial at ~100s
> uptime; it will automatically return to fastboot. Do not disable watchdog.
>
> Controller runtime failure fixed: this boot omits androidboot.slot_suffix.
> Helper now falls back to the pmOS root mapper's single backing PARTNAME,
> refusing unknown or multiple backing devices. Live result selects system_a
> (/dev/sda6) read-only, while pmOS remains system_b. Tested positive and two
> negative cases. Stock tool lists both paired controllers, disconnected;
> controller bridge starts, but controller input is NOT yet demonstrated.
> Added xf86-input-libinput after first display trial.
>
> Reproduction artifacts in 4k-bringup/:
> - pmos-system-desktop.sparse.img: decoded size 2516582400 bytes, decoded SHA256
>   efe1bcff08d126b783f37c97a1438570c539fdb248429b0eb385c643b29ad3b0.
> - Use verified pmos-boot-4k-rootready-final.img from prior milestone.
> - Raw image fastboot failed BEFORE sending due to host footer-read path;
>   sparse format flashed successfully using -S 256M. No verity bypass flags.
> - Sparse image predates follow-up input/slot/autostart changes. After boot,
>   unpack desktop-input-apks.tar.gz into /tmp and install offline with
>   apk add --no-network /tmp/desktop-input-apks/*.apk; extract
>   desktop-trial-fixes.tar.gz at / to apply the tested helper and service.
> - desktop-first-boot.log, desktop-second-boot.log, desktop-fb.png,
>   desktop-build-complete.log and build-desktop-image.py retain evidence.
> - The attached headset already includes these follow-up changes.
>
> Outstanding: wearer display confirmation, controller connection/input,
> Wi-Fi modem/WLAN protection-domain offline investigation, audio and VR gaps.
> No modem voting, firmware updates, NV writes, or boot-chain changes performed.

> **Goal and display update — 2026-09-30:** Owner wants a usable pmOS headset,
> including display and Wi-Fi, not just an SSH boot. ADSP now boots ONLINE;
> corrected Wi-Fi findings are in ADSP-WIFI-HANDOFF.md. WLAN service manifest
> is modem/wlan_pd, not ADSP audio_pd; modem PIL remains untested/disabled.
>
> Current installed diagnostic root is only 257.8 MiB (109.6 MiB free) and
> configured ui=none. Live source-built kernel exposes /dev/fb0, /dev/kgsl-3d0,
> mdssfb_80000, 32 bpp, virtual_size 2880,3200, stride 11520 and mode
> U:2880x1600p-90. This proves framebuffer registration, not visible desktop or
> measured refresh. /proc/asound/cards reports no soundcards.
>
> Local port README says software labwc/Xorg fbdev desktop worked with an OTA
> kernel; source-built kernel panel validation still pending. It explicitly
> lists GPU acceleration, tracking, passthrough, Wi-Fi and audio as unfinished.
> A desktop-only apk simulation on Kali resolved 96 packages, estimated full
> root size 419.4 MiB. Need expand root image within system_b before installing.
> Keep existing guarded boot, UUIDs, 4K block size and verified recovery path.
>
> Started desktop dependencies installation in Kali build chroot (labwc,
> xorg-server, xf86-video-fbdev, xinit, dbus, font-dejavu), but Kali <BUILD_HOST_IP>
> became unreachable before output. Inspect apk state and running pmbootstrap
> processes before retrying. No desktop image built or flashed. Headset was
> returned to fastboot and serial verified after display discovery.

> **Latest verified state — 2026-09-29 late evening:** pmOS now boots through
> `switch_root` into OpenRC default runlevel with working key-authenticated SSH.
> Both filesystems use 4096-byte blocks. The guarded real-root watchdog was
> observed returning the fully booted device to fastboot. Device is currently
> LEFT IN FASTBOOT, slot B, with the verified guarded boot image installed.
> Wi-Fi driver loads, but wlan0 is still absent; ADSP bring-up remains unfinished.
>
> **Reproducible working pair:** `4k-bringup/pmos-boot-4k-rootready-final.img`
> and `4k-bringup/pmos-system-4k-ssh.img`. The latter includes the networking,
> SSH-key, DHCP-service, and rmtfs-alias fixes. It was captured with filesystems
> unmounted after `e2fsck -fn` passed; both superblocks require no journal replay.
> This is still a diagnostic boot: 30-second pre-switch pause, 300-second
> watchdog, USB root debug port 2324. See the appended full-boot findings below.
> The original sections describe historical blockers; block-size fix is now
> TESTED AND CONFIRMED. Do not use the earlier unguarded rootdebug images.

_Author: prior agent session, 2026-09-29. Written for a fresh agent to pick up cold._
_Supersedes `BRINGUP-FINDINGS.md` (kept as the short summary)._

---

## 0. TL;DR — the one thing to try first

The port gets a from-scratch pmOS kernel booting on the Quest 1, brings up USB
networking, and correctly maps the rootfs — but **`switch_root` fails because
mounting the ext4 rootfs oopses the kernel** and the device drops off the USB bus.
The previous session called this an "unsolved kernel bug." **It probably isn't.**

**Root-cause hypothesis (high confidence, untested fix):**
The Quest's UFS reports **`logical_block_size = 4096`** (confirmed:
`/sys/block/sda/queue/logical_block_size = 4096`, `minimum_io_size = 8192`).
But `pmbootstrap`'s `mkfs.ext4` created **pmOS_root with a 1024-byte block size**
(confirmed: `dumpe2fs … | grep 'Block size' → 1024`; `blkid → BLOCK_SIZE="1024"`).

A filesystem block size **must be ≥ the block device's logical block size**. The
Linux block layer cannot issue sub-`logical_block_size` I/O. ext4 with 1024-byte
blocks on a 4096-logical device asks the block layer for 1024-byte transfers → on
this downstream 4.4 kernel that BUGs/oopses instead of failing cleanly → device
hangs off the bus.

This explains **every** symptom:
- `blkid` works (metadata read via page cache, 4096-aligned) — FS looks valid.
- `mount` crashes (journal replay / group descriptors / inode tables = real block I/O).
- The "compat" image (ext4 features stripped with `tune2fs`) **still** crashed —
  because `tune2fs` does not change block size; it was still 1024.

**The fix to test first:** rebuild the rootfs (and boot fs) with a **4096-byte
ext4 block size** so FS block size == device logical sector size. See §6.

---

## 1. Why this matters / context

- The Quest 1 bootloader-unlock exploit chain (QuestStack: GhostLock/ionstack
  temp-root + CVE-2021-1931 ABL overflow) is **~1 month old** as of this writing —
  this is fresh ground and the pmOS port (Block-Flock / pmaports-oculus-monterey)
  is early-stage. Cracking the mount crash likely unblocks the whole port for the
  community.
- Device is fully unlocked (`ro.boot.flash.locked=0`, verifiedbootstate=orange),
  fully backed up (see §3), and on-demand rootable. **There is a known-good Android
  restore path**, so experiments are low-risk. The owner is fine with cop-frame
  drops but "not trying to brick my one device" — always keep a watchdog armed and
  keep the backups pristine.

---

## 2. Hardware / software stack (exact versions)

| Component | Value |
|---|---|
| Device | Oculus Quest 1, codename **monterey**, serial `<SERIAL>` |
| SoC | Snapdragon 835 / MSM8998, WCN3990 integrated Wi-Fi |
| Stock OS | Android 10, build `49845030443200410` (v49), `user` build |
| Storage | UFS, **logical_block_size = 4096** (4Kn), min_io = 8192 |
| Boot model | legacy system-as-root (`ro.build.system_root_image=true`) |
| Verity | old **android-verity** (dm target via bootloader `dm=` cmdline), `veritymode=enforcing`; **no vbmeta partition, no `avbctl`** |
| pmbootstrap | **3.11.1**, git `fde5aad` (2026-09-29) |
| pmaports | git `2254bfc`, grafted with the Block-Flock packages under `device/testing/` |
| Kernel pkg | `linux-oculus-monterey` **4.4.205-r3**, `_commit=6929f734ce0e602018790ff3a52dc7bad646af60` |
| Service mgr | **OpenRC** (systemd is non-bootable on 4.4) |
| UI | none (console/SSH target; no working display/GPU driver yet) |

Kernel config facts (config-oculus-monterey.aarch64): `CONFIG_EXT4_FS=y`,
`CONFIG_EXT4_USE_FOR_EXT2=y`, `CONFIG_JBD2=y`, `CONFIG_FS_MBCACHE=y`,
`CONFIG_DEVTMPFS` **disabled** (the mapper runs `mdev -s` to populate nodes).

---

## 3. Where everything lives

**Kali box `<BUILD_HOST_IP>` (build host):**
- `~/pmos/pmbootstrap` — pmbootstrap 3.11.1
- `~/pmos/pmaports` — grafted aports (the port packages live in `device/testing/`)
- `~/pmos/pmaports-oculus-monterey` — upstream Block-Flock clone (reference)
- `~/.config/pmbootstrap_v3.cfg` — device=oculus-monterey, work=~/pmos/work,
  aports=~/pmos/pmaports, ui=none, jobs=6, ssh_keys=True, service_manager=openrc,
  mirrors.alpine=`https://mirrors.edge.kernel.org/alpine/` (NOT dl-cdn — see §8)
- `~/pmos/work/…` — pmbootstrap chroots/rootfs
- `/tmp/postmarketOS-export/{boot.img,oculus-monterey.img}` — last export
- `~/pmos/logs/*.log` — build/install logs
- `~/pmos/wifi-bringup-experiment.sh` — the Wi-Fi experiment (needs pmOS booting)

**Mac (flash host, headset on USB here):**
- `/private/tmp/claude-501/quest-pmos/` — staging + image variants + port scripts
  (`prepare-monterey-boot`, `flash-verified-slot-b`, `return-to-slot-a`, …)
- fastboot/adb: `~/Library/Application Support/QuestStack/platform-tools/37.0.1/osx-universal/`

**vela backups `/Volumes/vela/Backups/quest1-recovery/`:**
- `<SERIAL>/` — full verified stock backups (boot_b.img, system_b.img [2.56 GB],
  persist, modem NV, vision, all slotted partitions, manifest.csv). `boot_b.img`
  doubles as the BootSignature template for `prepare-monterey-boot`.
- `quest1-root.sh` (→ `~/bin/quest1-root`) — on-demand ionstack root
- `quest1-demeta-session.sh` (→ `~/bin/quest1-demeta`) — de-Meta session script
- `pmos/` — this doc, `BRINGUP-FINDINGS.md`, `wifi-bringup-experiment.sh`

**Restore to stock Android:** `fastboot flash boot_b <vela>/boot_b.img` +
`fastboot flash system_b <vela>/system_b.img`, reboot. (pmOS only ever touches
boot_b + system_b; the bootloader chain on slot B is untouched stock.)

---

## 4. How the port boots (mental model)

pmOS lives **inside** the Android `system_b` partition (`/dev/sda7`). `system_b`
contains an internal GPT with two subpartitions: **pmOS_boot** (ext2) and
**pmOS_root** (ext4). Boot sequence:

1. Quest boots the flashed `boot_b` (custom pmOS boot.img). Header must be v0 with a
   valid Android version field and a legacy 4096-byte BootSignature page grafted on
   (`prepare-monterey-boot` copies it from the stock `boot_b.img` template) or the
   bootloader rejects it — **even though unlocked**.
2. `honor-pmos-initramfs.patch` + `pmos_force_initramfs` on the cmdline forces the
   kernel to use the initramfs (normal system-as-root ignores the ramdisk).
3. Initramfs runs **`oculus-map-pmos-subpartitions`** (full script in Appendix A):
   it reads system_b's embedded GPT via a throwaway 512-sector loop view, then
   creates two **dm-linear** mappings `/dev/mapper/oculus-pmos-{boot,root}` onto the
   4Kn `system_b` device (enforcing 4096-byte / %8-sector alignment).
4. Initramfs then mounts pmOS_root by `pmos_root_uuid` and `switch_root`s into it.
   **← THIS is where it dies (the mount oops, §5 blocker C).**

Debug shell: the previous session patched `oculus-initramfs-recovery-hook` to
background-start `nc -lk -s 172.16.42.1 -p 2323 -e /sbin/oculus-debug-login`
**after** USB net is up (the port's own `oculus.force-debug` is broken — binds nc
before the IP exists and blocks boot). Shell reliably opens ~30 s post-boot on
`172.16.42.1:2323`. A recovery watchdog reboots to fastboot after
`oculus.recovery_timeout` seconds (default 300) — **always leave it armed**; the
prior session hung the device for hours by killing it.

---

## 5. Blockers, with data (in the order they were hit)

### A. cmdline UUID truncation (real, secondary)
pmbootstrap's cmdline (`deviceinfo_kernel_cmdline` + `pmos_boot_uuid` +
`pmos_root_uuid` + `pmos_rootfsopts`) overflows the Android boot header's **512-byte
main cmdline field**, and this bootloader **ignores the extra_cmdline field**. So
`pmos_root_uuid` gets chopped mid-string. Observed in `/proc/cmdline`:
```
… pmos_force_initramfs  pmos_boot_uuid=9f5e7452-f290-4994-9d3c-9a92f272ee64 pmos_root_uuid=d23502fb-5ca4-4cf0-bbf   ← truncated (real: …bbfe-67ad8819c696)
root=/dev/dm-0 dm="system none ro,0 1 android-verity /dev/sda7" … skip_initramfs …
```
(Note the bootloader also **appends** its own args — `root=`, `dm=`, `skip_initramfs`,
`androidboot.*`, `console=none` — after the truncation point.)

### B. Trimming the cmdline breaks USB networking (unexplained)
Removing `veritykeyid=…` / `msm_rtb.filter=…` from `deviceinfo_kernel_cmdline` to
make room for the full UUID **reliably kills USB-NCM bring-up** (no net, no shell) —
reproduced twice. Mechanism never understood. So there's a conflict:
- **Full cmdline** → net + debug shell work, but `pmos_root_uuid` truncated.
- **Trimmed cmdline** → full UUID, but no net (can't even reach the shell).

Note: this may be entangled with blocker C — a trimmed-cmdline boot that *also* hits
the mount oops would look identical ("no net") if the oops happens before/around net
bring-up. Worth re-checking once C is fixed. **Cleaner fix regardless:** patch the
initramfs to mount root by the `/dev/mapper/oculus-pmos-root` device name the mapper
already creates, instead of by `pmos_root_uuid` — sidesteps the 512-byte limit
entirely and makes blocker A moot.

### C. Mounting pmOS_root oopses the kernel — THE CRUX (see §0 + §6)
From the initramfs debug shell, the mapper is confirmed working:
```
dmsetup table:
  oculus-pmos-boot: 0 997376 linear 8:7 2048
  oculus-pmos-root: 0 579584 linear 8:7 999424
blkid /dev/mapper/oculus-pmos-root:
  LABEL="pmOS_root" UUID="d23502fb-…" BLOCK_SIZE="1024" TYPE="ext4"   ← note 1024
kmsg: "Monterey subpartition mapper: pmOS_boot and pmOS_root ready"
```
But:
```
mount -o ro /dev/mapper/oculus-pmos-root /tmp/r   → "Invalid argument" (EINVAL),
                                                     then device drops off USB bus
```
Any `mount` attempt (auto, `-t ext4`, feature-stripped image) crashes the device.
**See §0 for the root cause: ext4 block size 1024 < device logical_block_size 4096.**

### D. ext4 feature incompatibility (real, secondary — already handled)
Alpine's modern `mkfs.ext4` enabled features the 4.4 kernel can't mount:
`orphan_file` (kernel 5.15+), `metadata_csum`, `metadata_csum_seed`. Stripped with:
```
tune2fs -O ^orphan_file,^metadata_csum,^metadata_csum_seed <part>; e2fsck -fy <part>
```
Did NOT fix the mount crash (block size is the real issue), but is still required —
a proper fix belongs in `mke2fs.conf` / the device pkg so mkfs never adds them.

---

## 6. The fix to try (blocker C) — concrete steps

**Hypothesis:** pmOS_root/pmOS_boot must have a **4096-byte block size** to be
mountable on the 4Kn UFS. `mkfs.ext4` defaulted to 1024 because the subpartitions
are small (~283 MB root, ~487 MB boot region).

**Where pmbootstrap makes the fs:** the rootfs/boot filesystems are created during
`pmbootstrap install`. Options, easiest first:
1. **Post-process test (fastest signal, no rebuild):** you cannot change an existing
   ext4's block size in place. But you can prove the hypothesis cheaply by creating a
   tiny 4096-block ext4, `dd`-ing a minimal rootfs or even just testing whether the
   *stock* mount path works — or better, rebuild just the image (step 2).
2. **Force 4096 in the build:** make `mkfs.ext4` use `-b 4096`. Check whether the
   port/pmbootstrap exposes this via deviceinfo (`deviceinfo_rootfs_*`) or the
   subpartition-image builder in the Block-Flock packages
   (`device/testing/oculus-monterey-*` — grep for `mkfs.ext4`/`mkfs.ext2`). If it's
   pmbootstrap-internal, the port likely builds the inner GPT image itself (grep the
   port pkgs for `mkfs`, `sgdisk`/`parted`, `pmOS_root`). Set `-b 4096` there.
3. **Rebuild the inner-GPT image directly:** the two subpartitions are assembled into
   the image flashed to `system_b`. Recreate both filesystems with `mkfs.ext4 -b 4096
   -O ^orphan_file,^metadata_csum,^metadata_csum_seed -L pmOS_root …` (and the boot fs
   likewise), repopulate, reassemble the GPT image, reflash `system_b`.

**Verify before flashing:** `dumpe2fs -h <root> | grep 'Block size'` must say **4096**.
**Verify after boot from the 2323 shell:** `blockdev --getss /dev/mapper/oculus-pmos-root`
(should be 4096) and `mount -t ext4 -o ro /dev/mapper/oculus-pmos-root /tmp/r` must
succeed without the device dropping off the bus.

If 4096 blocks still crash: fall back to true kernel debugging — get the oops via a
serial UART (console=ttyMSM0 is already on the cmdline; needs the physical UART pads)
or a kexec crashdump, and look at whether it's the block layer rejecting the I/O size
or dm-linear on the 4Kn device. But test the block-size fix first.

---

## 7. Reproduce from scratch (fresh agent)

1. On kali: `cd ~/pmos/pmbootstrap && ./pmbootstrap.py build --force device-oculus-monterey`
   then `./pmbootstrap.py install --password <pw>`, then `./pmbootstrap.py export`.
   (Config is pre-seeded; if `init` re-prompts, drive with `yes ''`.)
2. Copy `boot.img` + `oculus-monterey.img` to the Mac staging dir.
3. Finalize boot: `prepare-monterey-boot <boot.img> <vela>/<SERIAL>/boot_b.img <out>`.
4. Get the device into fastboot (owner: power off, then Vol-Down+Power → USB update
   mode) and flash: `fastboot flash system_b <oculus-monterey.img>` then
   `fastboot flash boot_b <finalized-boot.img>`, `fastboot reboot`.
5. Wait ~30 s, connect debug shell: `nc 172.16.42.1 2323` (Mac gets 172.16.42.2).
   Diagnose there. **Keep the recovery watchdog armed** (do not pass
   `oculus.recovery_timeout=0`).

---

## 8. Dead ends / gotchas (don't repeat these)
- **Killing the recovery watchdog hangs the device off-bus** (no auto-recovery) →
  needs a physical power-cycle. Only ever use a short timeout, never 0.
- `mount` of the (1024-block) rootfs **crashes the kernel** — expect the shell to die
  right after; that's the bug, not a flaky connection.
- `dl-cdn.alpinelinux.org` gave a "v2 package integrity error" (index/pkg sync skew)
  → switched `mirrors.alpine` to `mirrors.edge.kernel.org`. Keep it there.
- `pmbootstrap` port packages drift checksums → `pmbootstrap checksum <pkg>` then
  `build --force`.
- macOS occasionally won't re-create the USB-NCM interface after many reboots; if
  `ping 172.16.42.1` fails, check `ifconfig | grep 172.16.42` for the iface itself.
- Magisk root does NOT work here (legacy system-as-root ignores the boot ramdisk).
- fastboot `-c` is wrong; the flag is `--cmdline`.

---

## 9. The original goal — Wi-Fi (downstream of a booting pmOS)
Analysis already done (not yet testable — blocked on §6):
- **NOT** a QRTR kernel port (that was a red herring). ICNSS never gets the WLFW
  `FW_READY` QMI event because the **ADSP** subsystem hosting the WLFW protection
  domain (`adspua.jsn`) is never PIL-booted; `rmtfs` runs `-P -r` (no PIL, read-only).
- **Fix direction:** ADSP-only PIL bring-up + writable rmtfs. The cellular-**modem**
  PIL (which wedged the upstream maintainer) is NOT needed.
- All firmware present on the device: `adsp.mdt`/`.b00–.b11`, `wlanmdsp.mbn`,
  `bdwlan*`. Experiment script: `~/pmos/wifi-bringup-experiment.sh`.

---

## Appendix A — `oculus-map-pmos-subpartitions` (the dm-linear mapper)
Path: `~/pmos/pmaports/device/testing/oculus-monterey-initramfs-support/oculus-map-pmos-subpartitions`
Key logic (see §0/§6 for why block size, not this script, is the likely bug):
- Populates nodes with `mdev -s` (no CONFIG_DEVTMPFS).
- Finds `system_b` by `PARTNAME` in `/sys/class/block/*/uevent`.
- Reads the embedded GPT via a **throwaway** `losetup --sector-size 512 --read-only
  --direct-io=on` view + `fdisk -l`, then **detaches it** (comment: "Never use its
  partition nodes for filesystem I/O on this kernel").
- Extracts partition 1 (boot) and 2 (root) start/length in 512-byte sectors.
- Creates `dmsetup create <name> --table "0 <length> linear <system_b> <start>"`,
  enforcing `start%8==0 && length%8==0` (4096-byte alignment) and bounds.
- Symlinks `/dev/mapper/oculus-pmos-{boot,root}` → the dm nodes.

Full script is in the repo; the alignment/`%8` logic and the 4Kn comments corroborate
that the storage is 4Kn — consistent with the block-size root cause.

## Appendix B — deviceinfo `deviceinfo_kernel_cmdline`
```
androidboot.configfs=true androidboot.hardware=monterey ehci-hcd.park=3 lpm_levels.sleep_disabled=1 sched_enable_hmp=1 sched_enable_power_aware=1 service_locator.enable=1 softdog.soft_panic=1 swiotlb=2048 user_debug=31 bootver=1596585601 cursysver=1596585601 minsysver=1 buildvariant=user console=ttyMSM0,115200n8 earlycon pmos_force_initramfs
```
`deviceinfo_bootimg_custom_args="--header_version 0 --os_version 10.0.0 --os_patch_level 2021-04 --second_offset 0x00f00000"` (bootloader rejects zeroed version fields).

## Follow-up: 2026-09-29 late evening — 4 KiB mount fix CONFIRMED

This session rebuilt BOTH inner filesystems at 4096-byte blocks, retaining the
original GPT, UUIDs, files, ownership, hardlinks, ACLs, and xattrs. Root is ext4;
boot remains ext2. `e2fsck -fn`, a read-only mount via 4096-sector loop devices,
and an `rsync -aHAXnci --delete` comparison passed on Kali (excluding only the
newly created lost+found directory metadata).

Flashed only system_b and boot_b, with the existing shell boot image first.
The physical Quest then reported BLOCK_SIZE=4096 and BOTH commands succeeded:

    mount -t ext4 -o ro /dev/mapper/oculus-pmos-root /tmp/root4k
    mount -t ext4 -o ro /dev/mapper/oculus-pmos-boot /tmp/boot4k

Both returned 0. Kernel logged successful EXT4 mounts for dm-1 and dm-0. USB shell
remained alive; watchdog PID 819 was explicitly checked alive before mounting.
Full transcript: `4k-bringup/mount-4k-result.log`.

Test system image: `4k-bringup/pmos-system-4k.img`, SHA256
`520c94cf651e637e93c73f3e6337d445e9b39660d7425d3fa8d9cd4c84f41ad0`.
Kali source/output: `~/pmos/rebuild-4k.py` and `~/pmos/4k-test/`.
Kali `pmbootstrap/pmb/install/format.py` now has a local Monterey-only change
requesting -b 4096 for ext2/ext4 and disabling orphan_file/metadata checksum
features for ext4. Patch saved in `4k-bringup/pmbootstrap-monterey-4k.patch`.

A second boot replaced the UUID arguments with
`pmos_root=/dev/mapper/oculus-pmos-root`, retaining ALL original platform options,
including msm_rtb.filter and veritykeyid. The main cmdline is now 459 bytes.
This image appears to progress further: USB stays up, ping works after manually
assigning the Mac en15 address 172.16.42.2, but DHCP stops and SSH is unavailable.
Port 2323 accepts then resets, consistent with an initramfs nc process whose
executable path disappeared after switch_root. Full userspace boot NOT yet
confirmed. Do not claim Wi-Fi is fixed or tested.

Recovery caveat discovered: merely preserving the initramfs watchdog process
across switch_root is insufficient. It may retain the deleted old root and lose
access to its reboot-mode executable. The NEXT prepared test image starts a
second watchdog using chroot into /sysroot BEFORE switch_root, with a 300-second
timeout, and a chrooted debug shell on USB-only port 2324. It also logs OpenRC
startup to /var/log/oculus-openrc.log (backs up /etc/inittab first).
Prepared, not yet flashed at the time of this entry:
`4k-bringup/pmos-boot-4k-rootdebug-final.img`, SHA256
`5610dce4ec95f9e0266da167987ac1b7378dc97b674a196664b7b4ed982bd3c7`.
Source scripts are saved alongside it and in
`/Users/<user>/work/quest-pmos-bringup/`.

The currently flashed boot at this entry is
`/private/tmp/claude-501/quest-pmos/pmos-boot-4k-watchdog-final.img`, with the
original-root watchdog retention but WITHOUT the root-chroot watchdog or port
2324 additions. If it does not recover, physical reboot to USB Update Mode is
needed, then flash only the prepared rootdebug boot_b and reboot. The 4K system
image is already installed. Stock backup images were not modified.

### Safety steering at 23:25 local

Owner emphasized this is their only device and must not be bricked. Further boot
experiments stopped; priority changed to restoring stock Android. Quest still
responds to ping on 172.16.42.1, but neither ADB nor fastboot is present. Requested
physical Power-off then Volume Down + Power into USB Update Mode. DO NOT flash the
prepared experimental rootdebug image as the next action. Restore known stock
system_b + boot_b once fastboot is available, then verify Android via ADB.

Stock backup hashes freshly read (files unmodified):
- boot_b.img (67108864 bytes): 7ec81a30c8f7b597dd2678b031a136f2690db5bb3f18996b23c31544e2917c9e
- system_b.img (2684354560 bytes): cf3143347ddd0b649a4a431ad735307ad07d2e7ffda2eadab34f576e112d2e81

### Owner clarification and verified restore

Owner clarified: continue work, but protect the only device. Restored stock
system_b and boot_b successfully; Android then reported sys.boot_completed=1,
ro.build.display.id=user-49845030443200410, ro.boot.slot_suffix=_b. The restore
path is therefore freshly demonstrated. Resuming a staged test: known shell
boot + 4K system, prove chrooted real-root shell/watchdog while original
initramfs watchdog remains armed, only then test another switch_root.

### Staged real-root recovery test — NOT PASSED

Reflashed known shell boot + verified 4K system image. Confirmed original
initramfs watchdog alive, mounted 4K root read-write, copied debug-login into
/usr/bin, and started chrooted BusyBox nc on USB port 2324. Real-root shell
worked: os-release readable, OpenRC 0.63.2 executable. Transcript saved in
`/Users/<user>/work/quest-pmos-bringup/root-watchdog-test.log`.

Started from that real-root shell:
    /bin/busybox ash -c "sleep 30; /usr/sbin/reboot-mode bootloader" >/var/log/oculus-watchdog-test.log 2>&1 &
PID was 4060. It DID NOT demonstrably return to fastboot. USB re-enumerated with
a new MAC, but NCM carrier remained inactive and neither ADB nor fastboot
appeared. Owner reports Meta logo. Original initramfs watchdog was not disabled.
Root and initramfs reboot-mode binaries have identical SHA256
4e6518938cb80af9e39b1dcccccb334cb4f4cad093027e68752c6c575301cd50.

Do NOT regard the prepared rootdebug image's watchdog as validated. Full
switch_root testing paused pending understanding of reboot failure. Requested
another physical shutdown and USB Update Mode to inspect with known debug boot.
Current system_b has 4K filesystems; boot_b is known `pmos-boot-shell-final.img`.
No other partitions were flashed. /var/log/oculus-watchdog-test.log in rootfs may
hold evidence; read before restoring system_b if feasible.

### Correction: apparent disconnects were also generic initramfs error handling

A read-only chroot reboot returned fastboot immediately. A separate 4K ext4 test
mounted rw, wrote a file, synced, waited 12 seconds, read it back and unmounted;
all passed (transcript `rw-filesystem-test.log`, ending uptime 58.53s).

After the next lost NCM connection, `diskutil list external` revealed PMOS_LOGS.
Read its 32 MiB image from the observed /dev/rdisk6 and extracted its FAT32 files
with `extract-fat32-logs.py` (macOS misidentifies the small FAT32 as FAT16 and
won't mount it). `pmOS_init.txt` confirms generic subpartition/root discovery
failed due to the old truncated UUID and exported logs. `debug_shell()` then
changes gadget configuration. Thus an inactive NCM interface/new MAC does NOT
establish a reboot or kernel panic. The extracted dmesg has no panic. Also,
BusyBox dmesg does NOT support -w; stderr was redirected to pmOS_init.log.

Prepare `pmos-boot-hold-final.img` (SHA256
f970c037fdcb155355a34ff572186520325ad84347c2281cd63cd6c594d479e4) to pause in the
recovery hook AFTER mapper, watchdog, and port2323 listener setup, avoiding the
UUID timeout and USB reconfiguration. Original 300-second watchdog stays armed.
This image is for stable initramfs diagnostics, NOT a full boot.

Running kernel confirms CONFIG_DEVTMPFS=y and CONFIG_DEVTMPFS_MOUNT=y, contrary
to the original handoff. PSTORE/RAMOOPS is enabled, but mounting pstore showed
no saved entries during this check. CONFIG_PANIC_TIMEOUT=5.

### Stable debug hold and validated detached watchdog

Original 300s initramfs watchdog returned to fastboot after log-export mode.
Flashed debug-hold boot_b, keeping the same 4K system_b. At uptime 19.44,
mounted root rw and launched:
    chroot /tmp/root4k /bin/busybox setsid /bin/busybox ash -c "sleep 15; /usr/sbin/reboot-mode bootloader" >/tmp/detached-root-watchdog.log 2>&1 </dev/null &
This returned to FASTBOOT as expected. setsid is present in root BusyBox.

Then flashed a rootdebug boot with this detached real-root watchdog. Still no
port2324 or SSH; port2323 resets while ping/NCM stay alive. A startup race is
possible: asynchronous chroot commands were immediately followed by switch_root,
which can remove /sysroot before children enter it. Next prepared rootready image
requires both real-root helpers to write readiness markers and remain alive,
pauses 30s for inspection, then permits switch_root. If readiness fails it stays
in initramfs with original watchdog armed. This revision is not yet tested at
this entry. Source `prepare-root-ready.py` saved in 4k-bringup.

### Current state at 23:49 local

Full boot with rootdebug (SHA256 beb369e56280401f5072e6b6069ad3fa2d36848ef0aef75720ddc4c8b9d1704d)
remains pingable with NCM carrier, but no working shell/SSH. The 300-second
deadline passed without recovery. Requested physical USB Update Mode again.
Next prepared guarded diagnostic: pmOS boot rootready, SHA256
621190ea2880728264953fbb559719e91a6bcbd1d3b36d7cb73262c094d675fb,
requires readiness files plus live helper PIDs BEFORE switch_root and pauses
30s for inspection. If not ready, keeps initramfs recovery active. Source in
/Users/<user>/work/quest-pmos-bringup/prepare-root-ready.py.
First inspect real-root /var/log/oculus-openrc.log, oc...root-debug.log and
oculus-root-watchdog.log during that pause (or use the stable debug-hold image
and mount root read-only to inspect).

## CONFIRMED FULL BOOT + SSH (late 2026-09-29)

Guarded boot image `4k-bringup/pmos-boot-4k-rootready-final.img`, SHA256
621190ea2880728264953fbb559719e91a6bcbd1d3b36d7cb73262c094d675fb,
boots through switch_root into OpenRC default runlevel. Verified PID1=init,
SSH login as root works, dropbear and USB recovery services started. Both
real-root readiness markers and helper PIDs verified before switch_root, and
real-root shell + watchdog verified alive after switch_root.

Actual reasons SSH/recovery failed in earlier full boots:
- /etc/network/interfaces DID NOT EXIST; networking failed and blocked dropbear.
- Background watchdog redirection raced switch_root: saved log explicitly said
  `/init_2nd.sh: line 114: can't open /dev/null: no such file`.
- /var/log/oculus-openrc.log confirms even those boots reached OpenRC default.

Current on-device root fixes (preserved in `4k-bringup/boot-fixes.tar.gz`):
- /etc/network/interfaces: `auto lo` + `iface lo inet loopback`.
- root authorized_keys contains dedicated bring-up public key. Private key exists
  only at /Users/<user>/work/quest-pmos-bringup/id_ed25519 (NOT copied to vela).
- Disabled duplicate boot service via `rc-update del unudhcpd.usb0 boot`; it used
  wrong default server 172.16.41.1 and competed with oculus-usb-recovery.
  Keep oculus-usb-recovery (172.16.42.1/24).
- /usr/sbin/oculus-rmtfs now compares canonical `readlink -f` for an existing alias.
  udev creates /dev/qcom_rmtfs_uio1 -> uio0; old helper wrongly required literal
  absolute /dev/uio0. rmtfs now STARTED with unchanged `-P -r` (no PIL, read-only).
- /etc/inittab captures OpenRC startup to /var/log/oculus-openrc.log.

Wi-Fi first actual userspace test:
`echo ON > /sys/kernel/boot_wlan/boot_wlan` returned 0 and kernel logged
`wlan: driver loaded` (v5.2.03.31F). No wlan0 yet. ADSP subsystem remains OFFLINING;
/dev/subsys_adsp exists (244:4), modem remains untouched. ADSP/WLAN firmware exists.
No ADSP vote, modem vote, or writable rmtfs experiment was performed in this run.
See booted-wifi-baseline.log and wifi-first-request.log.

Remaining nonfatal startup errors: systemd-sysusers/tmpfiles helpers report
`No error information` on kernel 4.4; zram swap fails; stock-runtime/controller
fail. These do NOT prevent OpenRC default or SSH. Build host <BUILD_HOST_IP> became
unreachable during this session, so on-device changes are archived locally and
still need incorporating into the port package sources on Kali.

SSH (when this test image is booted):
ssh -i /Users/<user>/work/quest-pmos-bringup/id_ed25519 -o IdentitiesOnly=yes \
  -o UserKnownHostsFile=/Users/<user>/work/quest-pmos-bringup/known_hosts root@172.16.42.1

IMPORTANT: test image deliberately keeps a 300s watchdog and a 30s pre-switch
pause. It is a guarded bring-up image, not an unattended installation.

### Working checkpoint saved and final device state

- Full-boot real-root 300-second watchdog automatically returned to fastboot.
- Booted stable debug-hold image, mounted root read-only to settle journal, then
  unmounted it. `e2fsck -fn /dev/mapper/oculus-pmos-root` completed all five passes,
  exit 0: 3717/36240 files, 39310/72448 blocks.
- Captured first 809500672 bytes of system_b with neither inner filesystem mounted.
- Saved `4k-bringup/pmos-system-4k-ssh.img` (mode 0600; contains device SSH host keys),
  SHA256 ed8cc6d45b11eafb188fea1ec31df9f57077064812898aea799395641c464199.
- Verified GPT/leading MiB matches source, boot/root block sizes are both 4096,
  and neither filesystem superblock has the journal-recovery-required flag set.
- Returned to fastboot and installed verified `pmos-boot-4k-rootready-final.img`;
  did NOT reboot again. Current slot B, device visible via fastboot.
- Stock backups remain unchanged. No bootloader/firmware/NV partitions were flashed.

Next boot of B runs the guarded SSH-capable pmOS installation. The watchdog will
return it to fastboot after roughly five minutes; keep this safeguard during
further experiments. For stock Android, use the demonstrated boot_b/system_b
restore pair in the original backup directory.

## Consolidation into port sources — 2026-09-30

The hand-won boot fixes are now folded into the port package sources and **validated by
a clean fresh `pmbootstrap install`** (zapped rootfs = real user path).

**`device-oculus-monterey` bumped to r34.** Four fixes, all verified on the fresh rootfs:
| Fix | How | Verified |
|---|---|---|
| ext4 **block size 4096** (boot+root) | pmbootstrap `format.py` local patch (see below) | dumpe2fs on exported img: 4096/4096 |
| `oculus-rmtfs` `readlink -f` (3 spots) | replaced source file with proven on-device version | grep count 3 |
| `/etc/network/interfaces` loopback | new `interfaces` source file + `package()` install | `auto lo` present |
| remove competing `unudhcpd.usb0` | new `.post-install` **and** `.post-upgrade` (`rc-update del` + `rm`) | 0 in boot runlevel |

Note on unudhcpd.usb0: it is enabled by `postmarketos-base-openrc`'s post-install; the
device pkg (which depends on postmarketos-base) removes it afterward. BOTH `.post-install`
and `.post-upgrade` are needed (apk runs the former on fresh install, the latter on upgrade).

**Reproducibility caveat (IMPORTANT):** the block-size fix lives as a **pmbootstrap-local
patch**, NOT in pmaports — `4k-bringup/pmbootstrap-monterey-4k.patch` (conditions on
`device == "oculus-monterey"`, forces `mkfs.ext4 -b 4096` and drops the too-new ext4
features). A clean rebuild on another machine must apply this to pmbootstrap first, then
build the port. A cleaner long-term home would be a deviceinfo option or upstream mkfs flag.

**Artifacts (vela `pmos/4k-bringup/`):**
- `pmaports-monterey-packages.tar.gz` — the actual port sources (git doesn't track the
  grafted packages), incl. device-oculus-monterey r34 with all fixes.
- `pmbootstrap-monterey-4k.patch` — the block-size patch for pmbootstrap.
- `pmos-system-4k-reproducible.img` (809500672 bytes, sha256
  `68296c00645d1393351187cc0f48daab49b72e40ea39a49351a79f7ae90c1ca3`) — clean-build rootfs,
  4096 blocks, all four fixes. (Fresh rootfs: pmbootstrap adds the user's SSH key at
  install; it does NOT carry the earlier bring-up key or host keys — use the proven
  `pmos-system-4k-ssh.img` if you need the exact SSH-ready image that booted.)

### REMAINING consolidation item — boot-side (not yet in sources)
The **rootfs** side is reproducible. The **boot image** side is NOT fully consolidated:
- The clean-built `deviceinfo_kernel_cmdline` still ends at `pmos_force_initramfs`, so
  pmbootstrap still appends the full `pmos_root_uuid` → the 512-byte truncation (blocker A)
  is still present in a clean-built boot.img.
- The proven-booting guarded image sidesteps this by mounting root via
  `pmos_root=/dev/mapper/oculus-pmos-root` (device name, not UUID) — but that lives only
  in the hand-prepared `pmos-boot-4k-rootready-final.img`, NOT in the port initramfs sources.
- So today: a clean build produces a correct **rootfs**, but booting still relies on the
  hand-prepared guarded boot image. To fully close this, teach the port initramfs to mount
  root by the mapper device name (the "cleaner fix" in §5B), OR emit `pmos_root=<device>`
  from the build instead of `pmos_root_uuid`. This is entangled with pmbootstrap/initramfs
  internals and with the diagnostic scaffolding (30s pause, port 2323/2324, extra watchdogs)
  that must be STRIPPED for a clean image — a good task to pair with the next boot session.


## Reproducibility review — recovered boot tools and host pins (2026-10-01)

`tools/prepare-monterey-boot` is recovered from the original workspace. It validates
Android header v0/page4096 and an unsigned source ending at aligned payload end;
restores the owner's template second-stage load address; appends its4096-byte DER
BootSignature page with the `/boot` length updated. It does not produce a valid
cryptographic signature and requires the already-unlocked bootloader behavior.
No device is accessed by this tool.

Offline reproduction using saved `pmos-boot-4k-rootready-raw.img` and owner stock
`boot_b.img` produced exactly the saved root-ready final image:
SHA256 `621190ea2880728264953fbb559719e91a6bcbd1d3b36d7cb73262c094d675fb`,
26161152 bytes; payload26157056, signature4096, second address0x00f00000.
This verifies finalization of that ramdisk, not clean generation of the ramdisk.

`python3 tools/prepare-4k-boot.py INPUT STOCK_TEMPLATE NEW_OUTPUT` now removes
`pmos_boot_uuid=`, `pmos_root_uuid=` and existing `pmos_root=` tokens, retains other
arguments, appends `pmos_root=/dev/mapper/oculus-pmos-root`, requires
`pmos_force_initramfs`, enforces <512bytes and clears the extended1024-byte field.
It does not edit ramdisk contents or disable guards. With saved shell-raw input it
produced484bytes of cmdline and SHA256
`031075f7f344ff3022eba3b61b33f01b5f8ec5aa8af39409037d1b63588f9646`.
That differs from the historical path image because the old script blindly chopped
all tokens after `pmos_boot_uuid`; the recovered wrapper preserves
`pmos_rootfsopts=defaults`. This new cmdline wrapper is offline-tested, not boot-tested.

Observed build-host source pins (read from Kali, not inferred from current upstream):

| Source | Commit |
|---|---|
| https://gitlab.postmarketos.org/postmarketOS/pmbootstrap.git | `fde5aadb8269898a17f3736ef63eba37df1bbaff` |
| https://gitlab.postmarketos.org/postmarketOS/pmaports.git | `2254bfcf53d13d79136e9c5187480a3582f32cad` |
| https://github.com/Block-Flock/pmaports-oculus-monterey.git | `7ce0ccce4d4879df9bb27d29a4ee59af7b2c639a` |

Observed pmbootstrap_v3.cfg: device=`oculus-monterey`, ui=`none`,
aports=`/home/<user>/pmos/pmaports`, work=`/home/<user>/pmos/work`.
Workdir records edge for native, rootfs_oculus-monterey and buildroot_aarch64.
The pmbootstrap checkout has the local `pmb/install/format.py` patch. Pins identify
bases; local grafted packages and the tracked4Kn patch are additional inputs.

Fresh-host configuration recipe: check out those exact base revisions, graft the
tracked device/testing packages per tutorial02, apply the tracked4Kn patch with
`git apply --check` first, then run `./pmbootstrap.py init` and select that pmaports
checkout, device `oculus-monterey`, UI `none`, a private work directory and your own
username. Verify with `./pmbootstrap.py config device`, `config ui`, `config aports`
and `config work` before build/install/export. This assembled fresh-host procedure
has not been executed from an empty cache. If patch context fails, stop and inspect
`format_partition_with_filesystem`; do not silently skip the4096-byte requirement.

### Clean unattended image remains an integration task

The historical root-ready transformation is preserved in `tools/prepare-root-ready.py`.
It expects a previously modified ramdisk and is not a clean upstream-initramfs builder.
It injects a30s readiness pause, USB TCP2324 debug listener, logging changes and
300s root watchdog while retaining the initramfs recovery process through switch_root.
Simply removing these breaks `oculus-wifi`'s explicit original-timer preflight and
recovery-guard takeover. TCP2323 comes from earlier initramfs debug scaffolding.
A clean conversion must replace those dependencies with a verified guard handoff,
remove both listeners and debug login installation, restore intended inittab logging,
and check cold boot/recovery/SSH without removing the independent hardware watchdog.
No tested clean conversion is available. Do not present deleting the pause/listeners
as a reproduced unattended install. This remaining blocker is tracked in
[reproducibility gaps](reproducibility-gaps.md).
