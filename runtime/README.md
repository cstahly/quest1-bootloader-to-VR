# Assemble the guarded orientation-only playground

This is the ordered assembly map from a **booted, guarded pmOS rootfs** to the
current test scene. Individual components and historical cold boot were tested on
the owner's headset; this consolidated clean-install sequence has **not** been run
end-to-end on a second device. It is not a claim that `packages/*` alone installs
this system. Follow the boot/rootfs tutorial first; obtain your own stock assets
using the extraction instructions, never another headset's calibration.

## 1. Package and asset prerequisites

Build the aarch64 device package from `packages/device-oculus-monterey` (source
revision 35), together with its declared dependencies, using the tutorial's
pmbootstrap/pmaports checkout. Install those APKs into the target rootfs.
With these package directories copied into the configured pmaports tree as in
the rootfs tutorial, the package build commands are:

```sh
pmbootstrap build device-oculus-monterey
pmbootstrap build oculus-wifi-monterey
```

Resolve/install the resulting signed APKs and their declared dependencies from
that build's repository into the target rootfs; do not mix unrelated Alpine
branches. The device APKBUILD owns `/usr/sbin/oculus-{stock-runtime,ipc-security,rmtfs,controller,controller-input,controller-uinput}`,
USB recovery, device-access helpers and their OpenRC links. Do not replace these
with the old desktop launchers. The controller service's historical "desktop"
name refers to its uinput bridge; it does not require a desktop.

Next build/install `packages/oculus-wifi-monterey` (0.1-r2), provision its owner-only
firmware directory and private WPA profile as described in
[Wi-Fi bring-up](../docs/adsp-wifi.md) and
[package README](../packages/oculus-wifi-monterey/README.md). The package installs
its own copies of `runtime/oculus-wifi{,.initd}`; do not install a divergent second
copy. Its three native helpers live in `/usr/libexec/oculus-wifi`.

Provide Monado, OpenXR loader, and the owner's optical mesh. **The installed
orientation-only baseline is Monado r3; `patches/monado/APKBUILD` currently builds
experimental r8, not an exact reconstruction of that installed binary.** r5 was
an isolated positional trial, not the global runtime. The retained r3 APK and recipe at commit `b89f021` are now identified; a clean
byte-identical rebuild remains unverified. See
[asset inventory](../docs/assets-inventory.md) and
[tracking log](../docs/basalt-positional-tracking.md). Do not quietly install r8
as the baseline. Expected runtime files are `monado-service` on PATH and
`/usr/share/openxr/1/openxr_monado.json`.

Mesh destination: `/var/lib/monado/monterey/distortion-mesh.bin` (0600). The
current wall-camera lookup is `/var/lib/monado/monterey/floor-map.bin` (0600),
despite its historical filename. Generate it on a host with Python + NumPy:

```sh
python3 tools/camera/build-floor-map.py /private/path/camera_calibration_v2.json /private/path/floor-map.bin
```

The map is a fixed three-metre depth approximation, not seamless/depth-aware
passthrough. The older `passthrough-map.bin` is not the current wall's lookup.

## 2. Build the unpackaged native components

Run from this repo root inside the target-compatible **aarch64 Alpine/pmOS build
rootfs**, not the Mac host. Use the same Alpine branch as the target:

```sh
apk add build-base linux-headers libx11-dev openxr-dev
mkdir -p /tmp/quest-runtime-build
cc -O2 -Wall -Wextra -Werror runtime/oculus-recovery-guard.c -o /tmp/quest-runtime-build/oculus-recovery-guard
cc -O2 -Wall -Wextra -Werror runtime/test-recovery-deadline.c -o /tmp/quest-runtime-build/test-recovery-deadline
/tmp/quest-runtime-build/test-recovery-deadline
cc -O2 -Wall -Wextra -Werror runtime/test-recovery-guard.c -o /tmp/quest-runtime-build/test-recovery-guard
/tmp/quest-runtime-build/test-recovery-guard
cc -O2 -Wall -Wextra -Werror packages/device-oculus-monterey/oculus-controller-uinput.c -o /tmp/quest-runtime-build/oculus-controller-uinput -lm
cc -O3 -Wall -Wextra renderer/fast/monterey-head-mesh.c renderer/fast/quest-lens-mesh.c -o /tmp/quest-runtime-build/monterey-head-mesh-fb -lX11 -lopenxr_loader -lm -pthread
```

The process tests use simulated reboot exit codes; they do not invoke real
recovery/poweroff. Use the renderer's [additional tests](../renderer/fast/README.md)
with the private mesh before deployment. The bridge source in this repo contains
`BTN_TRIGGER` needed by the guard. The extra bridge build above permits verifying
that artifact independently of a possibly older installed device APK.

The camera preload is **Bionic**, not an Alpine musl shared object. Build in the
verified Linux LLVM/LLD environment with the owner's extracted Android libc/libdl:

```sh
clang --target=aarch64-linux-android29 -fuse-ld=lld -nostdlib -shared -fPIC -O2 -Wall -Wextra -Werror tools/camera/stock-probe.c -L/private/path/bionic -lc -ldl -o /private/path/stock-camera.so
```

That is the recorded build command with its private library directory parameterized.
It is not a pinned standalone NDK toolchain recipe; the library/toolchain provenance
is a remaining reproduction dependency. Keep all experimental camera switches
unset, especially `CAMERA_CPU_INVALIDATE` and `CAMERA_SCENE_BANK_ONLY`: neither is
part of the installed verified baseline. Source head includes later uninstalled
experiments even though these paths default off.

