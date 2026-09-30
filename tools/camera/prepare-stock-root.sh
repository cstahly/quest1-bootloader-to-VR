#!/bin/sh
# Bounded stock HAL diagnostics. Owner libraries stay read-only; writable data is RAM.
set -eu
/usr/sbin/oculus-recovery-guard --check
[ "$(cat /sys/devices/soc/17817000.qcom,wdt/disable)" = 0 ]
[ "$(cat /sys/devices/virtual/misc/syncboss0/spi/control/num_cameras)" = 4 ]
root=/run/quest-camera-root
[ ! -e "$root" ] || { echo 'Camera root already exists; inspect before reusing' >&2; exit 2; }
mkdir -p "$root/system" "$root/vendor" "$root/apex" "$root/proc" "$root/sys" "$root/dev" "$root/data" "$root/tmp"
mount --bind /run/oculus-stock-runtime/system "$root/system"
mount -o remount,bind,ro,nosuid,nodev,exec "$root/system"
ln -s /system/vendor/lib64 "$root/vendor/lib64"
ln -s /system/vendor/etc "$root/vendor/etc"
ln -s /system/apex/com.android.runtime.release "$root/apex/com.android.runtime"
for fs in proc sys; do
    mount --bind "/$fs" "$root/$fs"
    mount -o remount,bind,ro,nosuid,nodev,noexec "$root/$fs"
done
# No block devices, persist, NV, firmware loaders or Android service manager.
for device in /dev/null /dev/urandom /dev/random /dev/ion /dev/media* /dev/video* /dev/v4l-subdev* /dev/syncboss0 /dev/syncboss_control0; do
    [ -c "$device" ] || continue
    name=${device##*/}
    touch "$root/dev/$name"
    mount --bind "$device" "$root/dev/$name"
done
echo "Prepared $root"
