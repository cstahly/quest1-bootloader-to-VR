# 6 — Native VR scene, optics and orientation tracking

The current accepted display path is a **direct-framebuffer diagnostic scene**, with
Monado orientation tracking and stock lens correction. No desktop is intended.
Xorg was an earlier diagnostic, not a required end-user session. This is not yet a
finished OpenXR compositor or VR home.

## Why a raw framebuffer image was not enough

The panel is 2880×1600, physically rotated relative to the logical scene. Stereo
centres first needed adjustment: two separately sharp images did not fuse until
corrected. That user-specific experiment is not a universal pixel-offset prescription.
Next, sensor axes and gravity initialization had to agree with the head coordinate
frame. Finally, the owner's stock per-channel lens distortion mesh corrected the
warping seen during head tilt. Changing field of view alone did not fix lens warp.

The renderer uses that mesh; runtime-wide Monado optics integration is still open.
Leave the headset still for gravity initialization. Forward heading is established
when the scene starts. Orientation tracking does not provide physical translation.

## Current source and deployment pieces

- [Monado patches](../../patches/monado): orientation driver changes.
- [Renderer and build notes](../../renderer/fast/README.md): CPU rasterizer, stock
  mesh loading and framebuffer submission. Older X11 benchmarks are historical.
- [Scene launcher](../../runtime/oculus-test-scene) and
  [OpenRC service](../../runtime/oculus-test-scene.initd).
- [Recovery implementation](../../runtime) and
  [autostart validation](../TEST-SCENE-AUTOSTART.md).

Installed owner paths are `/usr/bin/oculus-test-scene`, the renderer under
`/usr/libexec/oculus-test-scene`, and private optics assets under
`/var/lib/monado/monterey`. Building a device package that installs all these pieces
from a clean checkout remains work; copying a binary to `/tmp` is not that package.
Do not start another Monado service or renderer over the boot-managed instance.

## Boot behavior and controls

The verified default starts Wi-Fi, recovery guard and tracked scene automatically.
The launcher waits for actual gravity initialization (up to 60 seconds). The later
camera service is installed too, but one subsequent boot needed a manual camera
start; full camera startup reliability remains open.

- Hold either **index trigger for two seconds**, then release, to renew recovery to
  five minutes. A stuck held trigger cannot renew indefinitely.
- The status panel shows render-loop fps, drawing/submission time, angular speed,
  Wi-Fi address/signal, recovery countdown and recent frame intervals.
- Root maintenance may explicitly run `oculus-recovery-guard --renew`. There is no
  automatic keepalive; expiry still requests bootloader recovery.

The hardware watchdog remains separate and enabled. See the recovery notes before
replacing the guard executable: its control commands reject an inode mismatch with
an already-running older binary.

## Performance evidence

An early 60 fps loop displayed black because framebuffer writes were not submitted.
The corrected path calls `FBIOPAN_DISPLAY` and overlaps drawing with submission.
Later removal of a 60 Hz cap and CPU rendering improvements produced wearer-accepted
smooth orientation motion. Camera/scene tests have measured roughly 45–90 fps across
versions and panel states. These are specific test results, not a guaranteed refresh
rate or photon-latency measurement. Single-buffer scanout can still tear.

Hardware GPU rendering is unverified. The current scene is CPU-rendered; that is
separate from Lavapipe used by Vulkan runtime experiments. Do not replace the working
path with the isolated failing EGL probe.

## Acceptance

Verify a visible image (not merely an fps log), fused stereo, correct turn/tilt axes,
straight-looking lines through the lenses, frame pacing, and trigger renewal. Wait
for the wearer's observations before changing the scene. An actual framebuffer
capture proves rendered content, not through-lens comfort.

[Camera and drawing continuation](10-cameras-and-playground.md) ·
[Positional tracking](07-positional-tracking.md) ·
[Current work](09-status-and-next.md)
