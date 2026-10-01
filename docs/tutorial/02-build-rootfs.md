# 2 — Build the rootfs

pmbootstrap builds the Alpine-based rootfs for the port. The one thing that makes it
*boot* on this hardware:

> The Quest's UFS is **4Kn** — `logical_block_size = 4096`. `mkfs.ext4` defaults small
> filesystems to **1024-byte blocks**. A block smaller than the device's logical sector
> is unmountable, and this 4.4 kernel *oopses* on the mount instead of erroring — looks
> exactly like a kernel bug. Fix: **`mkfs.ext4 -b 4096`.**

That, plus networking/rmtfs fixes, is what this repo's packages + patch add on top of the
upstream Block-Flock port.

## Build

**1. Graft the port packages** into a pmaports checkout (git doesn't track the grafted
packages; they live in this repo's `packages/`):

```
cp -r packages/* <pmaports>/device/testing/
```

**2. Patch pmbootstrap with the 4Kn fix** — it's in pmbootstrap's `format.py`, *not*
pmaports (that's the reproducibility caveat):

```
cd <pmbootstrap>
git apply <this-repo>/patches/pmbootstrap-monterey-4k.patch
# conditions on device=="oculus-monterey": forces mkfs.ext4 -b 4096,
# drops too-new ext4 features (orphan_file, metadata_csum, metadata_csum_seed)
```

**3. Initialize/configure, then build + export:**

Exact observed source pins and a fresh-host configuration recipe are now recorded in
[the boot log](../boot-bringup.md#reproducibility-review--recovered-boot-tools-and-host-pins-2026-10-01).
The original host used `oculus-monterey`, UI `none`, and the `edge` channel. A fresh
host must initialize it against the intended pmaports checkout and select the device,
architecture and intended minimal userspace. Do not blindly accept prompts with
`yes`; the exact clean-host configuration is an outstanding reproduction item.
The following commands assume that configuration is already correct. Protect the
installation password from shell history/process logs in your local workflow.


```
./pmbootstrap.py build --force device-oculus-monterey
./pmbootstrap.py install --password <pw>
./pmbootstrap.py export                        # → boot.img + system_b image
```

## Check

Block size **must** be 4096 before you flash:

```
dumpe2fs -h <exported-root-fs> | grep 'Block size'     # → 4096
```

Says 1024 → patch didn't take (you patched *pmbootstrap*, not pmaports? rebuilt with
`--force`?). Don't try to `tune2fs` your way out — it can't change block size, rebuild.

**Other snags:** `dl-cdn.alpinelinux.org` "v2 package integrity error" → switch the
Alpine mirror to `mirrors.edge.kernel.org`. Port checksum drift → `pmbootstrap checksum
<pkg>` then `build --force`.

Detail: `../boot-bringup.md` §0 (root cause), §6 (fix), §7 (reproduce).

→ [3 — prepare boot + flash](03-prepare-boot-and-flash.md)
