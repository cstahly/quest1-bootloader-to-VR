# 8 — Gotchas + dead ends

Things that cost time, so you skip them.

## Dead ends — don't bother

- **GPU acceleration is dead on this hardware.** No open-source Vulkan driver for the
  **Adreno 540** (Turnip is a6xx+; a540 is out of scope). Monado's compositor is Vulkan →
  it stays **software (Lavapipe)**, period. Freedreno *GL* over KGSL covers a5xx but
  SIGSEGVs at EGL init and wouldn't help the Vulkan compositor anyway. Plan around
  software rendering. (`../KGSL-ACCELERATION.md`)
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
  command — use `pgrep -f … | xargs kill`.
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

## The one gap to turnkey

The rootfs rebuilds clean (stage 2). The boot image still needs the stage-3 hand step
(device-name root mount + BootSignature graft from your own `boot_b`), because the
device-name-mount fix isn't in the port initramfs yet. Fold that in — teach the initramfs
to mount root by mapper device name, and strip the diagnostic scaffolding (30s pause,
ports 2323/2324, extra watchdogs) — and it's a single `pmbootstrap install`.

← [index](README.md)
