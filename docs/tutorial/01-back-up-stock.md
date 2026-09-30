# 1 — Back up stock

One device. Before flashing, dump enough of stock Android to get back. Two backups
matter most: **`boot_b`** and **`system_b`** — the pair you'll overwrite. Stage 3 also
*reuses* your stock `boot_b` to finalize the new boot image, so you need it regardless.

Stock system/runtime files supply code, firmware and lens assets. Unit camera/IMU
calibration was found in the **private partition**, mounted as Android `/persist`;
it is not all contained in `system_b`, nor necessarily in the GPT partition named
`persist`. Back up the resolved owner partitions read-only. Keep backups private,
out of Git, and record partition names, sizes and hashes.

## Dump

From a rooted shell (QuestStack gives on-demand root), resolve names via `by-name` —
never hardcode `sdaN`:

```
ls -l /dev/block/by-name/                      # boot_a, boot_b, system_a, system_b, ...
dd if=/dev/block/by-name/boot_b   of=/sdcard/boot_b.img   bs=4M
dd if=/dev/block/by-name/system_b of=/sdcard/system_b.img bs=4M
# also grab boot_a/system_a, persist, modemst*, fsg, calibration — insurance
adb pull /sdcard/boot_b.img   <private-backup-directory>/boot_b.img
adb pull /sdcard/system_b.img <private-backup-directory>/system_b.img
sha256sum <private-backup-directory>/*.img > <private-backup-directory>/SHA256SUMS
```

## Check

- `boot_b.img` + `system_b.img` exist, non-empty, `sha256sum -c SHA256SUMS` passes
- document the exact matched boot/system restore pair and slot selection. The owner
  demonstrated restoration to stock during bring-up; that is historical evidence,
  not a reason to overwrite a working device merely to test a checksum.

That flash pair is your return-to-stock path from here on. Slot A is left untouched; verify that it actually contains a bootable stock system
before relying on it as a second fallback — you only ever touch B.

→ [2 — build the rootfs](02-build-rootfs.md)
