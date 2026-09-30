# 1 — Back up stock

One device. Before flashing, dump enough of stock Android to get back. Two backups
matter most: **`boot_b`** and **`system_b`** — the pair you'll overwrite. Stage 3 also
*reuses* your stock `boot_b` to re-sign the new boot image, so you need it regardless.

These dumps are also where your **device-specific blobs** come from later (WLAN firmware,
camera/IMU calibration, the lens mesh). Keep them local, out of git.

## Dump

From a rooted shell (QuestStack gives on-demand root), resolve names via `by-name` —
never hardcode `sdaN`:

```
ls -l /dev/block/by-name/                      # boot_a, boot_b, system_a, system_b, ...
dd if=/dev/block/by-name/boot_b   of=/sdcard/boot_b.img   bs=4M
dd if=/dev/block/by-name/system_b of=/sdcard/system_b.img bs=4M
# also grab boot_a/system_a, persist, modemst*, fsg, calibration — insurance
adb pull /sdcard/boot_b.img   $VELA/<serial>/boot_b.img
adb pull /sdcard/system_b.img $VELA/<serial>/system_b.img
sha256sum $VELA/<serial>/*.img > $VELA/<serial>/SHA256SUMS
```

## Check

- `boot_b.img` + `system_b.img` exist, non-empty, `sha256sum -c SHA256SUMS` passes
- you've confirmed the restore actually boots stock **before** moving on:
  `fastboot flash boot_b …/boot_b.img && fastboot flash system_b …/system_b.img && fastboot reboot`

That flash pair is your return-to-stock path from here on. Slot A stays stock too, as a
second fallback — you only ever touch B.

→ [2 — build the rootfs](02-build-rootfs.md)
