# Oculus Quest 1 → Nura (postmarketOS)

Bring-up notes, patches, and diagnostic tooling for running **Nura** (the Linux
distribution formerly known as **postmarketOS**) on the **Oculus Quest 1**
(codename `monterey`, Snapdragon 835 / MSM8998).

This is a working companion to the [Block-Flock `pmaports-oculus-monterey`][upstream]
port — the port's device packages plus the fixes, patches, and diagnostic renderers
developed while getting the device from "unlocked bootloader" to "boots to a shell,
draws an upright desktop, tracks your head, and is building an OpenXR runtime."

> ⚠️ **Work in progress, and these are working notes.** The docs under `docs/` are
> real bring-up logs written for continuity between sessions — they still contain
> local paths, a LAN IP, and the device serial. Nothing secret (no keys), but a
> sanitization pass is reasonable future work before wide publication.

## Product requirement

The owner wants a **VR-only headset**, with no desktop mode. Xorg/labwc was a temporary bring-up diagnostic, not the intended shell. Disable its boot service with `tools/disable-desktop-autostart.sh`; the direct VR launcher never restarts a desktop on exit. A native VR shell/compositor is still to be implemented.

## Status (2026-09-30)

**Working:**
- Boots Nura/pmOS fully → OpenRC + **SSH** over USB (NCM, `172.16.42.1`)
- **Display**: real framebuffer 2880×1600, Xorg fbdev, upright nested desktop
- **Controllers**: both tracked as pointers, button input observed
- **Head tracking (orientation)**: IMU gyro+accel fusion via a patched Monado driver;
  wearer-confirmed directions/tilt; 22 tests pass
- **Lens optics**: stock distortion mesh reverse-engineered; wearer-confirmed in a
  standalone X11 diagnostic renderer
- **Monado** (OpenXR runtime) building and installed
- **Wi-Fi**: 2.4/5 GHz scans, WPA2 association, DHCP, router/internet ping and DNS
  verified. Uses relative-offset RMTFS with read-only NV backing, native mapper
  and TFTP, and the owner's stock CNSS helper in a RAM-only writable environment.
  Packaged automatic startup and SSH over Wi-Fi are verified across reboot.
  The five-minute recovery timer remains enabled during bring-up; see
  [`docs/adsp-wifi.md`](docs/adsp-wifi.md).

**Not yet working / WIP:**
- GPU acceleration (software Lavapipe for now), audio, positional tracking, passthrough
- Optics integration into the Monado runtime (correction currently lives only in the
  standalone diagnostic renderer)
- Performance — X11 diagnostic ~19.5 fps; direct framebuffer backend in validation. Corrected threaded submission path measures59.7fps renderer/59.8fps MDSS driver; wearer reports definitely better, but motion is still not fully smooth. See `renderer/fast/README.md`.
- A fully clean-build **boot image** (the rootfs build is reproducible; the boot image
  still needs manual device-name root mounting — see the boot doc)

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
| **Basalt `libbasalt.so`** | Build the [mateosss/basalt](https://gitlab.freedesktop.org/mateosss/basalt) fork (implements Monado's VIT ABI); apply `patches/basalt/`. Build **float-only** (`use-double=false`). |
| **`replay-vit`** (offline VIO runner) | Compile `tools/tracking/replay-vit.cpp` against Basalt's `vit_interface.h`, link `libbasalt.so`. |
| **Monado runtime** | Build the monado-oculus-monterey port with `patches/monado/`. |
| **mesa-kgsl** | `packages/mesa-kgsl-monterey/` (pmbootstrap build). *Note:* GPU accel is a dead end on Adreno 540 — see `docs/KGSL-ACCELERATION.md`. |

**Cannot be redistributed (obtain from your own device):**
- **Meta/Oculus proprietary blobs** — WLAN/ADSP firmware, camera + IMU factory calibration
  (`camera_calibration_v2.json`, `imu_calibration.json`), and the stock distortion mesh.
  These are extracted from *your own headset's* stock system image via the on-device
  calibration store; they are device-specific and Meta's IP.
- **Stock partition backups** (`boot_b.img`, `system_b.img`, NV, etc.) — your own device
  dumps, kept in local backups. Required for recovery; device-specific.

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
