# Quest 1 → Nura: bring-up tutorial

Unlocked Quest 1 (`monterey`, SD835) → booting Nura/postmarketOS with a shell, Wi-Fi,
display, and head tracking. Paged, in order. Each page: the concept, the commands, how
you know it worked, and what bit us.

From-source port, not a flasher app — no one-click image. You'll build things. Where a
step needs a blob only your headset has, the page says so.

## Stages

| # | Page | What you get |
|---|------|--------------|
| 0 | [prerequisites](00-prerequisites.md) | host + tools + unlock check |
| 1 | [back up stock](01-back-up-stock.md) | a way back |
| 2 | [build the rootfs](02-build-rootfs.md) | bootable Nura rootfs (the 4Kn fix) |
| 3 | [prepare boot + flash](03-prepare-boot-and-flash.md) | signed boot image → slot B |
| 4 | [first boot + SSH](04-first-boot-and-ssh.md) | **root shell over USB** |
| 5 | [Wi-Fi](05-wifi.md) | `wlan0`, on the network at boot |
| 6 | [display / tracking / optics](06-headset-extensions.md) | draw + look around (3DoF) |
| 7 | [positional tracking](07-positional-tracking.md) | 6DoF via Basalt (WIP) |
| 8 | [gotchas + dead ends](08-safety-and-dead-ends.md) | the walls, so you skip them |

Stop after **stage 4** and you've got the headline: a Meta-free Linux Quest 1 with a
shell. 5–7 are extensions, roughly in decreasing order of "done."

## Two things that aren't turnkey yet

- **The boot image needs one hand step (stage 3).** The rootfs rebuilds clean from
  source; the boot image still mounts root by device name (to dodge a 512-byte cmdline
  truncation) and has to be re-signed against *your own* stock `boot_b`. That's the gap
  between "research port" and "one `pmbootstrap install`."
- **Some blobs are yours only.** WLAN/camera/IMU firmware + factory calibration are
  device-specific Meta IP — pulled from *your* stock image (stages 1, 5), not shipped
  here. We document the formats, not the blobs.

The `docs/*.md` one level up are the raw bring-up logs each page points back to — deeper
and messier.
