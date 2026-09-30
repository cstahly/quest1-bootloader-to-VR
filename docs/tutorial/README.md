# Quest 1 → Nura: the reproducible tutorial

Take an **Oculus Quest 1** (`monterey`, Snapdragon 835) from *"the bootloader is
unlocked"* to *"it boots Nura/postmarketOS, has a shell over SSH, joins Wi-Fi, draws on
its display, and tracks your head."* Each stage below is a self-contained **page** with
the exact steps, a **Definition of done** you can check before moving on, and a
**When it fails** section. Work them in order — each page assumes the one before it
passed its done-check.

This is a **from-source research port**, not a flasher app. There is no one-click image.
A capable person (comfortable with `fastboot`, cross-compiling, and a Linux build host)
can reproduce it; budget a few hours and expect to build things. Where a step needs a
blob only your own headset has, the page says so.

## The stages

| # | Page | What you get | Reproducible? |
|---|------|--------------|---------------|
| 0 | [`00-prerequisites.md`](00-prerequisites.md) | Host, hardware, accounts, safety rules | — |
| 1 | [`01-back-up-stock.md`](01-back-up-stock.md) | A full recovery net before you touch anything | ✅ turnkey |
| 2 | [`02-build-rootfs.md`](02-build-rootfs.md) | A bootable Nura rootfs (the 4Kn fix) | ✅ turnkey |
| 3 | [`03-prepare-boot-and-flash.md`](03-prepare-boot-and-flash.md) | A signed boot image, flashed to slot B | ⚠️ one manual step |
| 4 | [`04-first-boot-and-ssh.md`](04-first-boot-and-ssh.md) | Boots to OpenRC, root shell over USB | ✅ turnkey |
| 5 | [`05-wifi.md`](05-wifi.md) | `wlan0`, associated, on the network at boot | ⚠️ needs your blobs |
| 6 | [`06-headset-extensions.md`](06-headset-extensions.md) | Display, head tracking, lens optics | 🔬 diagnostic-grade |
| 7 | [`07-positional-tracking.md`](07-positional-tracking.md) | 6DoF body movement (Basalt VIO) | 🚧 in progress |
| 8 | [`08-safety-and-dead-ends.md`](08-safety-and-dead-ends.md) | The walls, and rules that keep the device alive | — read first |

**Stop after stage 4 and you have the headline result: a Meta-free Linux Quest 1 with a
shell.** Stages 5–7 are extensions in decreasing order of "done."

## The two hard truths about reproducing this

1. **The boot image is the one gap.** The *rootfs* rebuilds cleanly from source (stage
   2). The *boot image* still needs a hand step: it mounts root by device name to dodge
   a bootloader cmdline-truncation bug, and it must be re-signed against **your own
   device's** stock `boot_b`. Stage 3 walks this; it's the reason the port isn't fully
   turnkey yet.
2. **Some blobs are yours alone.** WLAN/camera/IMU firmware and factory calibration are
   Meta's IP and device-specific. They are **not** in this repo — you extract them from
   *your* headset's stock image (stages 1 and 5). We document the *formats*, never ship
   the blobs.

## Using this with an AI agent ("point your robot at it")

Each page is written so a coding agent can execute it and self-verify. The contract:

- **Advance only when the page's "Definition of done" check passes.** It's a real
  command with an expected output — the agent runs it and gates on it, it does not
  assume success.
- **Physical actions are the human's.** When a page says *ask the wearer to do X, then
  wait*, the agent must **print the instruction, stop, and wait for the human to confirm
  before continuing.** Never proceed through a "put the headset on" / "walk forward"
  step on your own. (The owner's standing rule: *"If you want me to do something
  physical you need to tell me to do it, then pause."*)
- **The cord is unplugged when the wearer walks.** Anything during a physical walk must
  run over **Wi-Fi**, backgrounded, confirmed-live *before* the walk — never over a USB
  SSH session that will drop.

### Rules the agent must never break (see stage 8 for why)
- **Never disable the recovery watchdog** (never `oculus.recovery_timeout=0`). It is the
  only thing that auto-recovers a bad boot on this single, irreplaceable device.
- **Never flash bootloader, modem, NV, or factory-calibration partitions.** Only
  `system_b` and `boot_b` are in play, and only after stage 1's backups exist.
- **Never commit secrets or blobs** — the bring-up SSH private key, Wi-Fi PSK, or any
  Meta firmware/calibration. They live outside git by design.
- **Keep the stock backups pristine.** They are the restore path; treat them read-only.

## If you just want to understand what happened, not reproduce it
The detail docs one level up (`../boot-bringup.md`, `../adsp-wifi.md`,
`../tracking-optics-performance.md`, `../basalt-positional-tracking.md`,
`../xbl-secureboot-notes.md`) are the raw bring-up logs — deeper, messier, and the
authoritative source each tutorial page points back to.
