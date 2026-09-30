# 3 — Prepare boot + flash

The non-turnkey stage. A clean `boot.img` won't boot as-is, for two device reasons:

1. **cmdline truncation.** The bootloader reads only the first 512 bytes of the kernel
   cmdline. pmbootstrap appends the rootfs's full `pmos_root_uuid`, which gets chopped →
   root not found by UUID. Fix: mount by device name instead —
   `pmos_root=/dev/mapper/oculus-pmos-root`. (Not baked into the port initramfs yet;
   that's the "cleaner fix" still open, so you do it by hand here.)
2. **Boot signature.** The image needs a valid `BootSignature`, grafted from *your own*
   stock `boot_b` (stage 1). That's why the backup wasn't optional.

`prepare-monterey-boot` (in `tools/`) does the graft; you supply the device-name cmdline.

## Finalize + flash

```
prepare-monterey-boot <exported boot.img> $VELA/<serial>/boot_b.img <finalized-boot.img>
# make sure the cmdline mounts root by device name, not UUID:
grep -a pmos_root <finalized-boot.img>        # → /dev/mapper/oculus-pmos-root
```

`tools/prepare-4k-boot.py` is the reference for how the guarded image was assembled; see
`../boot-bringup.md` §5A/§5B for the cmdline detail.

Flash slot **B** (A stays stock):

```
# device in fastboot: power off, Vol-Down+Power
fastboot flash system_b <exported system_b image>
fastboot flash boot_b   <finalized-boot.img>
fastboot reboot
```

## Check

- both flashes OKAY
- cmdline greps as device-name mount (above), image carries a BootSignature
- you flashed **B only** — not A, bootloader, modem, or NV

## Notes

- The bring-up images keep a **~300 s recovery watchdog** — a bad boot returns to
  fastboot on its own. A boot that drops off after ~5 min is the watchdog working, not a
  brick. Leave it armed; `oculus.recovery_timeout=0` once hung the device off-bus for
  hours with no auto-recovery.
- `fastboot`'s flag is `--cmdline`, not `-c`.
- Magisk / boot-ramdisk root doesn't apply — legacy system-as-root ignores it.
- Root not found / mount oops after flashing → back to the cmdline (device-name mount?)
  or 4Kn (stage 2 `dumpe2fs` said 4096?).

Detail: `../boot-bringup.md` §5, §6, "CONFIRMED FULL BOOT + SSH", "REMAINING
consolidation item".

→ [4 — first boot + SSH](04-first-boot-and-ssh.md)
