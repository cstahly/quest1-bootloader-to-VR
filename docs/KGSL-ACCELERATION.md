# KGSL acceleration investigation

The absence of /dev/dri is not itself a reason to change the kernel: this headset has /dev/kgsl-3d0, and a userspace Mesa KGSL backend can potentially use it directly.

The Adreno540 distinction matters. Upstream Mesa documentation explicitly excludes a5xx and earlier from Turnip Vulkan support: https://docs.mesa3d.org/drivers/freedreno.html. Freedreno OpenGL covers a5xx. The experimental https://github.com/lfdevs/mesa-for-android-container fork includes Freedreno KGSL plus surfaceless EGL opening /dev/kgsl-3d0; source inspected at401198fbcf61f74bf7214e1be57f811fc74c8358. KGSL pipe creation queries chip_id and GMEM via kernel ioctls; both property queries succeeded on this kernel; context creation and rendering still require validation. No hardware acceleration claim yet.

packages/mesa-kgsl-monterey/APKBUILD prepares an isolated /opt/mesa-kgsl OpenGL/EGL-only build, explicitly without Turnip. It does not replace system Mesa or set global environment variables. Hardware probes should explicitly point to this build and use MESA_LOADER_DRIVER_OVERRIDE=kgsl with EGL_PLATFORM=surfaceless. Keep existing Lavapipe and signed runtime rollback available.

Monado's main compositor uses Vulkan. Success with Freedreno GL over KGSL would prove a usable accelerated GL path, but does not by itself make that Vulkan compositor accelerated. Further integration would be required. Stock Android Vulkan compatibility is another possible investigation; not implemented or verified.

Build host source /home/<user>/pmos/mesa-kgsl. Package checksum generated; build running after Monado r2 finished, because both package builds share a buildroot. Never run concurrent package builds in that shared buildroot. Khronos hello_xr built in the separate rootfs chroot and is staged on the headset.

Device read-only capability probe succeeded: chip_id0x05040001 (540), GMEM1048576bytes, UCHE_GMEM_VADDR0x100000. This confirms the two KGSL properties needed by this fork are implemented, not that GPU rendering works. Probe source tools/kgsl/probe-properties.c.

## Device result — 2026-09-30

Built and signed mesa-kgsl-monterey-r0; installed in /opt/mesa-kgsl with explicit ELF dependencies and !tracedeps to avoid advertising private EGL/GLES libraries as system replacements. !fhs allows intentional/opt isolation. Old abuild .control metadata had to be moved aside before successful repackaging. Native NIR code generation from identical pinned source replaced the extremely slow emulated generator only in local build.ninja; canonicalrecipe unchanged. Local signed APK4.2MB, installedsize19.5MB; lz4-libs additionallyinstalled from signedcache. SystemMesa untouched, no globalenv overrides.

Read-only KGSL properties succeed. Offscreen EGL probe with LD_LIBRARY_PATH=/opt/mesa-kgsl/lib EGL_PLATFORM=surfaceless MESA_LOADER_DRIVER_OVERRIDE=kgsl fails during initialization with SIGSEGV/exit139; debug output only “Native platform type: surfaceless”. No shader/render/readback success. Kernel log shows a540_zap loading and reset release, no captured GPU/IOMMU fault. This is NOT a verified hardware-accelerated path. Logs localkgsl-egl-probe.log andkgsl-egl-probe-debug.log. Leave isolated driver opt-in; proceed with owner’s Wi-Fi priority rather than substituting this unvalidated driver into acceptedrenderer/runtime.
