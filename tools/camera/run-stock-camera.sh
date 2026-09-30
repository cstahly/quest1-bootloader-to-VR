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
cp "$bin/stock-camera.so" "$root/tmp/stock-camera.new.so"
mv "$root/tmp/stock-camera.new.so" "$root/tmp/stock-camera.so"
export CAMERA_EXPOSURE=1 CAMERA_ALL=1 CAMERA_MCU=1 CAMERA_STREAM=1
if [ "$mode" = live ]; then export CAMERA_LIVE=1; else unset CAMERA_LIVE; fi
unset LD_PRELOAD LD_LIBRARY_PATH
exec chroot "$root" /apex/com.android.runtime/bin/linker64 /system/bin/env \
 LD_LIBRARY_PATH=/apex/com.android.runtime/lib64/bionic:/apex/com.android.runtime/lib64:/system/lib64:/vendor/lib64:/system/lib64/vndk-29:/system/lib64/vndk-sp-29 \
 LD_PRELOAD=/tmp/stock-camera.so /system/bin/true
