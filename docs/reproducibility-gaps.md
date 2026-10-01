# Reproducibility gaps — collection-agent review

Updated 2026-10-01 after source, retained-image and build-host audit. This is the
collection agent's index to what can actually be reconstructed. **A fresh unlocked
Quest-to-playground install is not yet end-to-end verified.** Recovered instructions
and offline tests close specific holes; they do not establish a clean hardware run.
The headset remained parked in fastboot throughout this documentation review.

## Resolution map

| Area | Included evidence / procedure | Remaining boundary |
|---|---|---|
| Rootfs | [Boot log: source pins and observed config](boot-bringup.md#reproducibility-review--recovered-boot-tools-and-host-pins-2026-10-01), tutorial02, tracked4Kn patch | New empty-cache build not run; package mirrors are not frozen |
| Boot footer | `tools/prepare-monterey-boot`; saved raw input reproduces final root-ready image byte-for-byte | This proves footer assembly, not source-to-ramdisk reproduction |
| Short cmdline | Path-parameterized `tools/prepare-4k-boot.py`; offline regression test | New wrapper preserves extra arguments and has not booted on hardware |
| Guarded ramdisk | Historical `tools/prepare-root-ready.py` plus boot log and retained unsigned image | Script still depends on an earlier modified ramdisk and historical paths |
| Owner Wi-Fi assets | [Per-file source/checklist and extraction recipe](adsp-wifi.md#owner-firmware-and-android-runtime-extraction-recipe-2026-10-01) | Requires readable owner stock images, intact runtime and compatible firmware revision |
| Camera/IMU/optics | [Read-only extraction paths, commands, sizes and owner hashes](CAMERA-ROADMAP.md#reproducing-owner-calibration-and-optics-extraction-2026-10-01) | Unit calibration hashes are not universal acceptance values |
| Service assembly | [Ordered runtime install and startup](../runtime/README.md) | Assembled procedure not clean-installed; original timer adoption still required |
| Native Basalt | [Source/package pins, bundle hashes, build flags and metric definitions](basalt-positional-tracking.md#native-runtime-reproducibility-review-2026-10-01), tracked build/bundle tools | Complete transitive source lock and clean baseline configure/build remain open |
| Current research | [Checkpoint](tutorial/09-status-and-next.md) and tracking/camera logs | CPU invalidation is built, never run on device; r8 and prediction are not accepted defaults |

## Hard stops still open

### 1. Source-to-guarded-ramdisk and clean unattended boot

The missing footer helper is recovered and runnable. Test:

```sh
python3 tools/test-prepare-boot.py
python3 tools/prepare-monterey-boot OWNER_UNSIGNED_RAW OWNER_STOCK_BOOT NEW_OUTPUT
```

Saved root-ready unsigned input is now retained under vela backups with a hash in
[assets inventory](assets-inventory.md), rather than depending on `/private/tmp`.
It reproduces the known final image SHA256
`621190ea2880728264953fbb559719e91a6bcbd1d3b36d7cb73262c094d675fb`.
A fresh exported upstream ramdisk is **not equivalent** to that saved guarded input.
The historical ramdisk scripts must be consolidated into a parameterized transformation
from pinned upstream initramfs, with tests for each hook and switch_root behavior.

A clean unattended image cannot be made by deleting the30s pause, TCP2323/2324 and
`sleep300`: Wi-Fi preflight checks those original timer processes, and the renewable
guard adopts them. Refactor and verify that handoff before removing debug scaffolding.
Preserve the independent hardware watchdog. No tested clean-image removal recipe
exists yet; the report must retain this as a blocker.

### 2. Baseline packages and native dependency lock

The current Monado APKBUILD is experimental r8. The installed orientation-only
baseline was r3. The retained r3 APK was identified from PKGINFO and hashed; its source recipe is
recoverable at `b89f021` as documented in the tracking log. Rebuild and verify it
before promising a byte-identical baseline from fresh dependencies. Source flags and versions
of the19MB headless Basalt bundle are now recorded, and library hashes recomputed,
but a complete transitive lock/build recipe and second clean build remain incomplete.
The Bionic camera build still depends on matching owner libc/libdl and Linux LLVM;
the command is documented but a pinned standalone toolchain recipe is not established.

### 3. End-to-end acceptance

Run the ordered install on an independent checkout/build root only after the two
items above are resolved. Record input hashes, package manifests, generated image
hashes, boot/SSH, Wi-Fi association, gravity initialization, visible scene, four fresh
camera streams, trigger renewal, guard expiry/recovery and cold boot. The existing
owner's historical successes validate components, not this newly assembled process.
Physical tracking, low-light robustness, passthrough geometry and photon-to-display
latency are separate unfinished acceptance criteria.

## Corrections the report must preserve

- The author-location inventory is distinct from the new stock-source extraction
  recipes. Neither contains blobs, keys or PSKs. Unit-specific hashes identify the
  owner's evidence; another headset should usually differ for calibration.
- Factory camera/IMU calibration came from GPT `private` mounted as Android `/persist`,
  **not** the GPT partition named `persist`.
- The mesh is `/system/etc/calibration/distortion-mesh.bin`; HMD config is
  `/system/etc/xrs-hmdconfig.capnp.bin`. WLAN INI bytes are at
  `/system/vendor/etc/wifi/WCNSS_qcom_cfg.ini`, not its absolute firmware symlink.
- Native301/301 at43/69ms is a live15Hz run;40/66ms is recorded15Hz replay;
  601/601 is a30Hz four-camera run. Different workloads and timing origins.
- Historical exposure request/readback mismatches are not proof of sensor-command
  failure: stale CPU metadata was found. The verified ION invalidate ABI supports a
  pending experiment, not a demonstrated fix.
- The ARM toolchain file was already tracked; this audit verified it against Kali.
  It was not an outstanding missing-file blocker.

## Availability to the collection agent

All source/recipe/evidence summaries above are tracked in this repository. Deep
commands and exact values remain in matching bring-up logs; tutorial pages link to
them. Private binary assets remain on the exact hosts/paths in `assets-inventory.md`.
The collection agent can use the tracked hashes without copying private contents.
Local access to those hosts is still necessary to rerun binary tests; a clone alone
cannot supply private calibration or proprietary firmware. Before publication,
review the owner-location inventory separately: it contains identifying paths and
network details even though it contains no credential values.
