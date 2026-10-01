# 3 — Prepare boot + flash

The non-turnkey stage. A clean `boot.img` won't boot as-is, for two device reasons:

1. **cmdline truncation.** The bootloader reads only the first 512 bytes of the kernel
   cmdline. pmbootstrap appends the rootfs's full `pmos_root_uuid`, which gets chopped →
   root not found by UUID. Fix: mount by device name instead —
   `pmos_root=/dev/mapper/oculus-pmos-root`. (Not baked into the port initramfs yet;
   that's the "cleaner fix" still open, so you do it by hand here.)
2. **Legacy boot footer.** The tested unlocked boot path needs a `BootSignature` page, grafted from *your own*
   stock `boot_b` (stage 1). That's why the backup wasn't optional.

The recovered finalizer is now tracked as `tools/prepare-monterey-boot`.
`tools/prepare-4k-boot.py` now accepts paths and performs the short device-name
cmdline conversion before calling it. Neither tool patches the initramfs or flashes
hardware. A guarded first-boot ramdisk is still required; see the explicit remaining
ramdisk/clean-image gap in [the review brief](../reproducibility-gaps.md).
Copying a legacy footer does not cryptographically sign changed payloads.

The cmdline must fit entirely in the first 512 bytes and preserve required boot/recovery
arguments. A binary `grep` does not prove the bootloader sees the intended cmdline;
parse the Android header and check the primary field and cleared extension.

## Finalize + flash

```
python3 tools/prepare-4k-boot.py <unsigned-exported-boot.img> <owner-stock-boot_b.img> <new-finalized-boot.img>
# Prints cmdline length, complete cmdline, payload size and output SHA256.
# Requires v0/page4096, pmos_force_initramfs and a cmdline under512 bytes.
```

`tools/prepare-4k-boot.py` is the reference for how the guarded image was assembled; see
`../boot-bringup.md` §5A/§5B for the cmdline detail.

Flash slot **B** only after all image checks pass (A remains untouched):

```
# device in fastboot: power off, Vol-Down+Power
fastboot flash system_b <exported system_b image>
fastboot flash boot_b   <finalized-boot.img>
# inspect current-slot and slot B metadata; activate B only after verifying its images
fastboot getvar current-slot
fastboot getvar slot-unbootable:b
fastboot getvar slot-retry-count:b
fastboot set_active b
fastboot reboot
```

## Check

- both flashes OKAY
- tool validates the header and short device-name cmdline; image carries a BootSignature
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
