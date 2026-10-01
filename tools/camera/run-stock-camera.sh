#!/bin/sh
# Native launcher; Bionic preload applies only inside the read-only stock root.
set -eu
mode=${1:-probe}
case "$mode" in probe|live) ;; *) echo 'usage: run-stock-camera.sh [probe|live]' >&2; exit 2;; esac
/usr/sbin/oculus-recovery-guard --check
[ "$(cat /sys/devices/soc/17817000.qcom,wdt/disable)" = 0 ]
[ "$(cat /sys/devices/virtual/misc/syncboss0/spi/control/num_cameras)" = 4 ]
exec 9>/run/quest-camera.lock
flock -n 9 || { echo 'Camera capture is already owned' >&2; exit 2; }
bin=${0%/*}
root=/run/quest-camera-root
if [ ! -d "$root" ]; then sh "$bin/prepare-stock-root.sh"; fi
[ -f "$root/system/vendor/lib64/libqcameraoculushal.so" ]
[ -c "$root/dev/syncboss0" ]
if [ "${CAMERA_DIAGNOSTIC_BANK_STEP:-0}" = 1 ] && { [ "${CAMERA_DIAGNOSTIC_BANK0_US+x}" != x ] || [ "${CAMERA_DIAGNOSTIC_BANK1_US+x}" != x ]; }; then
    echo 'BANK_DIAGNOSTIC step requires both diagnostic bank values; refusing to launch' >&2
    exit 2
fi
if [ "${CAMERA_DIAGNOSTIC_BANK0_US+x}" = x ] || [ "${CAMERA_DIAGNOSTIC_BANK1_US+x}" = x ]; then
    if [ "${CAMERA_FRAME_TRACE:-0}" != 1 ] || [ "${CAMERA_AUTO_EXPOSURE:-0}" != 0 ]; then
        echo 'BANK_DIAGNOSTIC requires FRAME_TRACE=1 and AUTO_EXPOSURE=0; refusing to launch' >&2
        exit 2
    fi
fi
if [ "${CAMERA_SCENE_BANK_ONLY:-0}" = 1 ] && { [ "${CAMERA_DIAGNOSTIC_BANK0_US+x}" = x ] || [ "${CAMERA_DIAGNOSTIC_BANK1_US+x}" = x ] || [ "${CAMERA_DIAGNOSTIC_BANK_STEP:-0}" = 1 ] || [ "${CAMERA_RAW_CAPTURE:-0}" = 1 ]; }; then
    echo 'SCENE_BANK_ONLY incompatible with diagnostic bank override/step/raw capture; refusing to launch' >&2
    exit 2
fi
if [ "${CAMERA_RAW_CAPTURE:-0}" = 1 ] || [ "${CAMERA_FRAME_TRACE:-0}" = 1 ] || [ "${CAMERA_SCENE_BANK_ONLY:-0}" = 1 ] || [ "${CAMERA_CPU_INVALIDATE:-0}" = 1 ]; then
    # Raw capture, tracing and scene selection use the private HAL layout. Fail closed
    # for any other owner library; normal feed operation keeps its existing path.
    if ! raw_hal_digest=$(sha256sum "$root/system/vendor/lib64/libqcameraoculushal.so"); then
        echo 'CAMERA diagnostic cannot verify owner HAL; refusing to launch' >&2
        exit 2
    fi
    raw_hal_digest=${raw_hal_digest%% *}
    if [ "$raw_hal_digest" != 109b418cec3182c069d20bda7e659666ce9202d731d1253c3302c4d5ff30dd86 ]; then
        echo 'CAMERA diagnostic unsupported owner HAL hash; refusing to launch' >&2
        exit 2
    fi
fi
cp "$bin/stock-camera.so" "$root/tmp/stock-camera.new.so"
mv "$root/tmp/stock-camera.new.so" "$root/tmp/stock-camera.so"
export CAMERA_EXPOSURE=1 CAMERA_ALL=1 CAMERA_MCU=1 CAMERA_STREAM=1
if [ "$mode" = live ]; then export CAMERA_LIVE=1; else unset CAMERA_LIVE; fi
unset LD_PRELOAD LD_LIBRARY_PATH
exec chroot "$root" /apex/com.android.runtime/bin/linker64 /system/bin/env \
 LD_LIBRARY_PATH=/apex/com.android.runtime/lib64/bionic:/apex/com.android.runtime/lib64:/system/lib64:/vendor/lib64:/system/lib64/vndk-29:/system/lib64/vndk-sp-29 \
 LD_PRELOAD=/tmp/stock-camera.so /system/bin/true
