# Stage 8 — Safety rules and dead ends

Read this **before** you flash anything. It's short, and every line was paid for.

## The rules that keep the device alive

This is one physical Quest 1, and it's the owner's only one. These are non-negotiable —
for a human and doubly for an AI agent driving the device.

1. **Never disable the recovery watchdog.** The bring-up images arm a ~300 s recovery
   timeout so a bad boot returns to fastboot on its own. **Setting
   `oculus.recovery_timeout=0` once hung the device off the bus for hours** and needed a
   physical power-cycle to recover. Only ever use a short timeout, never 0. A boot that
   drops off after ~5 min is the watchdog *working*, not a failure.
2. **Only ever flash `system_b` and `boot_b`** — and only after stage 1's backups exist.
   **Never flash the bootloader, modem, NV/`modemst*`, `fsg`, or factory-calibration
   partitions.** Slot A stays stock as your fallback.
3. **Keep stock backups pristine.** They are the restore path *and* the source of your
   device-specific blobs. Treat them read-only; checksum them (stage 1).
4. **Secrets and blobs never enter git.** The bring-up SSH **private key** lives only on
   the dev machine. The Wi-Fi **PSK** lives only in the device's `0600` config. Meta
   **firmware/calibration/distortion-mesh** are the owner's and are not redistributed —
   only their formats are documented.
5. **Physical actions are the wearer's, and you pause for them.** *"If you want me to do
   something physical you need to tell me to do it, then pause."* Print the instruction,
   stop, wait for confirmation. And remember **the cord is unplugged when they walk** —
   anything during a walk runs over Wi-Fi, backgrounded, confirmed-live first.
6. **Don't reason live camera/passthrough frames off-host without asking** — the owner is
   already pushing the machine; heavy off-device processing needs a green light.
7. **Never overwrite the owner's running sandbox.** If they're booted into something
   they're actively using, you *overlay*, you don't replace it.

## Dead ends — don't re-run these

- **GPU acceleration is a permanent dead end.** There is **no open-source Vulkan driver
  for the Adreno 540** (Turnip is a6xx-and-up; a540 is out of scope). Monado's compositor
  is Vulkan, so it stays **software-bound (Lavapipe)** on this hardware forever. Freedreno
  *GL* over KGSL covers a5xx but SIGSEGVs at EGL init and wouldn't help the Vulkan
  compositor anyway. Plan around software rendering. (`../KGSL-ACCELERATION.md`)
- **The ADSP-only Wi-Fi theory was wrong** — WLAN is a *modem* protection domain. Don't
  spend time PIL-booting the ADSP for Wi-Fi; run `cnss-daemon` (stage 5).
- **Replacing the Meta boot logo is behind the root of trust.** The "M" is a raw blt
  buffer drawn by **XBL**, which is signed (RSA-2048-PSS / SHA-256, chained to Oculus's
  fused `FW_Root_Pub`). A cosmetic pixel blob wearing full secure-boot armor. The RE
  notes are interesting; the change isn't cheap. (`../xbl-secureboot-notes.md`)
- **`tune2fs` can't fix a 1024-block image** — it doesn't change block size. Rebuild with
  `-b 4096` (stage 2).
- **Magisk / boot-ramdisk root doesn't apply** — legacy system-as-root ignores it.
- **Don't `cat` `fb0`/`partial_vsync` sysfs** — a read oopses this kernel.
- **`pkill -f "<pattern>"`** can kill its own shell when the pattern matches the invoking
  command — use `pgrep -f ... | xargs kill`.

## The one gap between "research port" and "turnkey"

The **rootfs** rebuilds cleanly (stage 2). The **boot image** still needs the hand step
in stage 3 (device-name root mount + BootSignature graft from your own `boot_b`), because
the device-name-mount fix isn't folded into the port's initramfs sources yet. Closing
that — teach the initramfs to mount root by mapper device name, and strip the diagnostic
scaffolding (30 s pause, ports 2323/2324, extra watchdogs) for a clean image — is the
task that would make this fully reproducible from a single `pmbootstrap install`.

← Back to the [index](README.md)
