# Cameras, passthrough and the drawing playground

This is part of the achieved bring-up, not a future camera-driver wish list. All
four outward-facing OV7251 cameras stream real **monochrome** images. They are not
colour passthrough cameras. Their current presentation and positional tracking are
separate features.

## Capture path

[Camera tools](../../tools/camera) use the owner's stock Bionic camera HAL in a
restricted RAM root. Stock code is read-only; only required camera/media/ION/SyncBoss
nodes are exposed. No block devices or writable factory partitions are needed.
`stock-probe.c`, `prepare-stock-root.sh` and `run-stock-camera.sh` are the source
entry points; the preload must explicitly link stock Bionic `libc` and `libdl`.
A turnkey asset-extraction/build/install command is not yet provided.

Normal boot is required: the stock kernel skips camera probing in charger mode.
All four sensors delivered 640×480 8-bit frames. The 640×481 buffer's first row is
metadata, not imagery. MCU setup, tagged exposure and acknowledgements were needed;
querying four video nodes alone did not prove capture. Initial near-black images
were resolved with exposure setup and a lit, unobstructed room.

Raw capture is about 60 Hz; short controller-exposure frames are rejected for preview.
The published feed is four 320×240 images at about 30 Hz, with timestamps, counters,
exposure and gain. The atomic 307328-byte ABI is documented in
[camera-feed.h](../../tools/camera/camera-feed.h). The renderer rejects stale or
malformed frames. Driver timestamps are not calibrated photon timestamps.

`oculus-camera` is installed/enabled on the development headset. Its
[service](../../runtime/oculus-camera.initd) depends on the guard and read-only stock
runtime. Clean camera stop/start worked; investigate the later missing autostart
before describing all cold boots as reliable. Do not restart Wi-Fi along with it.

## What the wearer has seen

Four live camera panels were placed in a world-space arc to the left of the cube,
like furniture, rather than in the HUD. Actual framebuffer composites have been
captured. Earlier plain grey squares were synthetic layout tests, not camera images.

Factory camera intrinsics/extrinsics were read from the owner's **private partition**
(Android's actual `/persist` backing), not merely from `system_b` or the GPT partition
named `persist`. Four Fisheye62 models and camera-to-device transforms support a
fixed-depth composite. That composite has parallax/seam errors and was **rejected by
the wearer**. It is not calibrated, comfortable stereo passthrough.

A smaller combined image on a floor panel beside the cube is the requested replacement.
Floor-map source exists; final wearer acceptance is still unrecorded. Preserve the
four individual camera stations. Do not call a fixed-depth panorama depth-aware.

## Drawing and controls

- **Hold A or X and move your head:** a head-aimed pink line is drawn in the scene;
  release saves the stroke. The wearer confirmed this works and enjoyed it.
- The stick adjusts pen depth (0.4–8m). It does **not** move the viewpoint.
- Grip/B implements hold-to-peek; stick click cycles raw images, edges and motion
  trails. Implemented mappings still need systematic controller/wearer validation.
- Index-trigger hold remains reserved for recovery renewal.

Strokes save atomically to `/var/lib/monado/monterey/spatial-ink.bin` (up to 2048 points).
Back up drawings before replacing or testing persistence. Drawing is head-aimed:
controller pointers/button events do not prove tracked hand position.

Effects consume a shared camera feed without changing raw tracking inputs. Current
motion trails use brightness differences, not optical flow. Planned creative uses
include optical flow, particles, silhouettes and other camera-driven 3D drawing.

## Still to do

Correct stitching and near-object parallax; verify the floor panel; improve exposure
and camera startup; implement real controller aiming; finish measured physical
movement tracking; extend creative effects. No joystick locomotion and no desktop.

[Full camera history/roadmap](../CAMERA-ROADMAP.md) ·
[Status and next steps](09-status-and-next.md) · [Index](README.md)
