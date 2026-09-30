# Quest 1 (monterey) XBL / secure-boot reverse-engineering notes

_Research notes — MSM8998 boot chain, from the device's own signed images (read-only).
The motivating goal was cosmetic (replace the boot logo); the interesting artifact is a
map of the secure-boot verification and the authenticator implementation. 2026-09-30._

> **Legality/scope:** all analysis is of the owner's own device backups, read-only.
> Nothing was flashed. No bypass was executed. This is a static-analysis map + leads.

## Where the boot logo actually is (settled)

- The **`splash` partition** holds two BMPs — both the **old grey Oculus logo**, unused.
- **ABL (`LinuxLoader`) does NOT contain the Meta "M".** Decompiled proof: its
  "Oculus graphics" code calls GOP `Blt` with `BltOperation = 1`
  (`EfiBltVideoToBltBuffer`) — it *reads* the framebuffer into a backup buffer
  (`FUN_00017240` / "Backup the boot logo blt buffer") and later restores it. It
  preserves a logo an earlier stage drew; it does not draw one.
- Therefore the **M is drawn by XBL** (which has `BootDisplayLib/BootDisplay.c` +
  `FontLib/Font.c`). XBL is **signed, not encrypted** (only sig/cert blocks are
  high-entropy). Raw-image scans (BGRA/RGB565/blue-hue) found **no plain bitmap**, and
  the one "blue" hit was a 1.5 MB high-entropy blob → the M is either **compressed** in
  XBL or **rendered via FontLib** (glyph/vector). Not yet pinned to exact bytes.

## The verification spec (what PBL checks on XBL) — from the actual certs

Parsed out of `xbl_b`'s hash-table segment (`seg[1]`, ELF flags `0x02200000`):

- **Hash:** SHA-256 per segment (`OU 07 0001 SHA256`).
- **Signature:** **RSA-2048, RSA-PSS** on the leaf (`Signature Algorithm: rsassaPss`).
- **Cert chain:** `FW_Root_Pub` → `FW_Int_Pub` → leaf `CN=Oculus … security@oculus.com`.
  The **root's SHA-256 is the fused `OEM_PK_HASH`** (the trust anchor).
- **OU binding fields (leaf):** `HW_ID 3002000001370000` (JTAG `0x30020000` = MSM8998,
  OEM `0x0137`), `OEM_ID 0137`, `SW_ID 0`, `DEBUG 0` (debug fuses off),
  `IN_USE_SOC_HW_VERSION 1`.
- **`# SecBootEnableFlag = 0x1`** string present → **secure boot enforced.** (Definitive
  confirmation = read the QFPROM fuse on-device; ~certain given `androidboot.secure=1`.)

**To boot modified XBL you must forge a hash-table segment that (1) carries correct
SHA-256 of the patched segments [trivial] and (2) verifies under RSA-2048-PSS up a chain
rooting to the fused `FW_Root_Pub` hash [the wall].** No key, no RSA-2048 factoring →
the only doors are a **verifier logic bug** or **fault injection** (tethered).

## The authenticator, located and decompiled

XBL contains its own `boot_authenticator.c` (it authenticates the images XBL loads:
TZ/ABL/PMIC — same scheme PBL uses on XBL). The whole module is decompiled in the Ghidra
project (`~/ablwork/proj`, program `xbl_b.img`), function range **`0x1402c000`–`0x1402e400`**
(118 functions). Key pieces identified:

- **`FUN_1402cf98`** — MBN hash-header parse. Reads size fields from the (attacker-supplied)
  header at offsets **`0x14` (code_size)**, **`0x1c` (signature_size)**, **`0x24`
  (cert_chain_size)** into a state struct. Header/data body starts at base `+0x28`.
- **`FUN_1402d040`** — pointer setup. Computes:
  - data   = base + 0x28
  - **sig  = base + 0x28 + code_size**
  - **cert = base + 0x28 + code_size + signature_size**
  …directly from the header sizes, **with no visible bound against the hash segment's
  actual allocated size.** ← **LEAD (Tier-2 integer overflow / OOB pointer).**
- **`FUN_1402cd50`** — appends per-segment hash entries (0x30 = 48 bytes each) into a
  buffer; has a `xbl_sec` hardened/duplicated path (anti-glitch double-check) gated by
  `FUN_1402c458`. Bounds the copy (`>0x2f` room check), so this loop looks guarded.
- Big unread verify candidates (next to read): **`FUN_1402c838` (480)**,
  **`FUN_1402cb98` (408)**, **`FUN_1402d250` (364)**, **`FUN_1402d408` (260)**,
  **`FUN_1402dc80` (376)** — these should contain the segment-hash **compare loop**
  (coverage question) and the **RSA-PSS** call.

## Open leads, ranked (the "paper")

1. **Tier-2 (memory corruption):** does anything bound `code_size`/`signature_size`/
   `cert_chain_size` (from `FUN_1402cf98`/`d040`) against the segment's real size before
   they're used as pointers/lengths? If not, an oversized header field → OOB read/write
   in the verifier = code-exec in the bootloader. **Start here** — it's the most concrete.
2. **Tier-1 (coverage gap):** find the segment-hash compare loop (likely `FUN_1402c838`/
   `cb98`). Does it hash *every loadable* segment, or can a segment be loaded/executed but
   skipped (count mismatch, `filesz`/`memsz` trick, a flag)? Append-unhashed-segment = win
   without touching the signature.
3. **Tier-1 (anchor confusion):** confirm the root-cert SHA-256 is compared strictly
   against the fused value (not a root supplied in the image).
4. **Tier-2 (RSA-PSS):** once the PSS verify function is found, check trailer `0xbc`,
   salt-length enforcement, MGF1, and digest-length handling.

**Honest caveat for a writeup:** an XBL-authenticator bug is confirmable on-device (forge a
TZ/ABL image XBL loads) and is a strong result on its own. A bug that lets you boot
*modified XBL itself* requires the **PBL** verifier to share the flaw — and proving that
needs PBL dumped from the SoC boot ROM (not in any partition). State the bootrom-vs-
bootloader gap explicitly.

## Recovery net (before ever writing to XBL)
Confirm **EDL (9008) reachability** on monterey **and** availability of a PBL-accepted
**firehose** (`prog_ufs_firehose_8998`) — that's the only way to restore a bad `xbl_b`
(stock backup is in the vela recovery dir). Without it, a bad XBL flash = permanent brick.

## Reusable artifacts
- Ghidra project: `~/ablwork/proj` (ABL `abl-dxe.efi` + `xbl_b.img`, both analyzed).
- Extracted `LinuxLoader` PE, carved certs (`xbl_cert*.der`), decompile logs in
  `~/ablwork/` and the job tmp. JDK note: Ghidra needs the non-Homebrew arm64 JDK and
  `DYLD_LIBRARY_PATH` unset (Homebrew libz breaks its launcher).
