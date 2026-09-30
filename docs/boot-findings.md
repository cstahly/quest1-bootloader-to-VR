> **⚠️ SUPERSEDED — read `MONTEREY-PMOS-HANDOFF.md` instead.** It has the full
> data and the corrected root cause for blocker #4: the mount crash is almost
> certainly an **ext4 block-size (1024) vs UFS logical_block_size (4096) mismatch**
> — an image/config fix (`mkfs.ext4 -b 4096`), NOT the "kernel-level bug" this
> file's #4 claims below. Trust the handoff over the #4 note here.

# Quest 1 (monterey) postmarketOS bring-up — findings (2026-09-29)

Goal: fix Wi-Fi on the pmOS port (Block-Flock/pmaports-oculus-monterey).
Prereq: get pmOS to fully boot. Built from scratch on kali (~/pmos), unlocked Quest.

## What works
- From-scratch pmOS **kernel boots** on the hardware. USB-NCM networking comes up
  (Mac gets 172.16.42.2, device 172.16.42.1) with the FULL deviceinfo cmdline.
- The subpartition mapper works: /dev/mapper/oculus-pmos-{boot,root} created correctly
  from system_b's inner GPT; blkid reads them as ext2/ext4 with the right labels.
- A reliable initramfs debug shell was added (patched oculus-initramfs-recovery-hook
  to background-start `nc -lk -s 172.16.42.1 -p 2323 -e /sbin/oculus-debug-login`
  AFTER net is up, non-blocking — the port's own `oculus.force-debug` is broken: it
  binds nc before the IP exists and blocks boot). Shell reliably opens ~30s after boot.

## Blockers found (in order hit)
1. **cmdline UUID truncation.** pmbootstrap's cmdline (deviceinfo + pmos_boot_uuid +
   pmos_root_uuid + pmos_rootfsopts) overflows the Android boot header's 512-byte main
   cmdline field, and THIS bootloader ignores the extra_cmdline field — so pmos_root_uuid
   gets chopped mid-string (`d23502fb-5ca4-4cf0-bbf` instead of `...bbfe-67ad8819c696`).
   pmOS can't find root by the broken UUID. Confirmed in /proc/cmdline.
2. **Trimming the cmdline breaks USB networking.** Removing veritykeyid/msm_rtb.filter to
   make room for the full UUID reliably kills the USB-NCM bring-up (no net, no shell) —
   reproduced twice. Mechanism not understood. So "full cmdline = net but truncated UUID"
   vs "trimmed = full UUID but no net" — a conflict with no resolution yet.
3. **ext4 feature incompatibility (fixed at image level).** Alpine's mkfs.ext4 creates
   pmOS_root with `orphan_file` (kernel 5.15+), `metadata_csum`, `metadata_csum_seed` —
   the 4.4 kernel can't mount them → EINVAL. Fix: `tune2fs -O ^orphan_file,^metadata_csum,
   ^metadata_csum_seed` + `e2fsck -fy` on the image's p1/p2 (see pmos-system-compat.img).
   A proper fix belongs in mke2fs.conf / the device pkg so mkfs never adds them.
4. **Mounting pmOS_root CRASHES the kernel** (device drops off the USB bus, no recovery),
   even with features stripped. This is the real wall — matches the port README's note
   that this kernel "gets partition I/O wrong when you override the sector size" (UFS is
   4096-byte logical sectors; the inner GPT uses 512-byte LBAs; the dm-linear/loop path
   mishandles it). This is a kernel-level bug, not an image/config fix.

## Where to resume
- Blocker #4 is the crux and is genuine kernel debugging (the mapper's dm-linear sector
  handling, or the ext4/UFS I/O path). Get a serial UART or kexec-crashdump to see the
  oops. Coordinate with the port maintainer (TheCez / Block-Flock) — this is their
  unsolved territory.
- Blockers #1/#2 need untangling: why does a shorter cmdline kill USB net? Compare
  /proc/cmdline of a full vs trimmed boot (needs net on the trimmed boot, which is the
  problem). Alternatively patch the initramfs to mount root by the /dev/mapper device
  name instead of pmos_root_uuid, sidestepping the length limit entirely.
- Build tree + all images are staged: kali ~/pmos (pmbootstrap), and
  /private/tmp/.../quest-pmos on the Mac (boot/system variants). Recovery backups +
  quest1-root.sh here on vela.

## Wi-Fi (the original goal) — analysis already done, not yet testable
- NOT a QRTR kernel port. WLFW is an ADSP protection-domain (adspua.jsn); needs ADSP PIL
  bring-up + writable rmtfs. All firmware present (adsp.mdt/b00-b11, wlanmdsp.mbn, bdwlan*).
  Cellular-modem PIL (the maintainer's wedge) is NOT needed. Experiment script staged at
  pmos/wifi-bringup-experiment.sh. Needs a booting pmOS first (blocked on #4).
