# 6 — Display / head tracking / optics

Draw on the panel, track head *orientation* (3DoF), correct the lens distortion — enough
to wear it and look around a scene. **Diagnostic-grade:** the pieces work in a standalone
renderer (wearer-confirmed), but there's no finished OpenXR compositor/home yet.

## What's what

- **Display:** real framebuffer 2880×1600. Xorg fbdev, or the direct-framebuffer path
  (~59.7 fps vs ~19.5 under X11).
- **Head tracking:** syncboss IMU gyro+accel fusion → where you're looking. **3DoF**
  (rotation only). Positional/walking is stage 7.
- **Optics:** pre-distort the panel image to cancel the lenses. The stock **distortion
  mesh** was RE'd from your system image; the renderer applies it. Mesh is device-specific
  Meta IP — pull from your device; only the format's documented here.

## Run it (on the headset, over SSH)

Stage the renderer + your `distortion-mesh.bin` to `/tmp` (both renderers look for the
mesh beside the executable):

```
/tmp/test-lens-mesh /tmp/distortion-mesh.bin        # mesh loads?
```

Static display/perf bench (no sensors, no wearer):

```
DISPLAY=:0 timeout 12 /tmp/monterey-head-mesh      --static
DISPLAY=:0 timeout 20 /tmp/monterey-head-mesh-fast --static
```

One Monado service for tracked mode (don't start a second runtime):

```
DISPLAY=:0 XDG_RUNTIME_DIR=/run/user/10000 \
XR_RUNTIME_JSON=/usr/share/openxr/1/openxr_monado.json \
XRT_COMPOSITOR_NULL=1 XRT_NO_STDIN=1 MONTEREY_LOG=info \
monado-service &        # log should show gravity init + Quest presence
```

Tracked scene — heading initializes to wherever you're looking, so launch it once the
headset's on and facing forward:

```
DISPLAY=:0 /tmp/monterey-head-mesh-fast
```

## Worked when

- `--static` runs at a stable rate
- with Monado up and the headset worn, turning your head moves the view the right way
- straight lines look straight through the lenses

## Honest limits

- **3DoF only** — no positional "around" (that's stage 7).
- Diagnostic scene, not an OpenXR home. Optics live in the standalone renderer, not the
  Monado runtime yet.
- **Software rendering** — no Vulkan driver for the Adreno 540 (stage 8). Cadence wandered
  72–90 Hz across tests; don't trust a fixed panel rate.
- Don't `cat` `fb0`/`partial_vsync` sysfs — a read oopses this kernel.

Detail: `../tracking-optics-performance.md`, `../VR-RUNTIME-IN-PROGRESS.md`.

→ [7 — positional tracking](07-positional-tracking.md)
