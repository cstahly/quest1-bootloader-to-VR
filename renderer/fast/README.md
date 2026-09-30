# Default scene and live diagnostics (2026-09-30)

The installed boot default now launches this tracked framebuffer diagnostic with a
head-locked optical HUD and independent trigger-renewable recovery countdown. See
[`../../docs/TEST-SCENE-AUTOSTART.md`](../../docs/TEST-SCENE-AUTOSTART.md). Cold-boot
scene/Wi-Fi startup and six real trigger renewals were verified. Manual launches keep
the180s limit; guarded default sessions continue while the independent timer is live.
The wrapper now waits for actual gravity initialization (up to60s). The following
older performance notes remain useful history; their no-autostart/no-input statements
are superseded by the dedicated recovery service and current implementation.

# Diagnostic renderer performance

The cached X11 version still runs at approximately19.5fps on Quest1. The first direct framebuffer implementation ran the same static scene at approximately59.7fps, with about11.4ms drawing and3.1ms copying per frame, but showed black because it omitted MDSS display submissions. This is NOT a valid displayed-frame result. The corrected path unblanks the existing mode and calls FBIOPAN_DISPLAY after each copy; The corrected pipeline uses two CPU buffers and a submission worker so draw and driver wait overlap. Static on-device measurement:59.7fps render loop and59.8fps in the MDSS driver measured_fps counter. Wearer confirmed a visible scene and definitely better motion, but remaining stutter is unresolved. Projection math, per-channel lens warp, stereo centers, heading and gravity conventions are shared.

Build in the existing ARM rootfs chroot with build-base, linux-headers, libx11-dev and openxr-dev:

```sh
cc -O3 -Wall -Wextra monterey-head-mesh.c quest-lens-mesh.c -o monterey-head-mesh-fb -lX11 -lopenxr_loader -lm -pthread
cc -O2 test-framebuffer.c -o test-framebuffer -pthread
./test-framebuffer
cc -O3 test-lens-mesh.c quest-lens-mesh.c -o test-lens-mesh -lm
./test-lens-mesh /path/to/stock/distortion-mesh.bin
```

Place binary, run-head-mesh-fb.sh and the device's stock distortion-mesh.bin together. On the headset run:

```sh
./run-head-mesh-fb.sh --static   # benchmark without tracking
./run-head-mesh-fb.sh            # tracked view, 12-second calibration wait
```

The wrapper temporarily stops oculus-desktop-trial and terminates its remaining Xorg process, refuses conflicting runtime/display ownership, and leaves the desktop stopped on exit (owner requires VR-only operation). The wrapper does not change boot configuration or the recovery watchdog. Use `../../tools/disable-desktop-autostart.sh` to remove desktop boot links. A demo lasts at most180seconds; the guarded boot can end sooner. For short benchmarks use `timeout 20 ./run-head-mesh-fb.sh --static`; child-process cleanup was verified on-device. Wearer should be looking forward when the tracked scene starts.

MONTEREY_FRAMEBUFFER=1 selects the direct backend in the executable. Without it the X11 path remains available. MONTEREY_NO_PRESENT=1 suppresses visible X11 copies for profiling only. Frame timing reports pose, draw, present and cumulative loop rate. Direct mode follows the bounded driver-submission worker instead of a60Hz sleep. Every240 submissions it reports interval p50/p95/p99/max and counts intervals over25ms. On this headset, display cadence measured72Hz; removing the60Hz cap eliminated recurring27.78ms intervals in the static test.

Direct access validates exact2880x1600 32bpp stride and supported RGB layouts before mmap; it never changes the display mode. Color masks are converted to the queried channel order. Logical pixels are rotated180degrees before scanout; each eye is clipped independently. Lines retain24 projection subdivisions and RGB correction, with a3px software rasterizer.

Limitations: framebuffer copies use a single scanout buffer and may tear; submissions request FB_ACTIVATE_VBL but the two CPU buffers do not provide atomic scanout page flipping. There is no GPU acceleration or systemwide Monado optics integration here. Direct mode does not currently receive X11 controller/key recenter events. Tracked motion and image orientation require wearer validation; static fps alone is insufficient. Stock proprietary mesh and built executables are not committed.

For separate physical calibration/readiness, set `MONTEREY_START_FILE` to a fresh nonexistent path. The wrapper starts/calibrates the runtime, then waits for that file before starting the scene. Verify `Gravity initialized` in `/tmp/monado-mesh-fb-service.log`, ask the wearer to put it on, and create the file only after their readiness reply. Do not reuse a stale gate file.
