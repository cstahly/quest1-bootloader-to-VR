# Stage 2 — Build the Nura rootfs

**Goal:** a bootable pmOS/Nura rootfs image for the Quest, built from source.

## Concept

pmbootstrap compiles a full Alpine-based rootfs for the device port. The one thing that
makes it *boot* on this hardware is a filesystem detail:

> The Quest's UFS storage is **4Kn** — it reports `logical_block_size = 4096`. But
> `mkfs.ext4` defaults small filesystems to **1024-byte blocks**. A filesystem block
> smaller than the device's logical sector is **unmountable** — and this old 4.4 kernel
> *oopses* on the mount instead of failing cleanly, which looks exactly like a
> mysterious kernel bug. **The fix is one flag: `mkfs.ext4 -b 4096`.**

That fix, plus networking/rmtfs fixes, is what this repo's port packages and patch add on
top of the upstream Block-Flock port.

## Steps

**1. Graft the port packages** into a pmaports checkout (git doesn't track the grafted
packages; this repo carries them under `packages/`):

```
# copy this repo's packages/* into your pmaports checkout under device/testing/
cp -r packages/* <pmaports>/device/testing/
```

**2. Apply the 4Kn block-size patch to pmbootstrap** (the block-size fix lives in
pmbootstrap's `format.py`, *not* in pmaports — this is the reproducibility caveat):

```
cd <pmbootstrap>
git apply <this-repo>/patches/pmbootstrap-monterey-4k.patch
# it conditions on device == "oculus-monterey": forces mkfs.ext4 -b 4096
# and drops the too-new ext4 features (orphan_file, metadata_csum, metadata_csum_seed)
```

**3. Build and export:**

```
cd <pmbootstrap>
./pmbootstrap.py build --force device-oculus-monterey
./pmbootstrap.py install --password <pw>     # if init re-prompts, drive with: yes '' |
./pmbootstrap.py export
```

Exports land in pmbootstrap's export dir: `boot.img` and the system image
(`oculus-monterey.img` / the inner-GPT `system_b` image).

## Definition of done

The block size **must** be 4096 on both filesystems — verify *before* flashing:

```
dumpe2fs -h <exported-root-fs> | grep 'Block size'      # → Block size: 4096
```

- [ ] `pmbootstrap export` produced a `boot.img` and a `system_b` image.
- [ ] `dumpe2fs` reports **Block size: 4096** for boot and root. (If it says 1024, the
      patch didn't apply — do not flash; go fix it.)

## When it fails

- **`dumpe2fs` says 1024:** the patch isn't in effect. Confirm you patched *pmbootstrap*
  (not pmaports) and rebuilt with `--force`.
- **`dl-cdn.alpinelinux.org` "v2 package integrity error":** index/pkg skew — switch the
  Alpine mirror to `mirrors.edge.kernel.org` and retry.
- **Port package checksum drift:** `pmbootstrap checksum <pkg>` then `build --force`.
- **Don't** try to "fix" an already-built 1024-block image with `tune2fs` — it can't
  change block size; rebuild.

## Full detail

`../boot-bringup.md` §0 (root cause), §6 (the fix), §7 (reproduce), and the
"Consolidation into port sources" section (what's in `device-oculus-monterey` r34).

→ Next: [`03-prepare-boot-and-flash.md`](03-prepare-boot-and-flash.md)
