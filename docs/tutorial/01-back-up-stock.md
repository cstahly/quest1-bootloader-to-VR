# Stage 1 — Back up stock (the recovery net)

**Goal:** a complete, pristine set of partition backups so any experiment is reversible.

## Concept

You have one device. Before flashing anything, capture enough of stock Android that you
can always return. Two backups matter most: **`boot_b`** and **`system_b`** — the exact
pair you'll overwrite. Stage 3 also *reuses* your stock `boot_b` to re-sign the new boot
image, so this isn't optional even if you're feeling brave.

These backups are also where your **device-specific blobs** come from later: WLAN
firmware, camera/IMU factory calibration, the lens distortion mesh (stages 5–6). They
are Meta's IP and specific to your unit — keep them **local and private**, never in git.

## Steps

With the device unlocked and in a rooted/temp-root state (QuestStack gives on-demand
root), or via fastboot for the flashable partitions, dump at minimum the A/B boot and
system slots plus the calibration/persist areas. Use your own storage path; this
tutorial refers to it as `$VELA/<serial>/`.

```
# Identify the partition map on-device (rooted shell):
ls -l /dev/block/by-name/            # symlinks: boot_a, boot_b, system_a, system_b, ...

# Dump the two you'll overwrite (rooted shell; pull with adb, or dd to sdcard then pull):
dd if=/dev/block/by-name/boot_b    of=/sdcard/boot_b.img    bs=4M
dd if=/dev/block/by-name/system_b  of=/sdcard/system_b.img  bs=4M
# ...and boot_a/system_a, persist, modemst*, fsg, and the calibration partitions.
adb pull /sdcard/boot_b.img   $VELA/<serial>/boot_b.img
adb pull /sdcard/system_b.img $VELA/<serial>/system_b.img
```

Record a checksum for each so you can prove they're untouched later:

```
sha256sum $VELA/<serial>/*.img > $VELA/<serial>/SHA256SUMS
```

## Definition of done

- [ ] `$VELA/<serial>/boot_b.img` and `system_b.img` exist and are non-empty.
- [ ] `sha256sum -c $VELA/<serial>/SHA256SUMS` passes.
- [ ] You have a known restore command in hand, e.g.
      `fastboot flash boot_b $VELA/<serial>/boot_b.img && fastboot flash system_b $VELA/<serial>/system_b.img`
      — verified to boot stock Android **before** you proceed.

## When it fails

- **`dd` on the wrong slot:** always resolve names via `/dev/block/by-name/` — never
  hardcode `sdaN` numbers; they differ between units and firmware.
- **Never dump-and-restore the bootloader, modem, or NV/factory partitions as part of
  routine work.** You back them up once for insurance; you do not flash them here. (See
  stage 8.)

## Restore path (keep this visible)

To go back to stock at any time: reboot to fastboot, flash the stock `boot_b` +
`system_b` pair from this directory, `fastboot reboot`. This is the demonstrated,
known-good return path referenced throughout the boot doc.

→ Next: [`02-build-rootfs.md`](02-build-rootfs.md)
