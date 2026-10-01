# Reproducibility gaps — review brief

Written 2026-10-01 as input to a final end-to-end review (logic + technical). The
question this answers is **not** "is anything leaked / is the tone right" — it is:
**if a stranger with an unlocked Quest 1 clones this repo and follows the tutorial, where
do they hit a wall because a required artifact/step/value is not actually here?**

The docs are honest that these gaps exist (see tutorial `README.md` "Important
reproduction gaps" and `09-status-and-next.md`). But an honest admission of a hole is not
a filled hole. For an actual reproducer these are hard stops. Tracked here so the final
review closes them deliberately instead of leaving them scattered across pages.

## Hard stops — reproduction dies here

### 1. Stage 3 boot image — the showstopper
`prepare-monterey-boot`, the finalizer that turns the exported `boot.img` into a bootable
image, is **not in the repo** (absent from tracked files). The only reference,
`tools/prepare-4k-boot.py`, hardcodes one developer's paths. A reader therefore **cannot
produce a boot image at all** → never reaches first boot → stages 4–10 are unreachable.
Nothing else matters until this is closed.

Needs:
- the actual finalizer script, de-hardcoded (no developer-specific paths);
- the exact recipe for the device-name root-mount (`pmos_root=/dev/mapper/oculus-pmos-root`,
  the "cleaner fix" not yet in the port initramfs);
- the recipe for stripping the guarded bring-up scaffolding (30 s pause, TCP 2323/2324,
  extra watchdogs) into a clean unattended image.

### 2. Owner-blob extraction — no recipe, and three stages need it
`docs/assets-inventory.md` lists where **the author's** copies sit on the author's
Kali/Mac. It is **not** a procedure for a new user to pull the equivalents off **their
own** headset. There is no runnable "extract file X from partition/path Y on a stock
device, expected filename, expected sha256" manifest. Without it:
- **Stage 5 (Wi-Fi):** can't stage WLAN firmware + `cnss-daemon` + vndk libs. The doc
  says firmware extraction "is not yet a complete generic tutorial command," and the
  filename manifest the runtime expects under `/usr/share/oculus-wifi/firmware` is not
  published as a checklist.
- **Stage 6 (optics):** can't obtain `distortion-mesh.bin`.
- **Stage 10 (cameras):** can't obtain the `/persist` camera/IMU intrinsics/extrinsics
  (explicitly a TODO, not provided).

Needs: a per-file extraction manifest — source partition/path on a *stock* device, how to
pull it, expected filename + sha256 — distinct from the author-location inventory.

### 3. No single install that assembles the headset
`09-status-and-next.md` admits it: copying `packages/*` does **not** reproduce the device
— the `runtime/` guard/scene/camera services are not packaged into the pmaports build. A
reader who reaches a shell then has to hand-install every service by reverse-engineering
the repo. There is no ordered bring-up (install script / meta-package / numbered list)
that turns "booted rootfs" into "the headset you have."

Needs: one ordered install path from booted rootfs → full headset (packages + runtime
services, in dependency order).

## Secondary gaps — milestone reachable, but under-specified

- **Stage 2 (rootfs):** clean-host pmbootstrap **init/config is undocumented** — which
  device profile, UI selection, and pinned `pmbootstrap` / `pmaports` commits. The 4Kn
  patch targets a specific pmbootstrap version; if upstream moved it may not apply, with
  no fallback noted.
- **Stage 7 (Basalt native):** the on-headset path depends on a **private 19 MB
  dependency bundle with no pinned versions or build manifest** — not reproducible from
  what is published. (Also reconcile the probe figures: tutorial 07/09 say 301/301 poses,
  43 ms median / 69 ms max; `tools/tracking/README.md` says 601/601 groups and 40 ms /
  66 ms for the 15 Hz case — confirm which run is quoted so they agree.)
- **Renderer build:** OK as-is — `renderer/fast/README.md` gives the chroot build deps.

## Priority order to actually close it
1. Ship the boot finalizer + clean-image recipe (unblocks first boot — everything else is
   downstream of this).
2. Write the owner-asset extraction manifest (unblocks Wi-Fi / optics / cameras).
3. One ordered install path, booted rootfs → full headset.

## Non-defect decisions for the owner (not blockers, but call them)
- `assets-inventory.md` is the one file that must never go public — it maps every owner
  blob + the SSH key location and concentrates the Kali LAN IP, username, device serial,
  and local paths. Fine for this private repo; decide whether to mark it "never publish"
  more loudly or split it out.
