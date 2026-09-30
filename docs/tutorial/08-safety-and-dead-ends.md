# 8 — Gotchas + dead ends

Things that cost time, so you skip them.

## Dead ends — don't bother

- **GPU acceleration is unverified, not ruled out.** The inspected Turnip path does
  not support Adreno 540. An isolated Freedreno OpenGL KGSL build reaches EGL init
  but crashes; it has not rendered/read back a frame. That does not prove no GPU
  path is possible. Monado's Vulkan compositor would need additional integration
  even if GL succeeds. See [KGSL findings](../KGSL-ACCELERATION.md).
- **ADSP-only Wi-Fi was wrong** — WLAN is a *modem* protection domain. Don't PIL-boot the
  ADSP for Wi-Fi; run `cnss-daemon` (stage 5).
- **Boot-logo swap is behind the root of trust.** The "M" is a raw blt buffer drawn by
  **XBL**, which is signed (RSA-2048-PSS / SHA-256, chained to Oculus's fused
  `FW_Root_Pub`). Cosmetic pixels in full secure-boot armor. RE notes are fun; the change
  isn't cheap. (`../xbl-secureboot-notes.md`)
- **`tune2fs` can't fix a 1024-block image** — doesn't change block size, rebuild with
  `-b 4096`.
- **Magisk / boot-ramdisk root** — ignored by legacy system-as-root.

## Kernel/shell traps

- **Don't `cat` `fb0`/`partial_vsync` sysfs** — a read NULL-derefs and oopses the kernel
  (kills the `cat`, device survives).
- Mounting a **1024-block** rootfs oopses the kernel — that's the 4Kn bug, not a flaky
  link (stage 2).
- **`pkill -f "<pat>"`** kills its own shell when the pattern matches the invoking
  command — inspect exact PIDs, executable names and parentage before terminating a process.
- **`fastboot`** cmdline flag is `--cmdline`, not `-c`.

## Device caveats (learned the hard way)

- **Watchdog:** the bring-up images arm a ~300 s recovery timeout so a bad boot returns to
  fastboot on its own. `oculus.recovery_timeout=0` once hung it off-bus for hours with no
  auto-recovery — leave the watchdog on unless you like power-cycling. A ~5-min drop is it
  working.
- **Flash B only.** A stays stock as your fallback; you don't need to touch bootloader,
  modem, or NV for any of this.
- **`/run` is tmpfs** — anything there is gone on reboot (bit us on the VIO captures).
- The **cnss-daemon holds an open modem FD** — don't restart it in place; it's freed at
  the next recovery reboot.

## Recovery and unfinished production behavior

Slot B has a finite retry count. A working Linux boot has still exhausted it and
returned to fastboot. Inspect `current-slot`, `slot-unbootable:b` and
`slot-retry-count:b`; the documented owner recovery used `fastboot set_active b`
followed by reboot **after confirming B held the known guarded image**. This is not
boot-success marking and not permission to select an unknown slot blindly.

Do not assume pressing Power always returns to the scene indefinitely. Charger mode
can boot on USB-connected poweroff and omit camera probing. Boot-success marking,
charger-only behavior and unplugged startup verification remain work.

The blue-logo investigation is paused; bootloader components are untouched. Nothing
in this guide requires changing XBL/ABL or weakening secure boot.

## More than one gap to turnkey

Boot finalization, clean pmbootstrap setup, private asset extraction, runtime
packaging, optics/compositor integration and positional tracking remain incomplete.
Do not strip recovery timers to make an image look production-ready. Debug shells
are development scaffolding and need a separately reviewed production access policy.
See the [current status and TODO](09-status-and-next.md).

← [index](README.md)
