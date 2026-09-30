# Isolated KGSL probes

Build for aarch64 Alpine/musl. `probe-properties.c` only opens KGSL and queries device information and UCHE GMEM base; it allocates no GPU context and submits no work. `probe-egl.c` creates a16x16 offscreen GLES2 pbuffer, clears it, compiles GLES2 shaders, draws a triangle, and checks its pixel readback and renderer string. This establishes a basic shader/draw/readback path, not Vulkan support.

```sh
cc -O2 probe-properties.c -o probe-properties
cc -O2 probe-egl.c -lEGL -lGLESv2 -o probe-egl
./probe-properties
LD_LIBRARY_PATH=/opt/mesa-kgsl/lib EGL_PLATFORM=surfaceless \
 MESA_LOADER_DRIVER_OVERRIDE=kgsl timeout 20 ./probe-egl
```

Run the EGL probe only after staging the isolated package and inspecting its dependencies. Keep system Mesa untouched. Confirm output actually identifies Adreno540/Freedreno and reports the expected pixel; loading EGL alone is insufficient. Never set these environment variables globally. These probes do not change framebuffer mode or factory calibration. Device recovery watchdog remains enabled.
