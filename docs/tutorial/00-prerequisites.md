# 0 — Prerequisites

## Precondition: unlocked bootloader

This starts at an unlocked bootloader. Unlocking is a separate chain — **QuestStack**
(GhostLock/ionstack temp-root + CVE-2021-1931 ABL overflow → `fastboot oem unlock`). Not
in this repo. Check:

```
fastboot getvar all 2>&1 | grep -Ei 'unlock|flash.locked|verifiedbootstate'
# want: unlocked:yes  (flash.locked=0, verifiedbootstate=orange)
```

Locked → do QuestStack first, nothing here applies.

## Hardware

Quest 1 (`monterey`, SD835/MSM8998, WCN3990 Wi-Fi), a real USB-C data cable, the
controllers (later stages).

## Build host

Linux (this was built on Kali):
- **pmbootstrap 3.11.x** (`pip install pmbootstrap`; this port used `fde5aad`)
- pmbootstrap's chroot gives you the **aarch64 cross toolchain** — you'll need it, the
  headset has no compiler
- `fastboot`, `adb`, `git`, build tools, lots of disk

A macOS/Linux box for USB-net + SSH into the headset works fine as the control side.

## This repo

```
git clone https://github.com/cstahly/quest1-bootloader-to-VR && cd quest1-bootloader-to-VR
```

Port packages, patches, tools, docs — no images/blobs/keys.

## Check

- `fastboot getvar all` → unlocked
- `pmbootstrap --version` → 3.11.x
- you can reach the Quest in fastboot and the build host

**fastboot not seeing it?** It has to be in bootloader mode — power off fully, hold
**Vol-Down + Power**.

→ [1 — back up stock](01-back-up-stock.md)
