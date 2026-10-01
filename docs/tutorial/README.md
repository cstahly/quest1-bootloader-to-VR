# Quest 1 → Nura: bring-up tutorial

Unlocked Quest 1 (`monterey`, SD835) → booting Nura/postmarketOS with a shell, Wi-Fi,
display, and head tracking. Paged, in order. Each page: the concept, the commands, how
you know it worked, and what bit us.

From-source port, not a flasher app — no one-click image. You'll build things. Where a
step needs a blob only your headset has, the page says so.

## Read this checkpoint first

[Current status, work in progress and TODO next](09-status-and-next.md) is the
maintained summary. This is an evidence-backed research guide, **not yet a validated
copy/paste installation on a fresh owner's device**. Historical scripts contain
private paths and the boot finalizer is missing from this repository. Each unresolved
step must be completed before flashing, not guessed around.

## Stages

| # | Page | What you get |
|---|------|--------------|
| 0 | [prerequisites](00-prerequisites.md) | host + tools + unlock check |
| 1 | [back up stock](01-back-up-stock.md) | a way back |
| 2 | [build the rootfs](02-build-rootfs.md) | bootable Nura rootfs (the 4Kn fix) |
| 3 | [prepare boot + flash](03-prepare-boot-and-flash.md) | prepared boot image → slot B |
| 4 | [first boot + SSH](04-first-boot-and-ssh.md) | **root shell over USB** |
| 5 | [Wi-Fi](05-wifi.md) | `wlan0`, on the network at boot |
| 6 | [display / tracking / optics](06-headset-extensions.md) | draw + look around (3DoF) |
| 7 | [positional tracking](07-positional-tracking.md) | 6DoF via Basalt (WIP) |
| 8 | [gotchas + dead ends](08-safety-and-dead-ends.md) | the walls, so you skip them |
| 9 | [current work / TODO](09-status-and-next.md) | verified state and ordered next steps |
| 10 | [cameras / drawing playground](10-cameras-and-playground.md) | four live cameras, saved 3D ink, passthrough limits |

Stop after **stage 4** and you've got the headline: Linux booting independently of the Android UI, with a shell. Proprietary stock
firmware/runtime components remain in use. Continue with the VR and camera chapters;
Basalt is unfinished research.

## Important reproduction gaps

- **The boot image needs one hand step (stage 3).** The rootfs rebuilds clean from
  source; the boot image still mounts root by device name (to dodge a 512-byte cmdline
  truncation) and needs legacy footer preparation using *your own* stock `boot_b`. The helper is now tracked and offline-verified; clean guarded-ramdisk integration
  and runtime/asset packaging still remain.
- **Some blobs are yours only.** WLAN/camera/IMU firmware + factory calibration are
  private inputs from your own stock runtime and calibration partitions (stages 1, 5), not shipped here. We document the formats, not the blobs.

The `docs/*.md` one level up are the raw bring-up logs each page points back to — deeper
and messier.
