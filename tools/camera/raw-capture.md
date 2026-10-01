# One-shot full-resolution capture (offscreen diagnostic only)

This optional feature supports comparing decimation/filter choices against the
**same** original four-camera scene cohort. It is not a recorder, tracking input,
stitched panorama, or replacement feed ABI. No room images belong in this repo.

`CAMERA_RAW_CAPTURE` defaults to 0/unset: no extra raw allocation, copy, or request
file polling. With `CAMERA_RAW_CAPTURE=1`, startup removes any stale request marker
and logs that it is waiting. This does **not** capture the startup frames.

After inspecting camera diagnostics and deciding the scene/exposure are suitable,
the operator creates a **new** empty private marker at
`/tmp/quest-camera-raw-v1.request` (for example with `umask 077; touch ...`). This
must happen after the waiting log. The probe checks at most four times per second,
consumes one request, and logs `RAW_CAPTURE armed ns=...`. No automatic assumption
is made that exposure has settled. Configured auto-exposure and startup exposure
verification are diagnostic flags, not proof the requested/applied state now matches.

The probe then copies retained scene buffers while it still owns each HAL buffer.
Short controller exposures below 1000 microseconds are excluded by the existing
classification. It captures the first cohort with all four timestamps **strictly
later than request consumption**, matching sequence/timestamps, and maximum skew
at most the existing 1 ms gate. It gives up after five seconds if that does not
happen. Only one allocation/write attempt is permitted per process; success,
allocation/IO failure, or timeout disables capture until an explicit process restart.

Success is the log `RAW_CAPTURE one-shot complete` and an atomically replaced file
`/tmp/quest-camera-raw-v1.bin`, mode 0600. The preceding request and capture timestamps
in its header identify the new artifact. An old `.bin` is intentionally preserved
on failure; existence alone does not prove success. A private `mkstemp` file is
fully written and closed before rename; incomplete files are never published.
Failed temporary writes are unlinked. This is atomic publication, not a durability
promise across power loss. No persistent firmware, calibration, or capture defaults
are changed by this feature.

Enabled/armed staging allocates about 1.23 MB, copies incoming full-resolution scene
buffers until one cohort is ready, and performs one synchronous ~1.23 MB write.
This can transiently delay camera processing and **must be used for offscreen bench
capture**, not during a wearer comparison. One static cohort supports pixel/filter
analysis; it cannot validate motion tracking or low-light movement by itself.

## Version 1 format

All integers are explicitly encoded little-endian; no native C struct layout is
serialized. Total size is 1,231,616 bytes. The header is 256 bytes, followed by four
307,840-byte camera payloads in **physical camera order 0,1,2,3**. These are not the
reordered VIO logical order. Each payload starts with all 640 stock metadata bytes,
then 640×480 tightly packed 8-bit monochrome pixels with stride 640.

Header: magic `QRAW001\0` (8 bytes); u32 header size, camera count, width, height,
stride, metadata bytes, per-camera payload bytes; u32 flags at byte 36 (bit0 configured
auto exposure, bit1 startup verification, bit2 preview denoise); u64 capture host
monotonic time at 40 and request-consumption time at 48; reserved zero bytes 56–63.

Four records of 48 bytes start at 64: u32 physical camera index and sequence; u64
driver timestamp; u32 applied exposure microseconds and Q4 gain; u64 absolute payload
offset; u32 payload length, requested exposure microseconds, requested Q4 gain, and
reserved zero. Requested settings are the current software settings at capture,
while applied exposure/gain come from each retained buffer's metadata. Driver
timestamps are not calibrated photon/exposure-midpoint timestamps.

`read-raw-capture.py PRIVATE_FILE` validates exact size/version/layout, freshness,
synchronization, reserved fields, and agreement with exposure/gain metadata. It
prints metadata only. Optional `--extract NEW_PRIVATE_DIRECTORY` writes four PGM
images into a new mode-0700 directory with mode-0600 files. The parser does not apply
filtering, rotation, stitching, camera reordering, or calibration.

Run `python3 tools/camera/test-raw-capture.py` for synthetic C-producer/Python-parser
format tests, malformed-file rejection, exact pixel order, private extraction,
cohort checks, and proof that staged data survives HAL-buffer reuse. Tests contain
no real camera images. Host tests and a Bionic build do not establish device capture
success on their own. The corrected authorized offscreen capture below now also passed.

## Owner HAL layout guard (2026-10-01 correction)

The first offscreen attempt safely refused capture: the public wrapper `raw[7]`
low 32 bits were 15. The original check incorrectly treated this word as a size.
It is an **fd**; the high 32 bits are unused zero, not a hidden length. No snapshot
was written during that rejected attempt. The guard was corrected using saved
binary evidence, not removed or relaxed to accept the observed fd.

This optional capture depends on the owner's traced private HAL layout. When
`CAMERA_RAW_CAPTURE=1` or `CAMERA_FRAME_TRACE=1`, `run-stock-camera.sh` requires the stock HAL SHA-256
`109b418cec3182c069d20bda7e659666ce9202d731d1253c3302c4d5ff30dd86` before copying
or launching the preload. Hash failure/mismatch refuses launch. Other modes retain
the existing path. Raw capture must use this launcher; bypassing it bypasses the
hash precondition for reading the private frame prefix.

Disassembly of that saved `libqcameraoculushal.so` establishes:

- `0x49d0` stores an internal frame pointer at the allocated wrapper's byte 0;
  `0x49f0` returns wrapper+8. Thus the private pointer is immediately before the
  public frame, valid only while the caller owns that HAL frame.
- `0x4a00`–`0x4a0c` copy internal+0x230 (buffer pointer) to public `raw[6]` and
  internal+0x228 (fd) to public `raw[7]` low32. No length is copied into this public
  wrapper.
- `0x4e7c`–`0x4ea8` populate internal+0x228 from allocation descriptor+0x10 (fd),
  internal+0x238 from descriptor+8 (mapped length), internal+0x230 from descriptor+0
  (buffer pointer), and internal+0x240 with the descriptor pointer.
- Allocation `0x3678`/`0x3680` rounds the requested length up to 4096 bytes.
  `0x36c8` passes that length to `mmap`; `0x3710` stores it in descriptor+8.
  The 307,840-byte frame therefore occupies a **311,296-byte mapping**.

Before each extra raw copy, the corrected helper requires this exact mapped length
and cross-checks internal/public buffer pointer and fd, rejecting nonzero public fd
padding. It copies only 307,840 bytes, never allocation padding. Any failed check
logs the unsupported layout/capacity and disables this one-shot request.

Synthetic tests now include fd15 with valid mapped length, undersized/nonrounded
lengths, pointer/fd mismatches, invalid padding and null internal pointer. These
prove guard semantics for a constructed layout. The corrected device attempt
completed on2026-10-01: all four cameras sequence660,14µs timestamp skew, parser
validation and private PGM extraction passed. Requested8000µs/gain240 versus
metadata6118µs/gain240 remains an exposure investigation, not a capture-format error.
The bounded50s probe restored original camera and scene services successfully.
No private image, binary or disassembly is committed.
