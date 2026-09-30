# Stage 0 — Prerequisites

**Goal:** have every tool, account, and safety net in place before you touch the device.

## Concept

You're going to replace the Quest's Android with a Linux (Nura/postmarketOS) rootfs
built from source, flash it to the unused `system_b`/`boot_b` slot, and boot it. Nothing
here works unless the bootloader is *already unlocked* and you have a way back to stock.

## The precondition: an unlocked bootloader

This tutorial **starts** at an unlocked bootloader. Unlocking is a separate exploit
chain, **QuestStack** (GhostLock/ionstack temporary root + the CVE-2021-1931 ABL
overflow → `fastboot oem unlock`). It is not part of this repo. Confirm you're unlocked:

```
adb reboot bootloader          # or: power off, then Vol-Down+Power → USB update mode
fastboot getvar all 2>&1 | grep -Ei 'unlock|flash.locked|verifiedbootstate'
# want: unlocked:yes  (equivalently flash.locked=0, verifiedbootstate=orange)
```

If that shows locked, stop — do QuestStack first.

## Hardware

- The Quest 1 (`monterey`, Snapdragon 835 / MSM8998, WCN3990 Wi-Fi).
- A good USB-C data cable.
- The controllers (for later stages).

## Build host

A Linux machine (this port was built on Kali) with:
- **pmbootstrap 3.11.x** (`pip install pmbootstrap` or from git; this port used `fde5aad`).
- An **aarch64 cross toolchain** — pmbootstrap's chroot provides one; you'll use it to
  build small on-device C tools (the headset has no compiler).
- `fastboot`, `adb`, `git`, standard build tools.
- Plenty of disk (pmbootstrap pulls a full Alpine build environment).

A macOS or Linux **control machine** for USB networking + SSH into the headset is fine
too (this bring-up drove the device from a Mac and built on Kali over the LAN).

## This repo

```
git clone https://github.com/cstahly/quest1-nura-port
cd quest1-nura-port
```

It carries the port packages, patches, tools, and these docs — **not** images, blobs, or
keys (see the README's "Deliberately NOT in this repo").

## Definition of done

- [ ] `fastboot getvar all` shows the bootloader **unlocked**.
- [ ] `pmbootstrap --version` prints 3.11.x.
- [ ] You can reach both the Quest (fastboot) and the build host.
- [ ] You've read [`08-safety-and-dead-ends.md`](08-safety-and-dead-ends.md) — the rules
      that keep a single irreplaceable device alive.

## When it fails

- **`fastboot` doesn't see the device:** it must be in bootloader/USB-update mode, not
  booted Android. Power off fully, then hold **Vol-Down + Power**.
- **Not unlocked:** this tutorial can't help — QuestStack is upstream of everything here.

→ Next: [`01-back-up-stock.md`](01-back-up-stock.md)
