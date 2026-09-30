# Stage 3 — Prepare the boot image and flash

**Goal:** a boot image that actually boots the stage-2 rootfs, flashed to slot B.

## Concept — this is the one non-turnkey stage

Two device-specific facts make a clean `boot.img` *not* boot as-is:

1. **cmdline truncation.** The bootloader reads only the first 512 bytes of the kernel
   command line. pmbootstrap appends the rootfs's full `pmos_root_uuid`, which gets
   **chopped**, so root can't be found by UUID. **Fix:** mount root by *device name*
   instead — `pmos_root=/dev/mapper/oculus-pmos-root`. (This is the "remaining
   consolidation item" the boot doc flags — it's not yet baked into the port's initramfs
   sources, so you do it here by hand.)
2. **Boot signature.** The image must carry a valid `BootSignature`, grafted from **your
   own device's** stock `boot_b` (from stage 1). This is why the backup was mandatory.

The helper `prepare-monterey-boot` (in `tools/`) does the graft. You supply the
device-name-mount cmdline.

## Steps

**1. Finalize the boot image** — graft the signature from your stock `boot_b`:

```
prepare-monterey-boot <exported boot.img> $VELA/<serial>/boot_b.img <finalized-boot.img>
```

Ensure the resulting cmdline mounts root by device name, not UUID
(`pmos_root=/dev/mapper/oculus-pmos-root`). See `../boot-bringup.md` §5A/§5B and the
"REMAINING consolidation item" note; `tools/prepare-4k-boot.py` is the reference for how
the guarded image was assembled.

> ⚠️ **Keep the recovery watchdog armed.** The bring-up boot images use a ~300 s recovery
> timeout so a bad boot returns to fastboot on its own. **Never** build an image with
> `oculus.recovery_timeout=0` — that removes the only automatic way back. (Stage 8.)

**2. Flash slot B** (never A — A stays as your stock fallback):

```
# device in fastboot (power off, then Vol-Down+Power → USB update mode)
fastboot flash system_b <exported system_b image>
fastboot flash boot_b   <finalized-boot.img>
fastboot reboot
```

> **Physical action — do this, then continue:** power the headset off, hold
> **Vol-Down + Power** to enter USB update mode, and confirm `fastboot devices` lists it
> before flashing. (Agents: print this, pause, wait for the human.)

## Definition of done

- [ ] `fastboot flash system_b` and `fastboot flash boot_b` both report OKAY.
- [ ] The finalized boot image's cmdline mounts root by **device name**
      (`grep -a pmos_root <finalized-boot.img>` shows `/dev/mapper/oculus-pmos-root`,
      not a truncated UUID).
- [ ] The finalized boot image carries a BootSignature (the graft step succeeded).
- [ ] You did **not** flash A, bootloader, modem, or NV.

## When it fails

- **`fastboot: -c` rejected:** the flag is `--cmdline`, not `-c`.
- **Boots then drops off USB after ~5 min:** that's the watchdog returning to fastboot —
  expected for a guarded image, not a failure. Reflash/boot B again to retry.
- **Root not found / mount oops:** you're back to the UUID-truncation or 4Kn issue —
  confirm the cmdline uses the mapper device name and that stage 2's `dumpe2fs` said
  4096.
- **Magisk/ramdisk root tricks don't apply here** — this is legacy system-as-root; the
  boot ramdisk approach is ignored.

## Full detail

`../boot-bringup.md` §5 (blockers A/B), §6, the "CONFIRMED FULL BOOT + SSH" section
(known-good guarded image + SHA256), and "REMAINING consolidation item — boot-side."

→ Next: [`04-first-boot-and-ssh.md`](04-first-boot-and-ssh.md)