## 3. Install unpackaged files before enabling services

In the following commands `TARGET` is an **offline mounted target rootfs**, `BUILD`
is the aarch64 build output directory, and `ASSETS` is a private staging directory
containing your mesh, floor map, and Bionic `stock-camera.so`. Set all three to
absolute paths. Inspect/back up existing destinations before replacing them. Do
not replace a live guard binary: its control calls compare executable inodes.

```sh
: "${TARGET:?offline rootfs}" "${BUILD:?aarch64 build outputs}" "${ASSETS:?private assets}"
install -Dm755 "$BUILD/oculus-recovery-guard" "$TARGET/usr/sbin/oculus-recovery-guard"
install -Dm755 "$BUILD/oculus-controller-uinput" "$TARGET/usr/sbin/oculus-controller-uinput"
install -Dm755 "$BUILD/monterey-head-mesh-fb" "$TARGET/usr/libexec/oculus-test-scene/monterey-head-mesh-fb"
install -Dm755 renderer/fast/run-head-mesh-fb.sh "$TARGET/usr/libexec/oculus-test-scene/run-head-mesh-fb.sh"
install -Dm755 runtime/oculus-test-scene "$TARGET/usr/bin/oculus-test-scene"
for unit in oculus-recovery-guard oculus-camera oculus-test-scene; do
    install -Dm755 "runtime/$unit.initd" "$TARGET/etc/init.d/$unit"
done
install -Dm644 runtime/oculus-camera.confd "$TARGET/etc/conf.d/oculus-camera"
install -Dm755 tools/camera/run-stock-camera.sh "$TARGET/usr/libexec/oculus-camera/run-stock-camera.sh"
install -Dm755 tools/camera/prepare-stock-root.sh "$TARGET/usr/libexec/oculus-camera/prepare-stock-root.sh"
install -Dm755 "$ASSETS/stock-camera.so" "$TARGET/usr/libexec/oculus-camera/stock-camera.so"
install -Dm600 "$ASSETS/distortion-mesh.bin" "$TARGET/var/lib/monado/monterey/distortion-mesh.bin"
install -Dm600 "$ASSETS/floor-map.bin" "$TARGET/var/lib/monado/monterey/floor-map.bin"
```

Inspect/merge an existing camera conf rather than overwriting owner settings. The
baseline conf requests 12000 us, gain16, automatic exposure and denoising. It does
not opt into diagnostics. Preserve `spatial-ink.bin` if it exists: it is user data,
not an install input. Renderer libraries (libX11, OpenXR loader and dependencies)
must be present on target, even though the default display path is framebuffer.

## 4. First-start ordering and verification

On a **fresh guarded boot**, with the original recovery shells and `sleep 300`
still present and uptime below 150 seconds:

1. Device sysinit establishes device access; stock runtime mounts Android read-only;
   IPC security and read-only RMTFS start; controller bridge starts.
2. Start `oculus-wifi` and inspect `/run/oculus-wifi/status`, firmware check and CNSS
   logs. It checks the original guard before publishing status. Its status file
   initially says `preparing`; that alone is not proof of successful association.
3. Start `oculus-recovery-guard`. Its adoption checks both original recovery timer
   process identities and arms the replacement before terminating them. Verify
   `/usr/sbin/oculus-recovery-guard --check` and `/run/oculus-recovery/status`.
4. Start `oculus-camera`; verify all four streams in `/var/log/oculus-camera.log`.
5. Start `oculus-test-scene`; it checks the guard, starts Monado with the null
   compositor, waits up to 60 seconds for gravity initialization, then starts the
   direct framebuffer scene. Leave the headset still during initialization.

Use `rc-service NAME start` in that order, checking each result. **Do not strip
`sleep 300`/guard scaffolding from the boot recipe yet:** current Wi-Fi and
renewable-guard preflight depend on it. Do not restart Wi-Fi/modem services in
place; their vote is held until reboot. A failed preflight is a reason to inspect
the logs and return through recovery, not to bypass the check.

After the manual boot passes, enable `oculus-wifi`, `oculus-recovery-guard`,
`oculus-camera`, and `oculus-test-scene` using `rc-update add NAME default`.
The init files encode the ordering above. Device-package services already have
runlevel links. Run `sh tools/disable-desktop-autostart.sh` on target to remove
only old desktop trial links. Do **not** enable `oculus-vr-session`: its default
`oculus-vr-home` executable does not exist.

Cold-boot acceptance checks (each must pass):

```sh
rc-status default
/usr/sbin/oculus-recovery-guard --check
cat /sys/devices/soc/17817000.qcom,wdt/disable  # must remain 0
cat /run/oculus-recovery/status
ip -4 addr show wlan0
cat /run/oculus-wifi/status
tail -n 30 /var/log/oculus-camera.log
tail -n 30 /var/log/oculus-test-scene.log
tail -n 20 /tmp/monado-mesh-fb-service.log
```

Also verify SSH and Wi-Fi connectivity, scene visibility/alignment, live camera
content, and a two-second physical trigger hold followed by release increments
renewals. Software fps alone cannot prove visible frames. No Basalt worker or
positional prediction is required for this orientation-only baseline. Keep the
five-minute fallback armed; it returns to fastboot. For historical measured
results and charging/poweroff limitations see
[autostart validation](../docs/TEST-SCENE-AUTOSTART.md).
