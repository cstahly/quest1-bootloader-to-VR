# Oculus Quest 1 → Nura (postmarketOS)

Bring-up notes, patches, and diagnostic tooling for running **Nura** (the Linux
distribution formerly known as **postmarketOS**) on the **Oculus Quest 1**
(codename `monterey`, Snapdragon 835 / MSM8998).

This is a working companion to the [Block-Flock `pmaports-oculus-monterey`][upstream]
port — the port's device packages plus the fixes, patches, and diagnostic renderers
developed while getting the device from "unlocked bootloader" to "boots to a shell,
draws an upright desktop, tracks your head, and is building an OpenXR runtime."

> 📖 **Start here: [`docs/tutorial/`](docs/tutorial/README.md)** — paged bring-up from
> unlocked bootloader to a booting, SSH-able, Wi-Fi Nura headset. Each page: the concept,
> the commands, how you know it worked, what bit us. The `docs/*.md` files below are the
> deeper raw bring-up logs it points back to.

> ⚠️ **Work in progress, and these are working notes.** The docs under `docs/` are
> real bring-up logs written for continuity between sessions. Personal details are
> replaced with placeholders such as `<SERIAL>`, `<BUILD_HOST_IP>` and `/home/<user>`;
> see [Contributing](#contributing-keep-personal-data-out).

## Product requirement

The owner wants a **VR-only headset**, with no desktop mode. Xorg/labwc was a temporary bring-up diagnostic, not the intended shell. Disable its boot service with `tools/disable-desktop-autostart.sh`; the direct VR launcher never restarts a desktop on exit. A native VR shell/compositor is still to be implemented.

## Status (2026-09-30)

Boot/OpenRC, USB and Wi-Fi SSH, native framebuffer VR scene, lens correction,
orientation tracking, trigger-renewable recovery/HUD, four monochrome camera feeds
and head-aimed saved 3D drawing have been demonstrated. No desktop is intended.

Physical positional tracking is still offline research, composite passthrough needs
correction, and there is no finished native OpenXR compositor/home. Hardware GPU
rendering, audio and production boot/recovery behavior remain unfinished.

See the maintained [status, in-progress work and TODO](docs/tutorial/09-status-and-next.md)
and [camera/playground chapter](docs/tutorial/10-cameras-and-playground.md). Older
chronological notes contain superseded experiments and performance measurements.

## The finding that unblocked booting

The Quest's UFS is **4Kn** (`logical_block_size = 4096`), but `mkfs.ext4` defaulted the
small rootfs to **1024-byte blocks**. A filesystem block size below the device's logical
sector size is unmountable — the old 4.4 kernel **oopsed** on the mount instead of failing
cleanly, which looked for a while like a mysterious kernel bug. The fix is one flag:
`mkfs.ext4 -b 4096`. Full write-up in [`docs/boot-bringup.md`](docs/boot-bringup.md).

## Layout

| Path | What |
|---|---|
| `docs/` | The bring-up handoffs: boot, ADSP/Wi-Fi, tracking/optics/performance |
| `packages/` | The pmaports port packages (`device-oculus-monterey` with the 4Kn/rmtfs/networking fixes, and `oculus-monterey-initramfs-support` with the subpartition mapper) |
| `patches/` | `pmbootstrap-monterey-4k.patch` (block-size fix) and the Monado head-frame patch + APKBUILD |
| `renderer/` | Standalone X11 diagnostic renderer for lens/optics/tracking (baseline + `fast/` optimized) |
| `tools/` | Bring-up scripts: 4K image rebuild, boot-image prep, IMU probing, capture helpers |

## Deliberately NOT in this repo

- **Secrets** — the bring-up SSH private key lives only on the dev machine, never here.
- **Binaries** — kernel/rootfs/boot images, APKs, compiled executables, framebuffer
  dumps. The rootfs is reproducible from `packages/` + `patches/`; images are kept as
  evidence in local backups, not git.
- **Meta/Oculus proprietary blobs** — the stock distortion-mesh and firmware are Meta's
  IP and are not redistributed. The reverse-engineered mesh *format* is documented in
  `docs/tracking-optics-performance.md` (interoperability research); the blob itself is not.
- **Upstream GPL kernel drivers** — reference copies of `icnss.c`, `subsys-pil-tz.c`,
  etc. that were studied for the Wi-Fi work belong in the kernel tree, not here.

(All of the above are enforced by `.gitignore` as defense-in-depth.)

## Reproducing a booting rootfs

1. Apply `patches/pmbootstrap-monterey-4k.patch` to pmbootstrap (the 4Kn block-size fix
   lives in pmbootstrap's `format.py`, not in pmaports).
2. Graft `packages/*` into a pmaports checkout under `device/testing/`.
3. `pmbootstrap install` → the rootfs comes out with 4096-byte block filesystems and the
   networking/rmtfs fixes. (The boot image still needs the device-name root-mount handling
   described in the boot doc.)

## Binaries — where each one comes from

None of these are committed (they're gitignored). Here's how to obtain or rebuild each:

| Binary | How to get it |
|---|---|
| **pmOS rootfs + boot images** | Rebuild via pmbootstrap — see *Reproducing a booting rootfs* above and `docs/boot-bringup.md`. The exact proven-booting *guarded* images live only in local backups, not git. |
| **`capture-imu`** (headset IMU reader) | Cross-compile `tools/tracking/capture-imu.c` for aarch64 (the headset has no compiler): `pmbootstrap chroot -r -- cc -O2 -static ...`. See `docs/basalt-positional-tracking.md`. |
| **Basalt `libbasalt.so`** | Build the [mateosss/basalt](https://gitlab.freedesktop.org/mateosss/basalt) fork (implements Monado's VIT ABI); apply accepted patches 0001/0003, not experimental 0002. Build **float-only** (`use-double=false`). |
| **`replay-vit`** (offline VIO runner) | Compile `tools/tracking/replay-vit.cpp` against Basalt's `vit_interface.h`, link `libbasalt.so`. |
| **Monado runtime** | Build the monado-oculus-monterey port with `patches/monado/`. |
| **mesa-kgsl** | `packages/mesa-kgsl-monterey/` (pmbootstrap build). *Note:* hardware rendering remains unverified on Adreno 540 — see `docs/KGSL-ACCELERATION.md`. |

**Cannot be redistributed (obtain from your own device):**
- **Meta/Oculus proprietary blobs** — WLAN/ADSP firmware, camera + IMU factory calibration
  (`camera_calibration_v2.json`, `imu_calibration.json`), and the stock distortion mesh.
  Stock code/firmware and private per-unit calibration come from different owner
  partitions; see the tutorial's backup and camera chapters.
- **Stock partition backups** (`boot_b.img`, `system_b.img`, NV, etc.) — your own device
  dumps, kept in local backups. Required for recovery; device-specific.

## Contributing: keep personal data out

Docs and tools use placeholders instead of anyone's real details: `<SERIAL>`,
`<BUILD_HOST_IP>`, `<HEADSET_WIFI_IP>`, `<LAN_GATEWAY_IP>`, `<LAN_SUBNET>`, and
`<user>` in `/Users/<user>`, `/home/<user>`. Once per clone:

```
cp local.env.example local.env          # your real values; gitignored
git config core.hooksPath .githooks     # pre-commit runs tools/check-pii.sh
```

The hook blocks commits that add private LAN IPs, home-directory paths, personal email
addresses or private keys, plus any exact strings you list one per line in
`.pii-patterns` (gitignored) — your serial, your name. CI runs the same check on every
push and pull request, with the repository secret `PII_PATTERNS` supplying the exact
strings. Notes that need real values (hosts, paths, credentials) belong in `local.env`
or outside the repo.

## Credits

- **Block-Flock `pmaports-oculus-monterey`** — the upstream port this builds on
  (confirm the canonical repo URL and link it here)
- [Nura / postmarketOS](https://nura.eco/) — the distribution
- QuestStack — the Quest 1 bootloader-unlock chain that makes any of this possible

## License

**TBD.** The pmaports port packages are MIT; the Monado patch applies to Monado
(Boost Software License 1.0). The renderer and tooling here are the author's own — pick a
license before publishing (MIT is the natural match for the pmaports side). Until then,
default copyright applies.

<!-- TODO: confirm and fill in the real upstream URL for the Block-Flock port -->
[upstream]: #credits
