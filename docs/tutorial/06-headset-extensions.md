# Stage 6 — Becoming a headset: display, head tracking, lens optics

**Goal:** draw on the panel, track head *orientation* (3DoF), and correct the lens
distortion — enough to wear it and look around a scene.

**Status: diagnostic-grade.** These work in a standalone renderer and are wearer-confirmed,
but they are **not yet integrated into a finished OpenXR compositor/home**. Treat this as
"the pieces work," not "there's a VR shell."

## Concept

- **Display:** the panel is a real framebuffer (2880×1600). You can drive it via Xorg
  fbdev, or — faster — a direct-framebuffer path (~59.7 fps vs ~19.5 fps under X11).
- **Head tracking (orientation only):** fuse the syncboss IMU (gyro + accel) to get
  where you're looking. This is **3DoF** — rotation, not position. Positional (walking)
  is stage 7.
- **Lens optics:** the panel image must be pre-distorted to cancel the lenses' pincushion.
  The stock **distortion mesh** was reverse-engineered from your system image; the
  renderer applies it. (The mesh blob is Meta's IP and device-specific — extract from
  *your* device; only the *format* is documented here.)

## Steps (on the booted headset, over SSH)

**1. Stage the renderer + your mesh** to `/tmp` (both renderers look for
`distortion-mesh.bin` beside the executable):

```
# from build host: tar the built renderer(s) + your extracted distortion-mesh.bin, scp, untar in /tmp
/tmp/test-lens-mesh /tmp/distortion-mesh.bin        # sanity-check the mesh loads
```

**2. Static display/perf benchmark** (no sensors, no wearer needed):

```
DISPLAY=:0 timeout 12 /tmp/monterey-head-mesh      --static
DISPLAY=:0 timeout 20 /tmp/monterey-head-mesh-fast --static
```

**3. Start one Monado service for tracked mode** (never start a second runtime):

```
DISPLAY=:0 XDG_RUNTIME_DIR=/run/user/10000 \
XR_RUNTIME_JSON=/usr/share/openxr/1/openxr_monado.json \
XRT_COMPOSITOR_NULL=1 XRT_NO_STDIN=1 MONTEREY_LOG=info \
monado-service &        # check log for gravity init + Quest presence
```

**4. Tracked scene** — *physical action, then wait:*

> **Ask the wearer:** "Put the headset on and look forward." **Then pause and wait for
> their confirmation** before launching the scene, so heading points forward. (Agents:
> do not proceed on your own.)

```
DISPLAY=:0 /tmp/monterey-head-mesh-fast     # launch only after the wearer confirms
```

**5. Ask, then wait:** "Is motion smooth and do the lines still look correct?" Don't
change the scene while waiting; mind the watchdog budget.

## Definition of done

- [ ] `--static` benchmark runs and reports a stable frame rate.
- [ ] With Monado running and the headset worn, turning your head moves the view the
      right way (wearer-confirmed).
- [ ] Straight lines look straight through the lenses (distortion corrected,
      wearer-confirmed).

## Honest limits

- This is **3DoF** (look around), **no positional** "around" — that's stage 7.
- It's a **diagnostic scene**, not an OpenXR home/compositor. Optics live in the
  standalone renderer, not yet in the Monado runtime.
- Rendering is **software** (see stage 8: no Vulkan driver for the Adreno 540). Measured
  cadence wandered 72–90 Hz across tests; don't claim a fixed panel rate.
- **Don't** `cat` `fb0`/`partial_vsync` sysfs on this kernel — a read triggers a
  NULL-pointer kernel oops. Known broken endpoint.

## Full detail

`../tracking-optics-performance.md` (tracking fix, lens mesh, performance, the exact
resume sequence) and `../VR-RUNTIME-IN-PROGRESS.md` (runtime integration state).

→ Next: [`07-positional-tracking.md`](07-positional-tracking.md)
